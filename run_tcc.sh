#!/usr/bin/env bash

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR_TCC="${PROJECT_ROOT}/build-tcc"

if [ ! -f "${BUILD_DIR_TCC}/cmds/sxs/sxs" ]; then
    echo "Error: TCC build not found. Run ./build_and_test.sh first or build manually."
    exit 1
fi

export DYLD_LIBRARY_PATH="${BUILD_DIR_TCC}/libs/buffer:${BUILD_DIR_TCC}/libs/map:${BUILD_DIR_TCC}/libs/ctx:${BUILD_DIR_TCC}/libs/slp:${BUILD_DIR_TCC}/libs/scanner:${BUILD_DIR_TCC}/libs/sxs:${DYLD_LIBRARY_PATH}"

exec "${BUILD_DIR_TCC}/cmds/sxs/sxs" "$@"

