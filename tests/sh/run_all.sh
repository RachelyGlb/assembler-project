#!/bin/bash
for file in tests/*.as; do
    if [ -f "$file" ]; then
        name=$(basename "$file" .as)
        echo "=== Testing: $name ==="
        echo "File: $file"
        ./assembler "${file%.as}"
        echo ""
    fi
done