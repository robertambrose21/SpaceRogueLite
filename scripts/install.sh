#!/bin/bash
set -e
set -u

THIRD_PARTY_DIR="third-party"
BUILD_TYPES=("Debug" "Release")
HEADER_PROJECTS=(
  "../net"
)

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
CWD="$(pwd)"
if [[ "$CWD" != "$ROOT_DIR" ]]; then
    echo "❌ Please run this script from the project root directory:"
    echo "   cd \"$ROOT_DIR\" && ./scripts/$(basename "$0") [Debug|Release]"
    exit 1
fi

# Clean install
CLEAN_CACHE=false

for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        CLEAN_CACHE=true
        break
    fi
done

if $CLEAN_CACHE; then
    echo "=============================="
    echo "Cleaning local Conan cache..."
    conan remove "*" -c
fi

# Install third party packages
for DIR in "$THIRD_PARTY_DIR"/*/; do
    [ -d "$DIR" ] || continue

    PACKAGE_NAME=$(basename "$DIR")
    echo "=============================="
    echo "Processing package: $PACKAGE_NAME"
    echo "Directory: $DIR"

    for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
        echo "--------------------------"
        echo "Build type: $BUILD_TYPE"

        pushd "$DIR" > /dev/null

        echo "Running conan install..."
        conan install . --build=missing -s build_type="$BUILD_TYPE"

        echo "Running conan create..."
        conan create . -s build_type="$BUILD_TYPE"

        # Go back to the previous directory
        popd > /dev/null
    done
done

# Install header packages
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for project in "${HEADER_PROJECTS[@]}"; do
  echo "============================"
  echo " Building $project ($BUILD_TYPE)"
  echo "============================"
  (
    cd "$ROOT_DIR/$project"
    conan create . -s build_type=$BUILD_TYPE
  )
done

# Install project
for DIR in */; do
    [ -d "$DIR" ] || continue
    [[ "$DIR" == "$THIRD_PARTY_DIR/" ]] && continue

    # Check for conanfile.py or conanfile.txt
    if [ ! -f "$DIR/conanfile.py" ] && [ ! -f "$DIR/conanfile.txt" ]; then
        echo "Skipping $DIR (no conanfile)"
        continue
    fi

    PACKAGE_NAME=$(basename "$DIR")
    echo "=============================="
    echo "Processing package: $PACKAGE_NAME (root folder)"
    echo "Directory: $DIR"

    for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
        echo "--------------------------"
        echo "Build type: $BUILD_TYPE"
        pushd "$DIR" > /dev/null

        echo "Running conan install..."
        conan install . --build=missing -s build_type="$BUILD_TYPE"

        popd > /dev/null
    done
done

echo "All packages processed."

# Install workspace-level dependencies
echo "=============================="
echo "Installing workspace dependencies..."
echo "=============================="

for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
    echo "--------------------------"
    echo "Build type: $BUILD_TYPE"

    BUILD_DIR_LOWER=$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')
    OUTPUT_FOLDER="build/$BUILD_DIR_LOWER"

    echo "Running conan install for workspace (output: $OUTPUT_FOLDER)..."
    conan install . --output-folder="$OUTPUT_FOLDER" --build=missing -s build_type="$BUILD_TYPE"
done

echo "=============================="
echo "✅ Installation complete!"
echo "=============================="
echo ""
echo "Next steps:"
echo "  1. Configure CMake:"
echo "     cmake --preset conan-debug    # or conan-release"
echo "  2. Build:"
echo "     cmake --build build/debug     # or build/release"