# Save this as: debug_commands.sh
#!/bin/bash

echo "🔍 Step-by-step debugging"
echo "========================"

# 1. Save the fixed debug helper
chmod +x fixed_debug_helper.sh

# 2. Run the fixed debug helper
./fixed_debug_helper.sh

# 3. Create and test correct macro files
chmod +x correct_macro_test.sh
./correct_macro_test.sh

# 4. Manual test of the original issue
echo "🧪 Manual debugging of macro issue..."

# Create a minimal macro test
cat > minimal_macro.as << 'EOF'
mcro simple
mov r1, #5
mcroend

simple
stop
EOF

echo "Testing minimal macro file:"
cat minimal_macro.as
echo ""

echo "Running assembler on minimal macro..."
./assembler minimal_macro

echo ""
echo "🔍 Checking what your assembler expects..."

# Try different formats to see what works
echo "Trying format 1: macro call without label"
cat > format1.as << 'EOF'
mcro test
mov r1, #1
mcroend

test
stop
EOF

echo "Trying format 2: macro call with MAIN label on separate line"
cat > format2.as << 'EOF'
mcro test
mov r1, #1
mcroend

MAIN:
test
stop
EOF

echo "Trying format 3: macro call with indentation"
cat > format3.as << 'EOF'
mcro test
mov r1, #1
mcroend

MAIN:   test
        stop
EOF

for fmt in format1 format2 format3; do
    echo "Testing $fmt:"
    ./assembler "$fmt" 2>&1 | head -2
    echo ""
done

echo "🔧 Check your parser logic for these cases!"

# Cleanup
read -p "Clean up test files? (y/n): " cleanup
if [ "$cleanup" = "y" ]; then
    rm -f minimal_macro.* format*.* 
fi
