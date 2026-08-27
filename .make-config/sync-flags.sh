#!/usr/bin/env bash
# Writes $2 (a fingerprint of the active build flags) into file $1, but only
# touches it when the content actually changed.

set -euo pipefail

target="$1"
content="$2"

mkdir -p "$(dirname "$target")"

if [ ! -f "$target" ] || [ "$(cat "$target" 2>/dev/null)" != "$content" ]; then
    printf '%s\n' "$content" >"$target"
fi
