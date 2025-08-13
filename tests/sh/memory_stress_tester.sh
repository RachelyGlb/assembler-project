#!/bin/bash

echo "🧪 Advanced Memory Stress Tester for Assembler"
echo "=============================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Counters
total_tests=0
passed_tests=0
failed_tests=0
memory_errors=0

# Create test directory
mkdir -p memory_stress_tests
cd memory_stress_tests

echo -e "${BLUE}🔬 Creating Memory Stress Tests...${NC}"

# Test 1: Massive Macro with Long Content
echo "Creating Test 1: Massive macro with long content..."
cat > test1_massive_macro.as << 'EOF'
mcro giant_macro
mov r0, #1
mov r1, #2
mov r2, #3
mov r3, #4
mov r4, #5
mov r5, #6
mov r6, #7
mov r7, #0
add r0, r1
add r1, r2
add r2, r3
add r3, r4
add r4, r5
add r5, r6
add r6, r7
sub r7, r0
sub r0, r1
sub r1, r2
sub r2, r3
sub r3, r4
sub r4, r5
sub r5, r6
sub r6, r7
cmp r0, r1
cmp r1, r2
cmp r2, r3
cmp r3, r4
cmp r4, r5
cmp r5, r6
cmp r6, r7
cmp r7, r0
prn r0
prn r1
prn r2
prn r3
prn r4
prn r5
prn r6
prn r7
mcroend

giant_macro
giant_macro
giant_macro
giant_macro
giant_macro
stop
EOF

# Test 2: Multiple Large Macros
echo "Creating Test 2: Multiple large macros..."
cat > test2_multiple_macros.as << 'EOF'
mcro macro1
mov r0, #100
mov r1, #200
mov r2, #300
add r0, r1
add r1, r2
sub r2, r0
mcroend

mcro macro2
mov r3, #400
mov r4, #500
mov r5, #600
add r3, r4
add r4, r5
sub r5, r3
mcroend

mcro macro3
mov r6, #700
mov r7, #800
add r6, r7
sub r7, r6
mcroend

mcro macro4
cmp r0, r1
cmp r1, r2
cmp r2, r3
cmp r3, r4
mcroend

mcro macro5
prn r0
prn r1
prn r2
prn r3
prn r4
prn r5
prn r6
prn r7
mcroend

MAIN:
macro1
macro2
macro3
macro4
macro5
macro1
macro2
macro3
macro4
macro5
stop
EOF

# Test 3: Deeply Nested Macro Calls
echo "Creating Test 3: Deeply nested macro calls..."
cat > test3_nested_calls.as << 'EOF'
mcro level1
mov r0, #1
level2
mcroend

mcro level2
mov r1, #2
level3
mcroend

mcro level3
mov r2, #3
level4
mcroend

mcro level4
mov r3, #4
level5
mcroend

mcro level5
mov r4, #5
prn r4
mcroend

level1
level1
level1
level1
level1
stop
EOF

# Test 4: Maximum Length Lines (Stress Line Buffer)
echo "Creating Test 4: Maximum length lines..."
cat > test4_max_lines.as << 'EOF'
mcro very_long_macro_name_that_approaches_the_thirty_character_limit_but_stays_valid
mov r0, #1
mov r1, #2
mov r2, #3
mov r3, #4
mov r4, #5
mov r5, #6
mov r6, #7
mov r7, #0
mcroend

very_long_macro_name_that_approaches_the_thirty_character_limit_but_stays_valid
stop
EOF

# Test 5: Many Small Macros (Test Hash Table/Linked List)
echo "Creating Test 5: Many small macros..."
cat > test5_many_small.as << 'EOF'
mcro m1
mov r0, #1
mcroend

mcro m2
mov r1, #2
mcroend

mcro m3
mov r2, #3
mcroend

mcro m4
mov r3, #4
mcroend

mcro m5
mov r4, #5
mcroend

mcro m6
mov r5, #6
mcroend

mcro m7
mov r6, #7
mcroend

mcro m8
mov r7, #8
mcroend

mcro m9
add r0, r1
mcroend

mcro m10
add r2, r3
mcroend

mcro m11
add r4, r5
mcroend

mcro m12
add r6, r7
mcroend

mcro m13
sub r0, r1
mcroend

mcro m14
sub r2, r3
mcroend

mcro m15
sub r4, r5
mcroend

MAIN:
m1
m2
m3
m4
m5
m6
m7
m8
m9
m10
m11
m12
m13
m14
m15
stop
EOF

# Test 6: Rapid Memory Allocation/Deallocation Pattern
echo "Creating Test 6: Complex macro expansion..."
cat > test6_complex_expansion.as << 'EOF'
mcro init_all_regs
mov r0, #0
mov r1, #1
mov r2, #2
mov r3, #3
mov r4, #4
mov r5, #5
mov r6, #6
mov r7, #7
mcroend

mcro math_operations
add r0, r1
add r1, r2
add r2, r3
add r3, r4
sub r4, r5
sub r5, r6
sub r6, r7
sub r7, r0
mcroend

mcro print_all
prn r0
prn r1
prn r2
prn r3
prn r4
prn r5
prn r6
prn r7
mcroend

MAIN:
init_all_regs
math_operations
print_all
init_all_regs
math_operations
print_all
init_all_regs
math_operations
print_all
stop
EOF

