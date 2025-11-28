#!/usr/bin/env bash
set -euo pipefail

# Default build type
BUILD_TYPE="Release"

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --build-type)
      BUILD_TYPE="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      echo "Usage: $0 [--build-type <Debug|Release|RelWithDebInfo|MinSizeRel>]"
      exit 1
      ;;
  esac
done

PROJECTS=(
  "../core"
  "../net"
  "../graphics"
  "../client"
  "../server"
)


ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for project in "${PROJECTS[@]}"; do
  echo "============================"
  echo " Building $project ($BUILD_TYPE)"
  echo "============================"
  (
    cd "$ROOT_DIR/$project"
    conan build . -s build_type=$BUILD_TYPE
  )
done

echo "✅ All projects built successfully ($BUILD_TYPE)"