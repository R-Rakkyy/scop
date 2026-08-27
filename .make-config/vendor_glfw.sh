#!/usr/bin/env bash
# Clones (if missing) and builds a static GLFW into $1, producing
# $1/build/src/libglfw3.a and headers under $1/include/
# Only invoked when `pkg-config glfw3` can't find a system copy.

set -euo pipefail

dir="$1"
build="$dir/build"
repo="https://github.com/glfw/glfw.git"

command -v git >/dev/null 2>&1 || { echo "vendor_glfw: git is required to fetch GLFW" >&2; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "vendor_glfw: cmake is required to build GLFW" >&2; exit 1; }

if [ ! -d "$dir/.git" ]; then
    echo "vendor_glfw: cloning GLFW into $dir" >&2
    rm -rf "$dir"
    git clone --depth 1 "$repo" "$dir"
fi

echo "vendor_glfw: configuring" >&2
cmake -S "$dir" -B "$build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DGLFW_BUILD_EXAMPLES=OFF \
    -DGLFW_BUILD_TESTS=OFF \
    -DGLFW_BUILD_DOCS=OFF \
    -DGLFW_INSTALL=OFF \
    >/dev/null

echo "vendor_glfw: building" >&2
cmake --build "$build" --parallel >/dev/null

test -f "$build/src/libglfw3.a"
