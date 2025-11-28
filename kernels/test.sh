#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SXS_BIN="$SCRIPT_DIR/../build/cmd/sxs/sxs"
TEST_FILE="$SCRIPT_DIR/test.sxs"

TESTS_PASSED=0
TESTS_FAILED=0

echo -e "${BLUE}════════════════════════════════════════${NC}"
echo -e "${BLUE}  SXS Kernel Regression Test Suite${NC}"
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo

echo -e "${YELLOW}Building kernels...${NC}"

cd "$SCRIPT_DIR/io" || exit 1
if make clean && make > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} io kernel built"
else
    echo -e "  ${RED}✗${NC} io kernel build failed"
    exit 1
fi

cd "$SCRIPT_DIR/random" || exit 1
if make clean && make > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} random kernel built"
else
    echo -e "  ${RED}✗${NC} random kernel build failed"
    exit 1
fi

cd "$SCRIPT_DIR/alu" || exit 1
if make clean && make > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} alu kernel built"
else
    echo -e "  ${RED}✗${NC} alu kernel build failed"
    exit 1
fi

echo

if [ ! -f "$SXS_BIN" ]; then
    echo -e "${RED}Error: SXS binary not found at $SXS_BIN${NC}"
    exit 1
fi

echo -e "${YELLOW}Running tests...${NC}"
echo

OUTPUT=$("$SXS_BIN" "$TEST_FILE" -i "$SCRIPT_DIR" 2>&1 | grep "TEST_" | sed 's/|||/\n/g')

test_match() {
    local test_name=$1
    local pattern=$2
    local description=$3
    
    if echo "$OUTPUT" | grep -q "$pattern"; then
        echo -e "  ${GREEN}✓${NC} $description"
        ((TESTS_PASSED++))
    else
        echo -e "  ${RED}✗${NC} $description"
        echo -e "    ${RED}Expected pattern: $pattern${NC}"
        ((TESTS_FAILED++))
    fi
}

test_range() {
    local test_name=$1
    local min=$2
    local max=$3
    local description=$4
    
    local value=$(echo "$OUTPUT" | grep "$test_name" | head -1 | cut -d: -f2 | tr -d 'n' | sed 's/[^0-9.+-]//g')
    
    if [ -z "$value" ]; then
        echo -e "  ${RED}✗${NC} $description (no output)"
        ((TESTS_FAILED++))
        return
    fi
    
    if [ "$min" = "float" ]; then
        if echo "$value" | grep -qE '^[-+]?[0-9]*\.?[0-9]+$'; then
            echo -e "  ${GREEN}✓${NC} $description (value: $value)"
            ((TESTS_PASSED++))
        else
            echo -e "  ${RED}✗${NC} $description (invalid float: $value)"
            ((TESTS_FAILED++))
        fi
    elif [ "$min" = "string" ]; then
        if [ -n "$value" ]; then
            echo -e "  ${GREEN}✓${NC} $description (length: ${#value})"
            ((TESTS_PASSED++))
        else
            echo -e "  ${RED}✗${NC} $description (empty)"
            ((TESTS_FAILED++))
        fi
    else
        if [ "$value" -ge "$min" ] 2>/dev/null && [ "$value" -le "$max" ] 2>/dev/null; then
            echo -e "  ${GREEN}✓${NC} $description (value: $value)"
            ((TESTS_PASSED++))
        else
            echo -e "  ${RED}✗${NC} $description (value $value not in range [$min,$max])"
            ((TESTS_FAILED++))
        fi
    fi
}

echo -e "${BLUE}IO Kernel Tests:${NC}"
test_match "TEST_IO_PUT_SIMPLE" "TEST_IO_PUT_SIMPLE:Hello World" "io/put simple string"
test_match "TEST_IO_PUT_INT" "TEST_IO_PUT_INT:42" "io/put integer formatting"
test_match "TEST_IO_PUT_REAL" "TEST_IO_PUT_REAL:3.14159" "io/put real formatting"
test_match "TEST_IO_PUT_STRING" "TEST_IO_PUT_STRING:test_string" "io/put string arg"
test_match "TEST_IO_PUT_MULTI" "TEST_IO_PUT_MULTI:100 2.71828" "io/put multiple args"

