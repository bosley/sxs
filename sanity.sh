#!/bin/bash

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${BLUE}════════════════════════════════════════${NC}"
echo -e "${BLUE}  SXS Full Sanity Check${NC}"
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo

echo -e "${YELLOW}Cleaning build directory...${NC}"
rm -rf "$SCRIPT_DIR/build"
mkdir "$SCRIPT_DIR/build"
echo -e "  ${GREEN}✓${NC} Build directory cleaned"
echo

echo -e "${YELLOW}Running CMake...${NC}"
cd "$SCRIPT_DIR/build"
if cmake .. > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} CMake configuration successful"
else
    echo -e "  ${RED}✗${NC} CMake configuration failed"
    exit 1
fi
echo

echo -e "${YELLOW}Building project with make -j7...${NC}"
if make -j7; then
    echo -e "  ${GREEN}✓${NC} Project build successful"
else
    echo -e "  ${RED}✗${NC} Project build failed"
    exit 1
fi
echo

echo -e "${YELLOW}Running kernel tests...${NC}"
cd "$SCRIPT_DIR/kernels"
if ./test.sh; then
    echo
    echo -e "${GREEN}════════════════════════════════════════${NC}"
    echo -e "${GREEN}  All Sanity Checks Passed!${NC}"
    echo -e "${GREEN}════════════════════════════════════════${NC}"
    exit 0
else
    echo
    echo -e "${RED}════════════════════════════════════════${NC}"
    echo -e "${RED}  Sanity Checks Failed${NC}"
    echo -e "${RED}════════════════════════════════════════${NC}"
    exit 1
fi

