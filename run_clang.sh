#!/usr/bin/env bash

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR_CLANG="${PROJECT_ROOT}/build-clang"

if [ ! -f "${BUILD_DIR_CLANG}/cmds/sxs/sxs" ]; then
    echo "Error: Clang build not found. Run ./build_and_test.sh first or build manually."
    exit 1
fi

exec "${BUILD_DIR_CLANG}/cmds/sxs/sxs" "$@"

