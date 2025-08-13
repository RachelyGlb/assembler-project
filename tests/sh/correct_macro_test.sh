#!/bin/bash

echo "Creating correct test files..."

# Test 1: Simple assembly (no macros)
cat > test_simple.as << 'EOF'
MAIN: mov r1, #5
      add r2, r1
      prn r2
      stop
EOF

# Test 2: Basic macro (correct format)
cat > test_macro.as << 'EOF'
mcro simple_macro
mov r1, #10
add r2, r1
mcroend

MAIN: simple_macro
      prn r2
      stop
EOF

# Test 3: Macro without main label
cat > test_macro_no_label.as << 'EOF'
mcro my_macro
mov r1, #5
mcroend

my_macro
stop
EOF

# Test 4: Multiple macros
cat > test_multiple_macros.as << 'EOF'
mcro init_macro
mov r0, #0
mov r1, #1
mcroend

mcro print_macro
prn r0
prn r1
mcroend

MAIN: init_macro
      print_macro
      stop
EOF

echo "Test files created. Now testing..."

# Test each file
for file in test_simple test_macro test_macro_no_label test_multiple_macros; do
    echo "=================================="
    echo "Testing: $file.as"
    echo "=================================="
    
    # Show file content
    echo "File content:"
    cat "$file.as"
    echo ""
    
    # Run assembler
    echo "Running assembler..."
    ./assembler "$file"
    
    # Check results
    if [ $? -eq 0 ]; then
        echo "✓ SUCCESS"
        if [ -f "$file.am" ]; then
            echo "Output file content:"
            cat "$file.am"
        fi
    else
        echo "✗ FAILED"
    fi
    
    echo ""
    echo ""
done

# Cleanup option
echo "Do you want to keep the test files? (y/n)"
read -r keep_files
if [ "$keep_files" != "y" ]; then
    rm -f test_simple.* test_macro.* test_macro_no_label.* test_multiple_macros.*
    echo "Test files cleaned up."
fi