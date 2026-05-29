#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR"

VM="${VM:-$ROOT/_build/default/virtual_machine/lama.exe}"

BENCH_BRANCH="${BENCH_BRANCH:-bench}"
OLD_BRANCH="${OLD_BRANCH:-origin/modules}"
OLD_VM="${OLD_VM:-$ROOT/old_vm.exe}"
BUILD_OLD_DRIVER=0
BUILD_OLD_ARTIFACTS="${BUILD_OLD_ARTIFACTS:-1}"
BUILD_CURRENT_ARTIFACTS="${BUILD_CURRENT_ARTIFACTS:-1}"

WORKDIR="$(mktemp -d /tmp/lama-compiler-perf.XXXXXX)"
CSV="${CSV:-$PWD/compiler_results.csv}"
PAYLOAD_DIR="${PAYLOAD_DIR:-$ROOT/compiler}"

RUNS="${RUNS:-1}"
WARMUPS="${WARMUPS:-1}"
CPU="${CPU:-4}"
MAX_CASES="${MAX_CASES:-0}"

. "$ROOT/perf_common.sh"

csv_header() {
  printf 'label,run,wall_seconds,max_rss_kb,asm_sha256\n' >"$CSV"
}

build_current_artifacts() {
  echo "building current lama.exe"
  (cd "$ROOT" && dune b runtime virtual_machine)
}

use_payload() {
  NATIVE="$PAYLOAD_DIR/src/lama-impl"
  NEW_BC_DIR="$PAYLOAD_DIR/src/new-vm"
  OLD_BC_DIR="$PAYLOAD_DIR/src/old-vm"
}

run_source() {
  local suite="$1" source="$2"

  local test_name label_prefix work_source asm_file
  test_name="$(basename "${source%.lama}")"
  label_prefix="${suite//\//_}-${test_name}"
  work_source="$WORKDIR/$label_prefix.lama"
  asm_file="${work_source%.lama}.s"

  cp "$source" "$work_source"

  measure_compiler "${label_prefix}-native-compile" "$WORKDIR" "$asm_file" "$NATIVE" "$work_source" || true
  measure_compiler "${label_prefix}-vm-compile" "$WORKDIR" "$asm_file" "$VM" -I "$NEW_BC_DIR" "$NEW_BC_DIR/Driver.bc" -- "$work_source" || true
  measure_compiler "${label_prefix}-old-vm-compile" "$OLD_BC_DIR" "$asm_file" "$OLD_VM" Driver.bc "$work_source" || true
}

run_suite() {
  local suite="$1"
  local dir="$PAYLOAD_DIR/$suite"
  local count=0
  local source

  for source in "$dir"/*.lama; do
    [[ -e "$source" ]] || continue
    run_source "$suite" "$source"
    count=$((count + 1))
    if [[ "$MAX_CASES" != "0" && "$count" -ge "$MAX_CASES" ]]; then
      break
    fi
  done
}

if [[ "$BUILD_OLD_ARTIFACTS" = "1" ]]; then
  build_old_artifacts
fi
if [[ "$BUILD_CURRENT_ARTIFACTS" = "1" ]]; then
  build_current_artifacts
fi
use_payload

echo "compiler payload: $PAYLOAD_DIR"
echo "benchmark work dir: $WORKDIR"

csv_header
run_suite "regression/expressions"
run_suite "regression/deep-expressions"

echo "results: $CSV"
