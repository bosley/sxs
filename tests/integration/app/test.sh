#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
SXS_RT_BINARY="$WORKSPACE_ROOT/build/cmd/sxs-rt/sxs-rt"

PASSED=0
FAILED=0

log_pass() {
    echo -e "${GREEN}✓${NC} $1"
    PASSED=$((PASSED + 1))
}

log_fail() {
    echo -e "${RED}✗${NC} $1"
    FAILED=$((FAILED + 1))
}

log_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

cleanup_temp() {
    if [ -n "$TEST_DIR" ] && [ -d "$TEST_DIR" ]; then
        log_info "Cleaning up test directory: $TEST_DIR"
        rm -rf "$TEST_DIR"
    fi
}

trap cleanup_temp EXIT

echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}           SXS-RT Integration Test Suite${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo

log_info "Verifying expect is installed..."
if ! command -v expect &> /dev/null; then
    log_fail "expect is not installed. Please install it first."
    echo "  macOS: brew install expect"
    echo "  Linux: apt-get install expect or yum install expect"
    exit 1
fi
log_pass "expect is installed"

if [ -n "$SXS_HOME" ]; then
    log_pass "SXS_HOME is set: $SXS_HOME"
else
    log_fail "SXS_HOME is not set - required system kernels won't be available"
    echo "  Please set SXS_HOME environment variable"
    echo "  Example: export SXS_HOME=\$HOME/.sxs"
    exit 1
fi

log_info "Locating sxs-rt binary..."
if [ ! -f "$SXS_RT_BINARY" ]; then
    log_fail "sxs-rt binary not found at: $SXS_RT_BINARY"
    echo "  Please build the project first: cd build && make"
    exit 1
fi
log_pass "Found sxs-rt binary: $SXS_RT_BINARY"

TEST_DIR=$(mktemp -d /tmp/sxs-test-XXXXXX)
log_info "Created test directory: $TEST_DIR"
echo

echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
echo -e "${BLUE} Test 1: Creating New Project${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"

expect << EOF
set timeout 30
log_user 1

spawn $SXS_RT_BINARY new test_project $TEST_DIR

expect {
    "Successfully created project: test_project" {
        send_user "\n"
    }
    timeout {
        send_user "TIMEOUT: new command did not complete\n"
        exit 1
    }
    eof
}

wait
set wait_result [lindex \$expect_out(buffer) 0]
EOF

if [ $? -eq 0 ]; then
    log_pass "Project creation command completed"
else
    log_fail "Project creation command failed"
fi

PROJECT_DIR="$TEST_DIR/test_project"

if [ -f "$PROJECT_DIR/init.sxs" ]; then
    log_pass "init.sxs exists"
else
    log_fail "init.sxs does not exist"
fi

if [ -d "$PROJECT_DIR/kernels/test_project" ]; then
    log_pass "kernels/test_project directory exists"
else
    log_fail "kernels/test_project directory does not exist"
fi

if [ -d "$PROJECT_DIR/modules/hello_world" ]; then
    log_pass "modules/hello_world directory exists"
else
    log_fail "modules/hello_world directory does not exist"
fi

echo

echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
echo -e "${BLUE} Test 2: Dependency Information (Pre-Build)${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"

DEPS_OUTPUT=$(SXS_RT_BINARY="$SXS_RT_BINARY" PROJECT_DIR="$PROJECT_DIR" expect << 'EOF'
set timeout 10
log_user 1

spawn sh -c "exec $env(SXS_RT_BINARY) deps $env(PROJECT_DIR) 2>&1"

expect {
    eof
}

wait
EOF
)

if echo "$DEPS_OUTPUT" | grep -q "Project Dependencies Information"; then
    log_pass "deps command shows project information header"
else
    log_fail "deps command missing project information header"
fi

if echo "$DEPS_OUTPUT" | grep -q "Project Kernels"; then
    log_pass "deps command shows project kernels section"
else
    log_fail "deps command missing project kernels section"
fi

if echo "$DEPS_OUTPUT" | grep -q "test_project"; then
    log_pass "deps command lists test_project kernel"
else
    log_fail "deps command missing test_project kernel"
fi

if echo "$DEPS_OUTPUT" | grep -q "Modules"; then
    log_pass "deps command shows modules section"
else
    log_fail "deps command missing modules section"
fi

if echo "$DEPS_OUTPUT" | grep -q "Cache Status"; then
    log_pass "deps command shows cache status section"
else
    log_fail "deps command missing cache status section"
fi

echo

echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
echo -e "${BLUE} Test 3: Running Project (Builds Kernel)${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"

RUN_OUTPUT=$(SXS_RT_BINARY="$SXS_RT_BINARY" PROJECT_DIR="$PROJECT_DIR" expect << 'EOF'
set timeout 60
log_user 1

spawn sh -c "exec $env(SXS_RT_BINARY) run $env(PROJECT_DIR) 2>&1"

expect {
    eof
}

wait
EOF
)

if echo "$RUN_OUTPUT" | grep -q "Processing Project Kernels"; then
    log_pass "run command processes project kernels"
else
    log_fail "run command did not process project kernels"
fi

if echo "$RUN_OUTPUT" | grep -q "Building kernel.*test_project"; then
    log_pass "run command builds test_project kernel"
else
    log_fail "run command did not build test_project kernel"
fi

if echo "$RUN_OUTPUT" | grep -q "Running Project"; then
    log_pass "run command executes project"
else
    log_fail "run command did not execute project"
fi

if echo "$RUN_OUTPUT" | grep -q "Starting test_project"; then
    log_pass "init.sxs executed with expected debug output"
else
    log_fail "init.sxs did not produce expected debug output"
fi

echo

echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
echo -e "${BLUE} Test 4: Dependency Information (Post-Build)${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"

DEPS_POST_OUTPUT=$(SXS_RT_BINARY="$SXS_RT_BINARY" PROJECT_DIR="$PROJECT_DIR" expect << 'EOF'
set timeout 10
log_user 1

spawn sh -c "exec $env(SXS_RT_BINARY) deps $env(PROJECT_DIR) 2>&1"

expect {
    eof
}

wait
EOF
)

if echo "$DEPS_POST_OUTPUT" | grep -q "✓ Yes"; then
    log_pass "deps command shows kernel is now cached"
else
    log_fail "deps command does not show kernel as cached"
fi

CACHE_KERNEL_DIR="$PROJECT_DIR/.sxs-cache/kernels/test_project"
if [ -d "$CACHE_KERNEL_DIR" ]; then
    log_pass "cache kernel directory exists"
    
    if ls "$CACHE_KERNEL_DIR"/libkernel_test_project.* 1> /dev/null 2>&1; then
        log_pass "built kernel library exists in cache"
    else
        log_fail "built kernel library not found in cache"
    fi
else
    log_fail "cache kernel directory does not exist"
fi

echo

echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
echo -e "${BLUE} Test 5: Clean Cache${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"

CLEAN_OUTPUT=$(SXS_RT_BINARY="$SXS_RT_BINARY" PROJECT_DIR="$PROJECT_DIR" expect << 'EOF'
set timeout 10
log_user 1

spawn sh -c "exec $env(SXS_RT_BINARY) clean $env(PROJECT_DIR) 2>&1"

expect {
    eof
}

wait
EOF
)

if echo "$CLEAN_OUTPUT" | grep -q "Cleaned cache from project"; then
    log_pass "clean command executed successfully"
else
    log_fail "clean command did not execute successfully"
fi

if echo "$CLEAN_OUTPUT" | grep -q "Removed.*items"; then
    log_pass "clean command reported removed items"
else
    log_fail "clean command did not report removed items"
fi

if [ ! -d "$PROJECT_DIR/.sxs-cache" ]; then
    log_pass ".sxs-cache directory was removed"
else
    log_fail ".sxs-cache directory still exists"
fi

echo

echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
echo -e "${BLUE} Test 6: Clean Cache (Already Clean)${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"

CLEAN_AGAIN_OUTPUT=$(SXS_RT_BINARY="$SXS_RT_BINARY" PROJECT_DIR="$PROJECT_DIR" expect << 'EOF'
set timeout 10
log_user 1

spawn sh -c "exec $env(SXS_RT_BINARY) clean $env(PROJECT_DIR) 2>&1"

expect {
    eof
}

wait
EOF
)

if echo "$CLEAN_AGAIN_OUTPUT" | grep -q "No cache to clean"; then
    log_pass "clean command reports no cache to clean"
else
    log_fail "clean command did not report 'no cache to clean'"
fi

echo
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}           Test Summary${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Passed:${NC} $PASSED"
echo -e "${RED}Failed:${NC} $FAILED"
echo

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi

