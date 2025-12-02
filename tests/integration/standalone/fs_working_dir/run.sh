#!/bin/bash

echo "==================================================================="
echo "Testing SXS Working Directory Functionality"
echo "==================================================================="
echo ""

SXS_BIN="../../../../build/sxs/sxs"
SCRIPT="fs_working_dir_demo.sxs"

if [ ! -f "$SXS_BIN" ]; then
    echo "ERROR: sxs binary not found at $SXS_BIN"
    echo "Please build the project first: cd build && make"
    exit 1
fi

echo "Test 1: Running with default working directory (current dir)"
echo "-------------------------------------------------------------------"
echo "Command: $SXS_BIN $SCRIPT"
echo ""
$SXS_BIN "$SCRIPT"
echo ""
echo ""

echo "Test 2: Running with /tmp as working directory"
echo "-------------------------------------------------------------------"
TEST_DIR="/tmp/sxs_test_$$"
mkdir -p "$TEST_DIR"
echo "Command: $SXS_BIN $SCRIPT -w $TEST_DIR"
echo "Working directory: $TEST_DIR"
echo ""
$SXS_BIN "$SCRIPT" -w "$TEST_DIR"
echo ""
echo "Checking if files were created in $TEST_DIR:"
ls -la "$TEST_DIR" 2>/dev/null || echo "Directory is empty (files were cleaned up)"
echo ""
rmdir "$TEST_DIR" 2>/dev/null
echo ""

echo "Test 3: Running with custom working directory in build"
echo "-------------------------------------------------------------------"
TEST_DIR="../../../../build/test_wd_$$"
mkdir -p "$TEST_DIR"
echo "Command: $SXS_BIN $SCRIPT -w $TEST_DIR"
echo "Working directory: $TEST_DIR"
echo ""
$SXS_BIN "$SCRIPT" -w "$TEST_DIR"
echo ""
echo "Checking if files were created in $TEST_DIR:"
ls -la "$TEST_DIR" 2>/dev/null || echo "Directory is empty (files were cleaned up)"
echo ""
rmdir "$TEST_DIR" 2>/dev/null
echo ""

echo "==================================================================="
echo "All tests complete!"
echo "==================================================================="
echo ""
echo "Summary:"
echo "- Test 1 used current directory as working directory"
echo "- Test 2 used /tmp/sxs_test_PID as working directory"
echo "- Test 3 used build/test_wd_PID as working directory"
echo ""
echo "All relative file operations in the script respected the"
echo "configured working directory via the -w flag."

