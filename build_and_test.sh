#!/usr/bin/env bash

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR_TCC="${PROJECT_ROOT}/build-tcc"
BUILD_DIR_CLANG="${PROJECT_ROOT}/build-clang"

NCPUS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "========================================"
echo "=== Building with TCC (no sanitizers) ==="
echo "========================================"

if [ -d "$BUILD_DIR_TCC" ]; then
    rm -rf "$BUILD_DIR_TCC"
fi
mkdir -p "$BUILD_DIR_TCC"

cd "$BUILD_DIR_TCC"
cmake -DUSE_TCC=ON -DEXEC_TESTS=OFF ..
make -j${NCPUS}

echo ""
echo "=== Running TCC tests ==="
export DYLD_LIBRARY_PATH="${BUILD_DIR_TCC}/libs/buffer:${BUILD_DIR_TCC}/libs/slp:${BUILD_DIR_TCC}/libs/scanner:${BUILD_DIR_TCC}/libs/sxs:${DYLD_LIBRARY_PATH}"
cmake -DUSE_TCC=ON -DEXEC_TESTS=ON ..
ctest --output-on-failure

echo ""
echo "=== Running TCC runtime integration tests ==="
cd "$PROJECT_ROOT"
./run_runtime_tests.sh "$BUILD_DIR_TCC"

echo ""
echo "========================================"
echo "=== Building with Clang (with ASan) ==="
echo "========================================"

if [ -d "$BUILD_DIR_CLANG" ]; then
    rm -rf "$BUILD_DIR_CLANG"
fi
mkdir -p "$BUILD_DIR_CLANG"

cd "$BUILD_DIR_CLANG"
cmake -DUSE_TCC=OFF -DENABLE_ASAN=ON -DEXEC_TESTS=ON ..
make -j${NCPUS}

echo ""
echo "=== Running Clang tests with leak detection ==="
ctest --output-on-failure

echo ""
echo "=== Running Clang runtime integration tests (with ASan) ==="
cd "$PROJECT_ROOT"
./run_runtime_tests.sh "$BUILD_DIR_CLANG"

echo ""
echo "========================================"
echo "=== All tests passed! ==="
echo "=== TCC: Build + Unit Tests + Runtime Tests ✓"
echo "=== Clang: Build + Unit Tests + Runtime Tests + Leak Detection ✓"
echo "========================================"

