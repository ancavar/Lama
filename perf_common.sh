build_old_artifacts() {
  echo "building old artifacts from '$OLD_BRANCH'"

  local old_root
  old_root="$(mktemp -d /tmp/lama-old.XXXXXX)"
  git -C "$ROOT" worktree add --detach "$old_root" "$OLD_BRANCH" >/dev/null
  trap 'git -C "$ROOT" worktree remove --force "$old_root" >/dev/null' RETURN

  if [[ "${BUILD_OLD_DRIVER:-1}" = "1" ]]; then
    (cd "$old_root" && dune clean && dune b ./src/Driver.exe)
  fi
  (cd "$old_root/virtual_machine" && make clean && make)

  if [[ "${BUILD_OLD_DRIVER:-1}" = "1" ]]; then
    rm -f "$OLD_DRIVER"
    cp "$old_root/_build/default/src/Driver.exe" "$OLD_DRIVER"
    chmod u+w,+x "$OLD_DRIVER"
  fi
  rm -f "$OLD_VM"
  if [[ -x "$old_root/virtual_machine/interpreter.exe" ]]; then
    cp "$old_root/virtual_machine/interpreter.exe" "$OLD_VM"
  else
    cp "$old_root/virtual_machine/lama.exe" "$OLD_VM"
  fi
  chmod u+w,+x "$OLD_VM"

  trap - RETURN
  git -C "$ROOT" worktree remove --force "$old_root" >/dev/null
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

run_in_dir() {
  local run_dir="$1"
  shift
  (cd "$run_dir" && "$@")
}

measure_case() {
  local label="$1" stdin_file="$2"
  shift 2

  local cmd=("$@")
  local rss_out
  rss_out="$(mktemp "$WORKDIR/.rss.XXXXXX")"
  [[ -n "${CPU:-}" ]] && cmd=(taskset -c "$CPU" "${cmd[@]}")

  for _ in $(seq 1 "$WARMUPS"); do
    if ! run_cmd "$stdin_file" "${cmd[@]}"; then
      echo "skip: $label warmup failed" >&2
      rm -f "$rss_out"
      return 1
    fi
  done

  local run
  for run in $(seq 1 "$RUNS"); do
    local start_ns end_ns wall_seconds max_rss_kb
    start_ns="$(date +%s%N)"
    if [[ -n "$stdin_file" && -f "$stdin_file" ]]; then
      if ! /usr/bin/time -f '%M' -o "$rss_out" "${cmd[@]}" <"$stdin_file" >/dev/null; then
        echo "skip: $label run $run failed" >&2
        rm -f "$rss_out"
        return 1
      fi
    else
      if ! /usr/bin/time -f '%M' -o "$rss_out" "${cmd[@]}" </dev/null >/dev/null; then
        echo "skip: $label run $run failed" >&2
        rm -f "$rss_out"
        return 1
      fi
    fi
    end_ns="$(date +%s%N)"
    wall_seconds="$(awk -v s="$start_ns" -v e="$end_ns" 'BEGIN { printf "%.9f", (e - s) / 1000000000 }')"
    max_rss_kb="$(tr -d '[:space:]' <"$rss_out")"
    printf '%s,%s,%s,%s\n' "$label" "$run" "$wall_seconds" "$max_rss_kb" >>"$CSV"
  done
  rm -f "$rss_out"
}

measure_compiler() {
  local label="$1" run_dir="$2" asm_file="$3"
  shift 3

  local cmd=("$@")
  local rss_out
  rss_out="$(mktemp "$WORKDIR/.rss.XXXXXX")"
  [[ -n "${CPU:-}" ]] && cmd=(taskset -c "$CPU" "${cmd[@]}")

  for _ in $(seq 1 "$WARMUPS"); do
    rm -f "$asm_file"
    if ! run_in_dir "$run_dir" "${cmd[@]}" </dev/null >/dev/null; then
      echo "skip: $label warmup failed" >&2
      rm -f "$rss_out"
      return 1
    fi
    if [[ ! -f "$asm_file" ]]; then
      echo "skip: $label warmup did not produce $asm_file" >&2
      rm -f "$rss_out"
      return 1
    fi
  done

  local run
  for run in $(seq 1 "$RUNS"); do
    local start_ns end_ns wall_seconds max_rss_kb asm_hash
    rm -f "$asm_file"
    start_ns="$(date +%s%N)"
    if ! /usr/bin/time -f '%M' -o "$rss_out" bash -c 'cd "$1" && shift && exec "$@"' bash "$run_dir" "${cmd[@]}" </dev/null >/dev/null; then
      echo "skip: $label run $run failed" >&2
      rm -f "$rss_out"
      return 1
    fi
    end_ns="$(date +%s%N)"
    if [[ ! -f "$asm_file" ]]; then
      echo "skip: $label run $run did not produce $asm_file" >&2
      rm -f "$rss_out"
      return 1
    fi
    wall_seconds="$(awk -v s="$start_ns" -v e="$end_ns" 'BEGIN { printf "%.9f", (e - s) / 1000000000 }')"
    max_rss_kb="$(tr -d '[:space:]' <"$rss_out")"
    asm_hash="$(sha256sum "$asm_file" | awk '{ print $1 }')"
    printf '%s,%s,%s,%s,%s\n' "$label" "$run" "$wall_seconds" "$max_rss_kb" "$asm_hash" >>"$CSV"
  done

  rm -f "$rss_out"
}
