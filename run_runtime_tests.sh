#!/usr/bin/env bash

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <build_dir>"
    exit 1
fi

BUILD_DIR="$1"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="${PROJECT_ROOT}/tests/runtime_tests"
SXS_BIN="${BUILD_DIR}/cmds/sxs/sxs"

export DYLD_LIBRARY_PATH="${BUILD_DIR}/libs/buffer:${BUILD_DIR}/libs/slp:${BUILD_DIR}/libs/scanner:${BUILD_DIR}/libs/sxs:${DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="${BUILD_DIR}/libs/buffer:${BUILD_DIR}/libs/slp:${BUILD_DIR}/libs/scanner:${BUILD_DIR}/libs/sxs:${LD_LIBRARY_PATH}"

if [ ! -f "$SXS_BIN" ]; then
    echo "Error: sxs binary not found at $SXS_BIN"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo "Error: Test directory not found at $TEST_DIR"
    exit 1
fi

PASSED=0
FAILED=0
FAILED_TESTS=()

regex_match() {
    local pattern="$1"
    local text="$2"
    
    pattern=$(echo "$pattern" | sed 's/\\\\/\\/g')
    
    if echo "$text" | grep -qE "^${pattern}$"; then
        return 0
    else
        return 1
    fi
}

compare_output() {
    local expected_file="$1"
    local actual_output="$2"
    
    local failed=0
    local pattern_num=0
    local current_line=1
    local total_lines=$(echo "$actual_output" | wc -l | tr -d ' ')
    
    while IFS= read -r expected_pattern; do
        pattern_num=$((pattern_num + 1))
        
        if [ -z "$expected_pattern" ]; then
            continue
        fi
        
        local found=0
        
        while [ $current_line -le $total_lines ]; do
            local actual_line=$(echo "$actual_output" | sed -n "${current_line}p")
            current_line=$((current_line + 1))
            
            if regex_match "$expected_pattern" "$actual_line"; then
                found=1
                break
            fi
        done
        
        if [ $found -eq 0 ]; then
            echo "  Pattern $pattern_num not found: $expected_pattern"
            failed=1
        fi
    done < "$expected_file"
    
    return $failed
}

echo "========================================"
echo "=== Runtime Integration Tests ==="
echo "========================================"
echo "Build dir: $BUILD_DIR"
echo "Test dir:  $TEST_DIR"
echo ""

for test_file in "$TEST_DIR"/*.sxs; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    test_name=$(basename "$test_file" .sxs)
    expected_file="${TEST_DIR}/${test_name}.expected"
    
    if [ ! -f "$expected_file" ]; then
        echo "⚠️  SKIP: $test_name (no .expected file)"
        continue
    fi
    
    echo -n "Running: $test_name ... "
    
    actual_output=$("$SXS_BIN" "$test_file" 2>&1 || true)
    
    if compare_output "$expected_file" "$actual_output" > /tmp/test_diff_$$.txt 2>&1; then
        echo "✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "✗ FAIL"
        cat /tmp/test_diff_$$.txt
        echo ""
        FAILED=$((FAILED + 1))
        FAILED_TESTS+=("$test_name")
    fi
    rm -f /tmp/test_diff_$$.txt
done

echo ""
echo "========================================"
echo "=== Test Summary ==="
echo "========================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "Failed tests:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  - $test"
    done
    echo ""
    exit 1
fi

echo "All runtime tests passed! ✓"
exit 0

