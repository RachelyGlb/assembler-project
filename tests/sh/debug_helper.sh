#!/bin/bash

# Debug Helper Script for Assembler Project
# This script helps identify and debug common issues

echo "🔍 Assembler Debug Helper"
echo "========================"

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
    
    # Check if all source files exist
    local source_files=("assembler.c" "preassembler.c" "file_utils.c" "line_parser.c" "macro_and_label_func.c" "word_extractor.c")
    local header_files=("assembler.h" "preassembler.h")
    local missing_files=0
    
    echo "Source files:"
    for file in "${source_files[@]}"; do
        if ! check_file "$file"; then
            missing_files=$((missing_files + 1))
        fi
    done
    
    echo -e "\nHeader files:"
    for file in "${header_files[@]}"; do
        if ! check_file "$file"; then
            missing_files=$((missing_files + 1))
        fi
    done
    
    if [ $missing_files -gt 0 ]; then
        echo -e "${RED}❌ Missing $missing_files files - cannot compile${NC}"
        return 1
    fi
    
    # Try compilation
    echo -e "\n${YELLOW}🔨 Attempting compilation...${NC}"
    if make clean > /dev/null 2>&1 && make > compile.log 2>&1; then
        echo -e "${GREEN}✓ Compilation successful${NC}"
        return 0
    else
        echo -e "${RED}✗ Compilation failed${NC}"
        echo "Compilation errors:"
        cat compile.log
        rm -f compile.log
        return 1
    fi
}

# Function to analyze common code issues
analyze_code() {
    echo -e "\n${BLUE}🔍 Code Analysis${NC}"
    echo "----------------"
    
    # Check for common C90 compliance issues
    echo "Checking C90 compliance..."
    
    # Check for // comments (should use /* */)
    if grep -n "//" *.c *.h 2>/dev/null; then
        echo -e "${YELLOW}⚠️  Found C++ style comments - use /* */ instead${NC}"
    else
        echo -e "${GREEN}✓${NC} No C++ style comments found"
    fi
    
    # Check for variable declarations in middle of function
    echo -e "\nChecking variable declarations..."
    if grep -n "int.*=" *.c 2>/dev/null | grep -v "^.*{" | head -5; then
        echo -e "${YELLOW}⚠️  Check variable declarations - should be at start of block in C90${NC}"
    fi
    
    # Check for missing includes
    echo -e "\nChecking includes..."
    local has_stdio=false
    local has_stdlib=false
    local has_string=false
    
    if grep -q "#include <stdio.h>" *.c *.h 2>/dev/null; then
        echo -e "${GREEN}✓${NC} stdio.h included"
        has_stdio=true
    fi
    
    if grep -q "#include <stdlib.h>" *.c *.h 2>/dev/null; then
        echo -e "${GREEN}✓${NC} stdlib.h included"
        has_stdlib=true
    fi
    
    if grep -q "#include <string.h>" *.c *.h 2>/dev/null; then
        echo -e "${GREEN}✓${NC} string.h included"
        has_string=true
    fi
    
    # Check for potential memory leaks
    echo -e "\nChecking memory management..."
    local malloc_count=$(grep -c "malloc\|calloc" *.c 2>/dev/null || echo 0)
    local free_count=$(grep -c "free(" *.c 2>/dev/null || echo 0)
    
    echo "malloc/calloc calls: $malloc_count"
    echo "free calls: $free_count"
    
    if [ $malloc_count -gt $free_count ]; then
        echo -e "${YELLOW}⚠️  More allocations than frees - check for memory leaks${NC}"
    elif [ $malloc_count -eq $free_count ]; then
        echo -e "${GREEN}✓${NC} Allocation/free count balanced"
    fi
}

