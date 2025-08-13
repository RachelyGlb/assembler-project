#!/bin/bash

echo "=== Assembler Test Suite ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a single test
run_test() {
    local test_file=$1
    local expected_result=$2  # "pass" or "fail"
    local description=$3
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${YELLOW}Test $TOTAL_TESTS:${NC} $description"
    echo "File: $test_file"
    
    # Extract base name without extension
    base_name=$(basename "$test_file" .as)
    dir_name=$(dirname "$test_file")
    
    # Run the assembler
    ./assembler "$dir_name/$base_name" > test_output.log 2>&1
    result=$?
    
    if [ "$expected_result" = "pass" ]; then
        if [ $result -eq 0 ]; then
            echo -e "${GREEN}✓ PASSED${NC} - Test completed successfully"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            
            # Check if expected output files were created
            if [ -f "${test_file%.as}.am" ]; then
                echo "  ✓ .am file created"
            else
                echo -e "  ${RED}✗ .am file not created${NC}"
            fi
        else
            echo -e "${RED}✗ FAILED${NC} - Expected success but got error"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            echo "Error output:"
            cat test_output.log
        fi
    else  # expected_result = "fail"
        if [ $result -ne 0 ]; then
            echo -e "${GREEN}✓ PASSED${NC} - Test correctly identified error"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            echo "Error message:"
            cat test_output.log
        else
            echo -e "${RED}✗ FAILED${NC} - Expected error but test passed"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
    
    echo
    rm -f test_output.log
}

# Check if assembler exists
if [ ! -f "./assembler" ]; then
    echo -e "${RED}Error: assembler executable not found${NC}"
    echo "Please compile the project first using: make"
    exit 1
fi

echo "Running valid tests..."
echo "===================="

run_test "tests/valid/test1_simple.as" "pass" "Simple assembly without macros"
run_test "tests/valid/test2_macros.as" "pass" "Basic macro functionality"
run_test "tests/valid/test3_complex.as" "pass" "Complex macros with labels"
run_test "tests/valid/test4_all_features.as" "pass" "All assembler features"
run_test "tests/valid/test5_edge_cases.as" "pass" "Edge cases"

echo "Running invalid tests..."
echo "======================="

run_test "tests/invalid/error1_line_too_long.as" "fail" "Line too long error"
run_test "tests/invalid/error2_invalid_macro.as" "fail" "Invalid macro names"
run_test "tests/invalid/error3_label_errors.as" "fail" "Label errors"
run_test "tests/invalid/error5_reserved_words.as" "fail" "Reserved words as names"
run_test "tests/invalid/error6_label_with_macro.as" "fail" "Label with macro call"

# Test with no input files
echo -e "${YELLOW}Test $((TOTAL_TESTS + 1)):${NC} No input files"
./assembler > test_output.log 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ PASSED${NC} - Correctly handled no input files"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "${RED}✗ FAILED${NC} - Should not error on no input files"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
TOTAL_TESTS=$((TOTAL_TESTS + 1))
echo
rm -f test_output.log

# Summary
echo "===================="
echo "Test Summary:"
echo -e "Total tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed! 😞${NC}"
    exit 1
fi
