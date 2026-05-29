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

. "$ROOT/perf_common.sh"

csv_header() {
  printf 'label,run,wall_seconds,max_rss_kb\n' >"$CSV"
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

run_source() {
  local suite="$1" source="$2" source_dir="$3"

  local test_name input_file=""
  test_name="$(basename "${source%.lama}")"
  [[ -f "$source_dir/$test_name.input" ]] && input_file="$source_dir/$test_name.input"

  local label_prefix="${suite//\//_}-${test_name}"

  if compile_native "$source" "$test_name"; then
    measure_case "${label_prefix}-native" "$input_file" "$WORKDIR/$test_name" || true
  else
    echo "skip: ${label_prefix}-native compile failed" >&2
  fi

  if compile_bytecode "$source"; then
    measure_case "${label_prefix}-vm" "$input_file" "$VM" -I "$STDLIB_DIR" "$WORKDIR/$test_name.bc" || true
  else
    echo "skip: ${label_prefix}-vm bytecode compile failed" >&2
  fi

  if compile_old_bytecode "$source"; then
    measure_case "${label_prefix}-old-vm" "$input_file" "$OLD_VM" "$OLD_BC_DIR/$test_name.bc" || true
  else
    echo "skip: ${label_prefix}-old-vm bytecode compile failed" >&2
  fi
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
run_source "performance" "$ROOT/performance/Sort.lama" "$ROOT/performance"

echo "results: $CSV"
