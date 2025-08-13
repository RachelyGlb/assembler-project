#!/bin/bash

# Fixed Debug Helper Script for Assembler Project
echo "🔍 Assembler Debug Helper (Fixed Version)"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to check file exists
check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $1 exists"
        return 0
    else
        echo -e "${RED}✗${NC} $1 missing"
        return 1
    fi
}

# Function to check compilation
check_compilation() {
    echo -e "\n${BLUE}📋 Checking Compilation${NC}"
    echo "----------------------"
    
    if [ ! -f "./assembler" ]; then
        echo -e "${RED}✗ Assembler executable not found${NC}"
        echo "Try: make clean && make"
        return 1
    else
        echo -e "${GREEN}✓ Assembler executable found${NC}"
        return 0
    fi
}

# Function to analyze memory management (fixed)
analyze_memory() {
    echo -e "\n${BLUE}🔍 Memory Management Analysis${NC}"
    echo "------------------------------"
    
    # Count total malloc/calloc calls across all files
    local total_malloc=$(grep -c "malloc\|calloc" *.c 2>/dev/null | awk -F: '{sum+=$2} END {print sum+0}')
    local total_free=$(grep -c "free(" *.c 2>/dev/null | awk -F: '{sum+=$2} END {print sum+0}')
    
    echo "Total malloc/calloc calls: $total_malloc"
    echo "Total free calls: $total_free"
    
    if [ "$total_malloc" -gt "$total_free" ]; then
        echo -e "${YELLOW}⚠️  More allocations than frees - check for memory leaks${NC}"
    elif [ "$total_malloc" -eq "$total_free" ]; then
        echo -e "${GREEN}✓${NC} Allocation/free count balanced"
    else
        echo -e "${YELLOW}⚠️  More frees than allocations - potential double free${NC}"
    fi
    
    # Show per-file breakdown
    echo -e "\nPer-file breakdown:"
    for file in *.c; do
        if [ -f "$file" ]; then
            local malloc_count=$(grep -c "malloc\|calloc" "$file" 2>/dev/null || echo 0)
            local free_count=$(grep -c "free(" "$file" 2>/dev/null || echo 0)
            printf "  %-20s malloc: %d, free: %d\n" "$file" "$malloc_count" "$free_count"
        fi
    done
}

# Function to test basic functionality
test_basic_functionality() {
    echo -e "\n${BLUE}🧪 Basic Functionality Test${NC}"
    echo "----------------------------"
    
    if [ ! -f "./assembler" ]; then
        echo -e "${RED}✗ Assembler executable not found${NC}"
        return 1
    fi
    
    # Test 1: Simple assembly without macro
    echo "Test 1: Simple assembly (no macro)..."
    cat > debug_simple.as << 'EOF'
MAIN: mov r1, #5
      stop
EOF
    
    if ./assembler debug_simple > debug_output.log 2>&1; then
        echo -e "${GREEN}✓${NC} Simple test passed"
        if [ -f "debug_simple.am" ]; then
            echo -e "${GREEN}✓${NC} Output file created"
            echo "Content preview:"
            head -3 debug_simple.am
        fi
    else
        echo -e "${RED}✗${NC} Simple test failed"
        echo "Error output:"
        cat debug_output.log
    fi
    
    # Test 2: Macro functionality (fixed)
    echo -e "\nTest 2: Macro functionality..."
    cat > debug_macro.as << 'EOF'
mcro test_macro
mov r1, #10
mcroend

test_macro
stop
EOF
    
    if ./assembler debug_macro > debug_output.log 2>&1; then
        echo -e "${GREEN}✓${NC} Macro test passed"
        if [ -f "debug_macro.am" ]; then
            echo -e "${GREEN}✓${NC} Macro output file created"
            echo "Content preview:"
            head -5 debug_macro.am
        fi
    else
        echo -e "${RED}✗${NC} Macro test failed"
        echo "Error output:"
        cat debug_output.log
    fi
    
    # Cleanup
    rm -f debug_simple.as debug_simple.am debug_macro.as debug_macro.am debug_output.log
}

# Function to test error handling
test_error_handling() {
    echo -e "\n${BLUE}🚨 Error Handling Tests${NC}"
    echo "------------------------"
    
    # Test with non-existent file
    echo "Testing non-existent file handling..."
    ./assembler non_existent_file > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${GREEN}✓${NC} Properly handled non-existent file"
    else
        echo -e "${YELLOW}⚠️${NC} Should return error for non-existent file"
    fi
    
    # Test with empty file
    echo "Testing empty file handling..."
    touch empty_test.as
    ./assembler empty_test > /dev/null 2>&1
    exit_code=$?
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓${NC} Empty file handled gracefully"
    else
        echo -e "${YELLOW}⚠️${NC} Empty file returned error (exit code: $exit_code)"
    fi
    rm -f empty_test.as empty_test.am
}

# Main execution
main() {
    check_compilation
    analyze_memory
    test_basic_functionality
    test_error_handling
    
    echo -e "\n${GREEN}🎉 Debug analysis complete!${NC}"
    echo -e "\n${BLUE}💡 Next Steps:${NC}"
    echo "1. Fix any memory management issues shown above"
    echo "2. Test with your actual test files: ./assembler test1"
    echo "3. Run full test suite: ./run_tests.sh"
}

# Run the main function
main "$@"
