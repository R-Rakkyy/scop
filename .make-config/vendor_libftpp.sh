#!/usr/bin/env bash
# Clones (if missing) and builds libftpp into $1, producing $1/libftpp.a.
# headers are under $1/include/

set -euo pipefail

dir="$1"
repo="https://github.com/R-Rakkyy/libftpp.git"

command -v git >/dev/null 2>&1 || { echo "vendor_libftpp: git is required to fetch libftpp" >&2; exit 1; }

if [ ! -d "$dir/.git" ]; then
    echo "vendor_libftpp: cloning libftpp into $dir" >&2
    rm -rf "$dir"
    git clone --depth 1 "$repo" "$dir"
fi

echo "vendor_libftpp: building" >&2
"${MAKE:-make}" -C "$dir" >/dev/null

test -f "$dir/libftpp.a"
