#!/usr/bin/env bash
# Pretty compile progress bar, driven by an actual per-run counter.

set -euo pipefail

src="$1"
pad="$2"
counter_file="$3"
total="$4"

name=$(basename "$src")

lock_file="$counter_file.lock"
exec 200>"$lock_file"
flock -x 200
count=$(($(cat "$counter_file" 2>/dev/null || echo 0) + 1))
printf '%s' "$count" >"$counter_file"
flock -u 200

if [ "$count" -gt 1 ]; then
    printf '\033[1A\033[2K'
fi

printf '%b' "${COLOR_INFO-}[ ${COLOR_SUCCESS-}"
printf '%-*s' "$pad" "$name"
printf '%b' " ${COLOR_INFO-}]: ${OK_MARK-OK}\n"

barlen=30
if [ "$total" -gt 0 ]; then
    filled=$((count * barlen / total))
    [ "$filled" -gt "$barlen" ] && filled=$barlen
    pct=$((count * 100 / total))
else
    filled=$barlen
    pct=100
fi
empty=$((barlen - filled))

filled_str=$(printf '%*s' "$filled" '' | tr ' ' '#')
empty_str=$(printf '%*s' "$empty" '' | tr ' ' '.')

printf '%b\n' "${COLOR_CYAN-}[${COLOR_SUCCESS-}${filled_str}${COLOR_RESET-}${empty_str}${COLOR_CYAN-}] ${COLOR_INFO-}${pct}%% (${count}/${total})${COLOR_RESET-}"
