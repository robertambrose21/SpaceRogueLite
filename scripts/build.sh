#!/usr/bin/env bash
# Build helper for Conan + CMake
# Usage: ./scripts/build.sh [Debug|Release]
set -euo pipefail

# --- Configuration ---
BUILD_TYPE="${1:-Release}"             # default to Release if no argument
BUILD_DIR="build/${BUILD_TYPE,,}"      # lowercase folder (e.g. build/release)

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
CWD="$(pwd)"
if [[ "$CWD" != "$ROOT_DIR" ]]; then
    echo "❌ Please run this script from the project root directory:"
    echo "   cd \"$ROOT_DIR\" && ./scripts/$(basename "$0") [Debug|Release]"
    exit 1
fi

echo "=== Building ${BUILD_TYPE} in ${BUILD_DIR} ==="

conan install . \
    --output-folder="${BUILD_DIR}" \
    --build=missing \
    -s build_type="${BUILD_TYPE}"

cmake -B "${BUILD_DIR}" -S . \
      -DCMAKE_TOOLCHAIN_FILE="${BUILD_DIR}/conan_toolchain.cmake" \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

cmake --build "${BUILD_DIR}"