#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SXS_BIN="$SCRIPT_DIR/../../../../build/sxs/sxs"

if [ ! -f "$SXS_BIN" ]; then
    echo "ERROR: sxs binary not found at $SXS_BIN"
    echo "Please build the project first: cd build && make"
    exit 1
fi

echo "==================================================================="
echo "Testing Kernel Lock Mechanism"
echo "==================================================================="
echo ""
echo "This test verifies that:"
echo "  1. Kernels can be loaded in datum expressions (#(load ...)) at start"
echo "  2. First non-datum expression triggers kernel lock"
echo "  3. Subsequent load attempts fail with error"
echo ""

run_test() {
    local test_file="$1"
    local test_name="$(basename "$test_file" .sxs)"
    local expected_file="${test_file%.sxs}.expected"
    
    echo "-------------------------------------------------------------------"
    echo "Test: $test_name"
    echo "-------------------------------------------------------------------"
    
    if [ ! -f "$expected_file" ]; then
        echo "ERROR: Expected file not found: $expected_file"
        return 1
    fi
    
    local output_file=$(mktemp)
    "$SXS_BIN" "$test_file" > "$output_file" 2>&1
    local exit_code=$?
    grep -v "^\[20" "$output_file" > "$output_file.filtered"
    mv "$output_file.filtered" "$output_file"
    
    echo "Exit code: $exit_code"
    echo ""
    echo "Output:"
    cat "$output_file"
    echo ""
    
    if diff -u "$expected_file" "$output_file"; then
        rm -f "$output_file"
        echo "✓ $test_name PASSED (output matches expected)"
        echo ""
        return 0
    else
        echo ""
        echo "✗ $test_name FAILED (output mismatch)"
        echo ""
        echo "Expected:"
        cat "$expected_file"
        echo ""
        echo "Got:"
        cat "$output_file"
        echo ""
        rm -f "$output_file"
        return 1
    fi
}

failed=0

echo "Test 1: Valid kernel loading (all loads before code)"
run_test "$SCRIPT_DIR/valid_lock.sxs" || failed=$((failed + 1))

echo "Test 2: Invalid kernel loading (load after code execution)"
run_test "$SCRIPT_DIR/invalid_lock.sxs" || failed=$((failed + 1))

echo "==================================================================="
if [ $failed -eq 0 ]; then
    echo "✓ All kernel lock tests passed!"
    echo ""
    echo "Summary:"
    echo "  - Kernels can be loaded in datum expressions at program start"
    echo "  - First non-datum expression triggers automatic kernel lock"
    echo "  - Subsequent kernel load attempts are correctly rejected"
    exit 0
else
    echo "✗ $failed test(s) failed"
    exit 1
fi

