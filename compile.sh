#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build"
BUILD_TYPE="Release"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --help|-h)
            echo "Usage: ./compile.sh [--release|--debug]"
            echo
            echo "  --release   Build Release configuration (default)"
            echo "  --debug     Build Debug configuration"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            echo "Usage: ./compile.sh [--release|--debug]" >&2
            exit 2
            ;;
    esac
done

echo "Configuring FuncCraft (${BUILD_TYPE})..."
cmake -S "${ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_LIBRARY=ON \
    -DBUILD_PYTHON=ON \
    -DBUILD_TEST=ON \
    -DBUILD_EXAMPLES=ON

echo "Building FuncCraft (${BUILD_TYPE})..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel
