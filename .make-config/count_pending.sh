#!/usr/bin/env bash
# Counts how many of the given object files are missing or stale, relative to the given flags file.

set -euo pipefail

flags_file="$1"
shift
count=0

for obj in "$@"; do
    if [ ! -f "$obj" ]; then
        count=$((count + 1))
        continue
    fi

    if [ -e "$flags_file" ] && [ "$flags_file" -nt "$obj" ]; then
        count=$((count + 1))
        continue
    fi

    dep="${obj%.o}.d"
    if [ -f "$dep" ]; then
        stale=0
        for f in $(sed -e 's/^[^:]*:[[:space:]]*//' -e 's/\\$//' "$dep"); do
            if [ ! -e "$f" ] || [ "$f" -nt "$obj" ]; then
                stale=1
                break
            fi
        done
        [ "$stale" -eq 1 ] && count=$((count + 1))
    fi
done

echo "$count"
