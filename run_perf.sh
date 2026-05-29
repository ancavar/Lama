#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR"

DRIVER="${DRIVER:-$ROOT/_build/default/src/Driver.exe}"
VM="${VM:-$ROOT/_build/default/virtual_machine/lama.exe}"
RUNTIME_DIR="${RUNTIME_DIR:-$ROOT/_build/default/runtime}"
STDLIB_DIR="${STDLIB_DIR:-$ROOT/_build/default/stdlib/x64}"

BENCH_BRANCH="${BENCH_BRANCH:-bench}"
OLD_BRANCH="${OLD_BRANCH:-origin/modules}"
OLD_DRIVER="${OLD_DRIVER:-$ROOT/old_Driver.exe}"
OLD_VM="${OLD_VM:-$ROOT/old_vm.exe}"
OLD_BC_DIR="${OLD_BC_DIR:-$ROOT/.bench-old-bytecode}"

WORKDIR="$(mktemp -d /tmp/lama-perf.XXXXXX)"
CSV="${CSV:-$PWD/results.csv}"

RUNS="${RUNS:-1}"
WARMUPS="${WARMUPS:-1}"
CPU="${CPU:-4}"

csv_header() {
  printf 'label,run,wall_seconds,max_rss_kb\n' >"$CSV"
}

switch_back_to_bench() {
  git -C "$ROOT" switch "$BENCH_BRANCH" >/dev/null
}

build_old_artifacts() {
  echo "building old artifacts from '$OLD_BRANCH'"

  git -C "$ROOT" switch --detach "$OLD_BRANCH" >/dev/null
  trap switch_back_to_bench RETURN

  (cd "$ROOT" && dune clean && dune b ./src/Driver.exe)
  (cd "$ROOT/virtual_machine" && make clean && make)

  cp "$ROOT/_build/default/src/Driver.exe" "$OLD_DRIVER"
  cp "$ROOT/virtual_machine/interpreter.exe" "$OLD_VM"
  chmod +x "$OLD_DRIVER" "$OLD_VM"

  trap - RETURN
  switch_back_to_bench
}

build_current_artifacts() {
  echo "building current Driver.exe, lama.exe, runtime, and stdlib"
  (cd "$ROOT" && dune b ./src/Driver.exe runtime stdlib/x64 virtual_machine)
}

compile_old_stdlib() {
  echo "building old VM stdlib bytecode in $OLD_BC_DIR"
  mkdir -p "$OLD_BC_DIR"

  local module
  for module in "$ROOT"/stdlib/*.lama; do
    [[ -e "$module" ]] || continue
    (cd "$OLD_BC_DIR" && "$OLD_DRIVER" -b "$module")
  done
}

compile_native() {
  local source="$1" test_name="$2"
  (cd "$WORKDIR" && "$DRIVER" -runtime "$RUNTIME_DIR" -I "$STDLIB_DIR" -o "$WORKDIR/$test_name" "$source")
}

compile_bytecode() {
  local source="$1"
  (cd "$WORKDIR" && "$DRIVER" -runtime "$RUNTIME_DIR" -I "$STDLIB_DIR" -b "$source")
}

compile_old_bytecode() {
  local source="$1"
  (cd "$OLD_BC_DIR" && "$OLD_DRIVER" -b "$source")
}

run_cmd() {
  local stdin_file="$1"
  shift
  if [[ -n "$stdin_file" && -f "$stdin_file" ]]; then
    "$@" <"$stdin_file" >/dev/null
  else
    "$@" </dev/null >/dev/null
  fi
}

measure_case() {
  local label="$1" stdin_file="$2"
  shift 2

  local cmd=("$@")
  [[ -n "${CPU:-}" ]] && cmd=(taskset -c "$CPU" "${cmd[@]}")

  for _ in $(seq 1 "$WARMUPS"); do
    run_cmd "$stdin_file" "${cmd[@]}"
  done

  for run in $(seq 1 "$RUNS"); do
    local result
    if [[ -n "$stdin_file" && -f "$stdin_file" ]]; then
      result="$({ /usr/bin/time -f '%e,%M' "${cmd[@]}" <"$stdin_file" >/dev/null; } 2>&1)"
    else
      result="$({ /usr/bin/time -f '%e,%M' "${cmd[@]}" >/dev/null; } 2>&1)"
    fi
    printf '%s,%s,%s\n' "$label" "$run" "$result" >>"$CSV"
  done
}

run_source() {
  local suite="$1" source="$2" source_dir="$3"

  local test_name input_file=""
  test_name="$(basename "${source%.lama}")"
  [[ -f "$source_dir/$test_name.input" ]] && input_file="$source_dir/$test_name.input"

  local label_prefix="${suite//\//_}-${test_name}"

  compile_native "$source" "$test_name"
  measure_case "${label_prefix}-native" "$input_file" "$WORKDIR/$test_name"

  compile_bytecode "$source"
  measure_case "${label_prefix}-vm" "$input_file" "$VM" -I "$STDLIB_DIR" "$WORKDIR/$test_name.bc"

  compile_old_bytecode "$source"
  measure_case "${label_prefix}-old-vm" "$input_file" "$OLD_VM" "$OLD_BC_DIR/$test_name.bc"
}

for required in git dune make cp chmod mkdir /usr/bin/time; do
  if ! command -v "$required" >/dev/null; then
    echo "missing required command: $required" >&2
    exit 1
  fi
done

build_old_artifacts
build_current_artifacts
compile_old_stdlib

echo "benchmark work dir: $WORKDIR"

csv_header

run_source "performance" "$ROOT/performance/Ackermann.lama" "$ROOT/performance"
run_source "performance" "$ROOT/performance/GenCyclicArrays.lama" "$ROOT/performance"

for source in "$ROOT"/regression/test*.lama; do
  [[ -e "$source" ]] || continue
  run_source "regression" "$source" "$ROOT/regression"
done

for source in "$ROOT"/stdlib/regression/test*.lama; do
  [[ -e "$source" ]] || continue
  run_source "stdlib/regression" "$source" "$ROOT/stdlib/regression"
done

echo "results: $CSV"
