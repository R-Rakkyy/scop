#!/usr/bin/env bash
# Clones (if missing) and builds the Vulkan headers + loader into
# $1 (Vulkan-Headers) and $2 (Vulkan-Loader), producing
# $2/build/loader/libvulkan.so.
# Only invoked when `pkg-config vulkan` can't find a system copy.
#
# /!\ A real GPU driver (ICD) must still be installed on the system to actually run.

set -euo pipefail

headers_dir="$1"
loader_dir="$2"
loader_build="$loader_dir/build"

headers_repo="https://github.com/KhronosGroup/Vulkan-Headers.git"
loader_repo="https://github.com/KhronosGroup/Vulkan-Loader.git"

command -v git >/dev/null 2>&1 || { echo "vendor_vulkan: git is required to fetch Vulkan" >&2; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "vendor_vulkan: cmake is required to build the Vulkan loader" >&2; exit 1; }

if [ ! -d "$headers_dir/.git" ]; then
    echo "vendor_vulkan: cloning Vulkan-Headers into $headers_dir" >&2
    rm -rf "$headers_dir"
    git clone --depth 1 "$headers_repo" "$headers_dir"
fi

if [ ! -d "$loader_dir/.git" ]; then
    echo "vendor_vulkan: cloning Vulkan-Loader into $loader_dir" >&2
    rm -rf "$loader_dir"
    git clone --depth 1 "$loader_repo" "$loader_dir"
fi

headers_install="$headers_dir/build/install"

echo "vendor_vulkan: installing headers (header-only, into $headers_install)" >&2
cmake -S "$headers_dir" -B "$headers_dir/build" \
    -DCMAKE_INSTALL_PREFIX="$headers_install" \
    >/dev/null
cmake --build "$headers_dir/build" --target install >/dev/null

echo "vendor_vulkan: configuring loader" >&2
cmake -S "$loader_dir" -B "$loader_build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DVULKAN_HEADERS_INSTALL_DIR="$(cd "$headers_install" && pwd)" \
    -DUPDATE_DEPS=OFF \
    >/dev/null

echo "vendor_vulkan: building loader" >&2
cmake --build "$loader_build" --parallel >/dev/null

test -f "$loader_build/loader/libvulkan.so"