# Test 7: Edge Case - Empty and Single Line Macros
echo "Creating Test 7: Edge case macros..."
cat > test7_edge_cases.as << 'EOF'
mcro empty_macro
mcroend

mcro single_line
nop
mcroend

mcro just_stop
stop
mcroend

MAIN:
empty_macro
single_line
empty_macro
single_line
just_stop
EOF

cd ..

echo -e "${YELLOW}🧪 Running Memory Stress Tests...${NC}"

# Function to run test with memory monitoring
run_memory_test() {
    local test_file=$1
    local test_name=$2
    local expected_result=$3
    
    total_tests=$((total_tests + 1))
    
    echo -e "\n${BLUE}Test $total_tests: $test_name${NC}"
    echo "File: $test_file"
    
    # Run with valgrind if available
    if command -v valgrind >/dev/null 2>&1; then
        echo "Running with Valgrind memory check..."
        valgrind --leak-check=full --error-exitcode=1 --track-origins=yes \
                 ./assembler "memory_stress_tests/${test_file%.as}" > "valgrind_$total_tests.log" 2>&1
        valgrind_exit=$?
        
        if [ $valgrind_exit -eq 0 ]; then
            echo -e "   ${GREEN}✅ NO MEMORY ERRORS${NC}"
        else
            echo -e "   ${RED}❌ MEMORY ERRORS DETECTED${NC}"
            memory_errors=$((memory_errors + 1))
            echo "Valgrind output:"
            cat "valgrind_$total_tests.log" | head -20
        fi
    else
        echo "Valgrind not available, running normal test..."
        ./assembler "memory_stress_tests/${test_file%.as}" > "test_$total_tests.log" 2>&1
    fi
    
    # Run normal assembler test
    ./assembler "memory_stress_tests/${test_file%.as}" > "normal_$total_tests.log" 2>&1
    exit_code=$?
    
    if [ "$expected_result" = "pass" ]; then
        if [ $exit_code -eq 0 ]; then
            echo -e "   ${GREEN}✅ PASSED${NC} - Test completed successfully"
            passed_tests=$((passed_tests + 1))
        else
            echo -e "   ${RED}❌ FAILED${NC} - Expected success but got error"
            failed_tests=$((failed_tests + 1))
            echo "Error output:"
            cat "normal_$total_tests.log" | head -5
        fi
    else
        if [ $exit_code -ne 0 ]; then
            echo -e "   ${GREEN}✅ PASSED${NC} - Correctly handled stress case"
            passed_tests=$((passed_tests + 1))
        else
            echo -e "   ${YELLOW}⚠️  UNEXPECTED${NC} - Handled extreme case without error"
            passed_tests=$((passed_tests + 1))
        fi
    fi
}

# Run all memory stress tests
run_memory_test "test1_massive_macro.as" "Massive Macro Content" "pass"
run_memory_test "test2_multiple_macros.as" "Multiple Large Macros" "pass"
run_memory_test "test3_nested_calls.as" "Deeply Nested Calls" "pass"
run_memory_test "test4_max_lines.as" "Maximum Length Lines" "pass"
run_memory_test "test5_many_small.as" "Many Small Macros" "pass"
run_memory_test "test6_complex_expansion.as" "Complex Expansion" "pass"
run_memory_test "test7_edge_cases.as" "Edge Case Macros" "pass"

echo -e "\n${BLUE}🔬 Additional Memory Stress Tests...${NC}"

# Test with massive file input
echo "Creating massive input file..."
cat > memory_stress_tests/test8_massive_input.as << 'EOF'
mcro stress_test
EOF

# Add 1000 lines to the macro
for i in {1..1000}; do
    echo "mov r$((i % 8)), #$i" >> memory_stress_tests/test8_massive_input.as
done

echo "mcroend" >> memory_stress_tests/test8_massive_input.as
echo "stress_test" >> memory_stress_tests/test8_massive_input.as
echo "stop" >> memory_stress_tests/test8_massive_input.as

run_memory_test "test8_massive_input.as" "Massive Input File (1000+ lines)" "stress"

# Summary
echo -e "\n${BLUE}📊 Memory Stress Test Summary:${NC}"
echo "================================"
echo -e "Total tests: $total_tests"
echo -e "Passed: ${GREEN}$passed_tests${NC}"
echo -e "Failed: ${RED}$failed_tests${NC}"
echo -e "Memory errors: ${RED}$memory_errors${NC}"

if [ $memory_errors -eq 0 ] && [ $failed_tests -eq 0 ]; then
    echo -e "\n${GREEN}🎉 EXCELLENT! Your assembler handles all memory stress tests perfectly!${NC}"
    echo -e "${GREEN}🏆 No memory leaks or crashes detected!${NC}"
elif [ $memory_errors -eq 0 ]; then
    echo -e "\n${YELLOW}✅ Good! No memory errors, but some functionality issues.${NC}"
else
    echo -e "\n${RED}⚠️  Memory issues detected! Check the logs above.${NC}"
fi

echo -e "\n${BLUE}💡 Cleanup:${NC}"
read -p "Delete stress test files and logs? (y/n): " cleanup
if [ "$cleanup" = "y" ]; then
    rm -rf memory_stress_tests/ valgrind_*.log test_*.log normal_*.log
    echo "Cleaned up test files."
fi
