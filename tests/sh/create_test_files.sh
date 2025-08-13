#!/bin/bash

# Script to create all test files for the assembler project

echo "Creating test file structure..."

# Create directories
mkdir -p tests/valid
mkdir -p tests/invalid

echo "Creating valid test files..."

# test1_simple.as - Basic functionality
cat > tests/valid/test1_simple.as << 'EOF'
; Simple test - no macros, basic assembly
MAIN: mov r1, #5
      add r2, r1
      prn r2
      stop
EOF

# test2_macros.as - Basic macro functionality  
cat > tests/valid/test2_macros.as << 'EOF'
; Test basic macro definition and usage
mcro simple_macro
mov r1, #10
add r2, r1
mcroend

MAIN: simple_macro
      prn r2
      stop
EOF

# test3_complex.as - Complex macros with labels
cat > tests/valid/test3_complex.as << 'EOF'
; Complex test with macros and labels
mcro loop_macro
LOOP: cmp r1, #0
      bne END
      dec r1
      jmp LOOP
END:  nop
mcroend

MAIN: mov r1, #5
LABEL1: loop_macro
        prn r1
        stop

DATA1: .data 10, 20, 30
STR1: .string "hello"
EOF

# test4_all_features.as - All assembler features
cat > tests/valid/test4_all_features.as << 'EOF'
; Test all language features
.extern external_symbol

mcro init_registers
mov r0, #0
mov r1, #1
mov r2, #2
mcroend

mcro print_and_increment
prn r0
inc r0
mcroend

MAIN: init_registers
LOOP: cmp r0, #10
      bne CONTINUE
      jmp END_PROG

CONTINUE: print_and_increment
          jmp LOOP

END_PROG: stop

.entry MAIN
.entry LOOP

NUMBERS: .data 1, 2, 3, 4, 5
MESSAGE: .string "Test completed"
MATRIX: .mat [2][3] 1, 2, 3, 4, 5, 6
EOF

# test5_edge_cases.as - Edge cases
cat > tests/valid/test5_edge_cases.as << 'EOF'
; Edge cases test
mcro empty_macro
mcroend

mcro single_line_macro
nop
mcroend

; Label at exact maximum length (30 chars)
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234: mov r1, #1

; Macro with underscore (should be valid)
mcro test_macro_with_underscore
mov r2, #2
mcroend

START: empty_macro
       single_line_macro
       test_macro_with_underscore
       stop
EOF

echo "Creating invalid test files..."

# error1_line_too_long.as
cat > tests/invalid/error1_line_too_long.as << 'EOF'
; This line is intentionally longer than 80 characters to test error handling for line length validation in the assembler system which should report an error
MAIN: mov r1, #5
      stop
EOF

# error2_invalid_macro.as
cat > tests/invalid/error2_invalid_macro.as << 'EOF'
; Invalid macro names and definitions
mcro 123invalid
mov r1, #5
mcroend

mcro mov
add r1, r2
mcroend

MAIN: 123invalid
      mov
      stop
EOF

# error3_label_errors.as
cat > tests/invalid/error3_label_errors.as << 'EOF'
; Label errors
123INVALID: mov r1, #5
mov: add r1, r2
DUPLICATE: mov r2, #3
DUPLICATE: add r3, r4

mcro test_macro
mov r1, #1
mcroend

test_macro: mov r2, #2

MAIN: stop
EOF

# error4_missing_mcroend.as
cat > tests/invalid/error4_missing_mcroend.as << 'EOF'
; Missing mcroend
mcro incomplete_macro
mov r1, #5
add r2, r1

MAIN: incomplete_macro
      stop
EOF

# error5_reserved_words.as
cat > tests/invalid/error5_reserved_words.as << 'EOF'
; Reserved words as macro/label names
mcro mov
add r1, r2
mcroend

mcro stop
prn r1
mcroend

MAIN: mov
      stop
      stop
EOF

# Create the test runner script
cat > tests/run_tests.sh << 'EOF'
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
run_test "tests/invalid/error4_missing_mcroend.as" "fail" "Missing mcroend"
run_test "tests/invalid/error5_reserved_words.as" "fail" "Reserved words as names"

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
EOF

# Make the test runner executable
chmod +x tests/run_tests.sh

echo "Test files created successfully!"
echo ""
echo "Directory structure:"
echo "tests/"
echo "├── valid/"
echo "│   ├── test1_simple.as"
echo "│   ├── test2_macros.as"
echo "│   ├── test3_complex.as"
echo "│   ├── test4_all_features.as"
echo "│   └── test5_edge_cases.as"
echo "├── invalid/"
echo "│   ├── error1_line_too_long.as"
echo "│   ├── error2_invalid_macro.as"
echo "│   ├── error3_label_errors.as"
echo "│   ├── error4_missing_mcroend.as"
echo "│   └── error5_reserved_words.as"
echo "└── run_tests.sh"
echo ""
echo "To run tests:"
echo "1. Compile your project: make"
echo "2. Run tests: ./tests/run_tests.sh"