#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_KERNEL_DIR="$WORKSPACE_ROOT/build/kernels"

if [ -z "$SXS_HOME" ]; then
    echo "Error: SXS_HOME not set"
    exit 1
fi

SXS_BIN="$SXS_HOME/bin/sxs"
SXS_KERNEL_DIR="$SXS_HOME/lib/kernels"

if [ ! -x "$SXS_BIN" ]; then
    echo "Error: sxs binary not found at $SXS_BIN"
    exit 1
fi

echo "Using sxs: $SXS_BIN"
echo "Installing test kernels to: $SXS_KERNEL_DIR"
echo ""

for kernel_dir in "$WORKSPACE_ROOT"/kernels/*/; do
    kernel_name=$(basename "$kernel_dir")
    if [ -f "$kernel_dir/kernel.sxs" ]; then
        echo "Installing kernel: $kernel_name"
        mkdir -p "$SXS_KERNEL_DIR/$kernel_name"
        cp "$kernel_dir/kernel.sxs" "$SXS_KERNEL_DIR/$kernel_name/"
        if [ -f "$BUILD_KERNEL_DIR/libkernel_${kernel_name}.dylib" ]; then
            cp "$BUILD_KERNEL_DIR/libkernel_${kernel_name}.dylib" "$SXS_KERNEL_DIR/$kernel_name/"
        fi
    fi
done

echo ""

run_test() {
    local test_file="$1"
    local test_name="$(basename "$(dirname "$test_file")")/$(basename "$test_file")"
    local expected_file="${test_file%.sxs}.expected"
    
    echo "==================================="
    echo "Running: $test_name"
    echo "==================================="
    
    if [ -f "$expected_file" ]; then
        local output_file=$(mktemp)
        "$SXS_BIN" "$test_file" 2>&1 | grep -v "^\[20" > "$output_file"
        local exit_code=${PIPESTATUS[0]}
        
        if [ $exit_code -ne 0 ]; then
            cat "$output_file"
            rm -f "$output_file"
            echo ""
            echo "✗ $test_name FAILED (exit code: $exit_code)"
            return $exit_code
        fi
        
        if diff -u "$expected_file" "$output_file"; then
            rm -f "$output_file"
            echo ""
            echo "✓ $test_name PASSED (output matches expected)"
        else
            echo ""
            echo "✗ $test_name FAILED (output mismatch)"
            echo "Expected output in: $expected_file"
            echo "Actual output saved to: $output_file"
            return 1
        fi
    else
        "$SXS_BIN" "$test_file" 2>&1 | grep -v "^\[20"
        local exit_code=${PIPESTATUS[0]}
        
        if [ $exit_code -eq 0 ]; then
            echo ""
            echo "✓ $test_name PASSED"
        else
            echo ""
            echo "✗ $test_name FAILED (exit code: $exit_code)"
            return $exit_code
        fi
    fi
    
    echo ""
}

if [ $# -eq 0 ]; then
    failed=0
    for test_file in "$SCRIPT_DIR"/*/*.sxs; do
        if [ -f "$test_file" ]; then
            run_test "$test_file" || failed=$((failed + 1))
        fi
    done
    
    echo "==================================="
    if [ $failed -eq 0 ]; then
        echo "All tests passed ✓"
    else
        echo "$failed test(s) failed ✗"
        exit 1
    fi
else
    run_test "$1"
fi
