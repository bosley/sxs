#!/bin/bash

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  SXS Integration Tests${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${YELLOW}Running Kernel Integration Tests...${NC}"
echo ""
if cd "$SCRIPT_DIR/kernel" && ./run.sh; then
    echo ""
    echo -e "${GREEN}✓ Kernel tests passed${NC}"
    echo ""
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo ""
    echo -e "${RED}✗ Kernel tests failed${NC}"
    echo ""
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    FAILED_TESTS=$((FAILED_TESTS + 1))
    exit 1
fi

echo -e "${YELLOW}Running Standalone Integration Tests...${NC}"
echo ""

if [ -d "$SCRIPT_DIR/standalone" ]; then
    for test_dir in "$SCRIPT_DIR/standalone"/*; do
        if [ -d "$test_dir" ] && [ -f "$test_dir/run.sh" ]; then
            test_name=$(basename "$test_dir")
            echo -e "${BLUE}─────────────────────────────────────────────────────────────${NC}"
            echo -e "${YELLOW}Running: $test_name${NC}"
            echo ""
            
            TOTAL_TESTS=$((TOTAL_TESTS + 1))
            
            if cd "$test_dir" && ./run.sh; then
                echo ""
                echo -e "${GREEN}✓ $test_name passed${NC}"
                echo ""
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                echo ""
                echo -e "${RED}✗ $test_name failed${NC}"
                echo ""
                FAILED_TESTS=$((FAILED_TESTS + 1))
                exit 1
            fi
        fi
    done
else
    echo -e "${YELLOW}No standalone tests found${NC}"
    echo ""
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "  Total tests:  $TOTAL_TESTS"
echo -e "  ${GREEN}Passed:       $PASSED_TESTS${NC}"
echo -e "  ${RED}Failed:       $FAILED_TESTS${NC}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✓ All integration tests passed!${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    echo ""
    exit 1
fi

