#!/usr/bin/env bash
# Clones (if missing) and builds a static GLFW into $1, producing
# $1/build/src/libglfw3.a and headers under $1/include/
# Only invoked when `pkg-config glfw3` can't find a system copy.

set -euo pipefail

dir="$1"
build="$dir/build"
repo="https://github.com/glfw/glfw.git"

has_pkg() {
    command -v pkg-config >/dev/null 2>&1 &&
    pkg-config --exists "$@"
}

wayland_backend=OFF
x11_backend=OFF

if command -v wayland-scanner >/dev/null 2>&1 &&
   has_pkg wayland-client wayland-protocols xkbcommon
then
    wayland_backend=ON
fi

if has_pkg x11 xrandr xinerama xcursor xi
then
    x11_backend=ON
fi

if [[ "$wayland_backend" == OFF && "$x11_backend" == OFF ]]; then
    echo "error: no usable GLFW backend found" >&2
    echo "Install either X11 or Wayland development packages." >&2
    exit 1
fi

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
    -DGLFW_BUILD_WAYLAND=$wayland_backend \
    -DGLFW_BUILD_X11=$x11_backend \
    >/dev/null

echo "vendor_glfw: building" >&2
cmake --build "$build" --parallel >/dev/null

test -f "$build/src/libglfw3.a"