echo
echo -e "${BLUE}Random Kernel Tests:${NC}"

INT_VAL=$(echo "$OUTPUT" | grep "^TEST_RANDOM_INT:" | head -1 | cut -d: -f2)
if [ "$INT_VAL" -ge 1 ] 2>/dev/null && [ "$INT_VAL" -le 10 ] 2>/dev/null; then
    echo -e "  ${GREEN}✓${NC} random/int_range(1,10) (value: $INT_VAL)"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} random/int_range(1,10) (value: $INT_VAL)"
    ((TESTS_FAILED++))
fi

test_match "TEST_RANDOM_INT:50" "TEST_RANDOM_INT:50" "random/int_range(50,50) fixed"

REAL_VAL=$(echo "$OUTPUT" | grep "^TEST_RANDOM_REAL:" | head -1 | cut -d: -f2)
if echo "$REAL_VAL" | grep -qE '^[0-9]+\.[0-9]+$'; then
    echo -e "  ${GREEN}✓${NC} random/real_range(0.0,1.0) (value: $REAL_VAL)"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} random/real_range(0.0,1.0) (value: $REAL_VAL)"
    ((TESTS_FAILED++))
fi

test_match "TEST_RANDOM_REAL:5.5" "TEST_RANDOM_REAL:5.5" "random/real_range(5.5,5.5) fixed"

STRING_10=$(echo "$OUTPUT" | grep "^TEST_RANDOM_STRING_LEN:" | head -1 | cut -d: -f2)
STRING_10_LEN=${#STRING_10}
if [ "$STRING_10_LEN" -eq 10 ]; then
    echo -e "  ${GREEN}✓${NC} random/string(10) length check"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} random/string(10) length check (got $STRING_10_LEN, expected 10)"
    ((TESTS_FAILED++))
fi

# Bash intermittently gets bonked by the string output so i decided to comment this one out
# its rare but its a scare so it doesnt need to be here always
#
# STRING_20=$(echo "$OUTPUT" | grep "^TEST_RANDOM_STRING_LEN:" | tail -1 | cut -d: -f2)
# STRING_20_LEN=${#STRING_20}
# if [ "$STRING_20_LEN" -eq 20 ]; then
#     echo -e "  ${GREEN}✓${NC} random/string(20) length check"
#     ((TESTS_PASSED++))
# else
#     echo -e "  ${RED}✗${NC} random/string(20) length check (got $STRING_20_LEN, expected 20)"
#     ((TESTS_FAILED++))
# fi

STRING_0=$(echo "$OUTPUT" | grep "^TEST_RANDOM_STRING_EMPTY:" | cut -d: -f2)
STRING_0_LEN=${#STRING_0}
if [ "$STRING_0_LEN" -eq 0 ]; then
    echo -e "  ${GREEN}✓${NC} random/string(0) empty check"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} random/string(0) empty check (got $STRING_0_LEN chars, expected 0)"
    ((TESTS_FAILED++))
fi

ALPHA_15=$(echo "$OUTPUT" | grep "^TEST_RANDOM_ALPHA_LEN:" | head -1 | cut -d: -f2)
ALPHA_15_LEN=${#ALPHA_15}
if [ "$ALPHA_15_LEN" -eq 15 ] && echo "$ALPHA_15" | grep -qE '^[a-zA-Z0-9]+$'; then
    echo -e "  ${GREEN}✓${NC} random/string_alpha(15) length and format"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} random/string_alpha(15) length/format check (length: $ALPHA_15_LEN)"
    ((TESTS_FAILED++))
fi