# Function to test specific functionality
test_functionality() {
    echo -e "\n${BLUE}🧪 Functionality Testing${NC}"
    echo "------------------------"
    
    if [ ! -f "./assembler" ]; then
        echo -e "${RED}✗ Assembler executable not found${NC}"
        return 1
    fi
    
    # Test basic functionality
    echo "Testing basic functionality..."
    
    # Create a simple test file
    cat > debug_test.as << 'EOF'
; Simple test
MAIN: mov r1, #5
      stop
EOF
    
    echo "Created debug_test.as"
    
    # Test assembler
    if ./assembler debug_test > debug_output.log 2>&1; then
        echo -e "${GREEN}✓${NC} Basic assembly test passed"
        
        if [ -f "debug_test.am" ]; then
            echo -e "${GREEN}✓${NC} Output file created"
            echo "Output content:"
            cat debug_test.am
        else
            echo -e "${YELLOW}⚠️${NC}  No .am file created"
        fi
    else
        echo -e "${RED}✗${NC} Basic assembly test failed"
        echo "Error output:"
        cat debug_output.log
    fi
    
    # Test macro functionality
    echo -e "\nTesting macro functionality..."
    cat > debug_macro.as << 'EOF'
mcro test_macro
mov r1, #10
mcroend

MAIN: test_macro
      stop
EOF
    
    if ./assembler debug_macro > debug_output.log 2>&1; then
        echo -e "${GREEN}✓${NC} Macro test passed"
        
        if [ -f "debug_macro.am" ]; then
            echo -e "${GREEN}✓${NC} Macro output file created"
            echo "Macro expansion result:"
            cat debug_macro.am
        fi
    else
        echo -e "${RED}✗${NC} Macro test failed"
        echo "Error output:"
        cat debug_output.log
    fi
    
    # Cleanup
    rm -f debug_test.as debug_test.am debug_macro.as debug_macro.am debug_output.log
}

# Function to check for common runtime issues
check_runtime_issues() {
    echo -e "\n${BLUE}🐛 Runtime Issue Detection${NC}"
    echo "---------------------------"
    
    if [ ! -f "./assembler" ]; then
        echo -e "${RED}✗ Cannot check runtime - assembler not built${NC}"
        return 1
    fi
    
    # Test with no arguments
    echo "Testing with no arguments..."
    ./assembler > /tmp/no_args.log 2>&1
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓${NC} Handled no arguments gracefully"
    else
        echo -e "${YELLOW}⚠️${NC}  No arguments test returned error code $exit_code"
    fi
    
    # Test with non-existent file
    echo "Testing with non-existent file..."
    ./assembler non_existent_file > /tmp/no_file.log 2>&1
    exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        echo -e "${GREEN}✓${NC} Properly handled non-existent file"
    else
        echo -e "${YELLOW}⚠️${NC}  Should return error for non-existent file"
    fi
    
    # Test with empty file
    echo "Testing with empty file..."
    touch empty_test.as
    ./assembler empty_test > /tmp/empty.log 2>&1
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓${NC} Handled empty file correctly"
    else
        echo -e "${YELLOW}⚠️${NC}  Empty file handling returned error"
    fi
    
    rm -f empty_test.as empty_test.am
    rm -f /tmp/no_args.log /tmp/no_file.log /tmp/empty.log
}

# Function to suggest fixes for common issues
suggest_fixes() {
    echo -e "\n${BLUE}💡 Common Issue Fixes${NC}"
    echo "---------------------"
    
    echo "1. Compilation Issues:"
    echo "   - Use /* */ comments instead of //"
    echo "   - Declare all variables at start of function/block"
    echo "   - Include proper headers: stdio.h, stdlib.h, string.h"
    echo "   - Use -ansi -pedantic -Wall flags"
    
    echo -e "\n2. Memory Issues:"
    echo "   - Every malloc() should have corresponding free()"
    echo "   - Check return values of malloc()"
    echo "   - Set pointers to NULL after freeing"
    echo "   - Use valgrind: make memcheck"
    
    echo -e "\n3. File Handling:"
    echo "   - Always check if fopen() returns NULL"
    echo "   - Close files with fclose()"
    echo "   - Handle file permission errors"
    
    echo -e "\n4. String Handling:"
    echo "   - Always null-terminate strings"
    echo "   - Check string lengths before copying"
    echo "   - Use strncpy() instead of strcpy() for safety"
    
    echo -e "\n5. Error Handling:"
    echo "   - Return meaningful exit codes"
    echo "   - Print clear error messages"
    echo "   - Clean up resources before exiting"
}

# Main execution
main() {
    local run_all=true
    
    # Parse command line arguments
    case "${1:-all}" in
        "compile")
            check_compilation
            ;;
        "analyze")
            analyze_code
            ;;
        "test")
            test_functionality
            ;;
        "runtime")
            check_runtime_issues
            ;;
        "fixes")
            suggest_fixes
            ;;
        "all"|"")
            check_compilation
            analyze_code
            test_functionality
            check_runtime_issues
            suggest_fixes
            ;;
        *)
            echo "Usage: $0 [compile|analyze|test|runtime|fixes|all]"
            echo "  compile  - Check compilation only"
            echo "  analyze  - Analyze code quality"
            echo "  test     - Test basic functionality"
            echo "  runtime  - Check runtime behavior"
            echo "  fixes    - Show common fixes"
            echo "  all      - Run all checks (default)"
            exit 1
            ;;
    esac
}

# Run the main function
main "$@"

echo -e "\n${GREEN}🎉 Debug analysis complete!${NC}"