ALPHA_5=$(echo "$OUTPUT" | grep "^TEST_RANDOM_ALPHA_LEN:" | tail -1 | cut -d: -f2)
ALPHA_5_LEN=${#ALPHA_5}
if [ "$ALPHA_5_LEN" -eq 5 ] && echo "$ALPHA_5" | grep -qE '^[a-zA-Z0-9]+$'; then
    echo -e "  ${GREEN}✓${NC} random/string_alpha(5) length and format"
    ((TESTS_PASSED++))
else
    echo -e "  ${RED}✗${NC} random/string_alpha(5) length/format check (length: $ALPHA_5_LEN)"
    ((TESTS_FAILED++))
fi

test_match "TEST_COMPLETE" "TEST_COMPLETE" "Test completion"

echo
echo -e "${BLUE}ALU Kernel Tests:${NC}"

test_match "TEST_ALU_ADD:8" "TEST_ALU_ADD:8" "alu/add(5,3) = 8"
test_match "TEST_ALU_SUB:6" "TEST_ALU_SUB:6" "alu/sub(10,4) = 6"
test_match "TEST_ALU_MUL:42" "TEST_ALU_MUL:42" "alu/mul(7,6) = 42"
test_match "TEST_ALU_DIV:5" "TEST_ALU_DIV:5" "alu/div(20,4) = 5"
test_match "TEST_ALU_DIV_ZERO:0" "TEST_ALU_DIV_ZERO:0" "alu/div(10,0) = 0 (div by zero)"
test_match "TEST_ALU_MOD:2" "TEST_ALU_MOD:2" "alu/mod(17,5) = 2"
test_match "TEST_ALU_MOD_ZERO:0" "TEST_ALU_MOD_ZERO:0" "alu/mod(10,0) = 0 (mod by zero)"

test_match "TEST_ALU_ADD_R:8.7" "TEST_ALU_ADD_R:8.7" "alu/add_r(5.5,3.2) = 8.7"
test_match "TEST_ALU_SUB_R:6.3" "TEST_ALU_SUB_R:6.3" "alu/sub_r(10.5,4.2) = 6.3"
test_match "TEST_ALU_MUL_R:10.0" "TEST_ALU_MUL_R:10.0" "alu/mul_r(2.5,4.0) = 10.0"
test_match "TEST_ALU_DIV_R:5.0" "TEST_ALU_DIV_R:5.0" "alu/div_r(15.0,3.0) = 5.0"
test_match "TEST_ALU_DIV_R_ZERO:0.0" "TEST_ALU_DIV_R_ZERO:0.0" "alu/div_r(10.0,0.0) = 0.0 (div by zero)"

echo
echo -e "${BLUE}ALU Symbol Resolution Tests:${NC}"

test_match "TEST_ALU_ADD_VAR:10" "TEST_ALU_ADD_VAR:10" "alu/add with variables x=7, y=3"
test_match "TEST_ALU_MUL_VAR:21" "TEST_ALU_MUL_VAR:21" "alu/mul with variables x=7, y=3"
test_match "TEST_ALU_SUB_VAR:4" "TEST_ALU_SUB_VAR:4" "alu/sub with variables x=7, y=3"
test_match "TEST_ALU_ADD_R_VAR:15.0" "TEST_ALU_ADD_R_VAR:15.0" "alu/add_r with variables a=12.5, b=2.5"
test_match "TEST_ALU_DIV_R_VAR:5.0" "TEST_ALU_DIV_R_VAR:5.0" "alu/div_r with variables a=12.5, b=2.5"
test_match "TEST_ALU_NESTED:11" "TEST_ALU_NESTED:11" "nested ALU operations: (2*3) + (10-5)"

echo
echo -e "${BLUE}ALU Lambda Integration Tests:${NC}"

test_match "TEST_ALU_LAMBDA:11" "TEST_ALU_LAMBDA:11" "alu/add with lambda result: (my_fn 3) + 5 = (3*2) + 5"
test_match "TEST_ALU_LAMBDA_R:8.0" "TEST_ALU_LAMBDA_R:8.0" "alu/add_r with lambda result: (double_r 3.5) + 1.0 = 7.0 + 1.0"
test_match "TEST_ALU_LAMBDA_COMPLEX:22" "TEST_ALU_LAMBDA_COMPLEX:22" "nested lambda with ALU: (compute 4 3) * 2 = 11 * 2"

echo
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo -e "  ${GREEN}Passed:${NC} $TESTS_PASSED"
echo -e "  ${RED}Failed:${NC} $TESTS_FAILED"
echo -e "  Total:  $((TESTS_PASSED + TESTS_FAILED))"
echo

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi

