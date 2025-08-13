#!/bin/bash

echo "🔧 Fixed Memory Management Checker"
echo "=================================="

# Count malloc/free properly
echo "Checking memory management across all .c files:"
echo ""

total_malloc=0
total_free=0

echo "Per-file analysis:"
echo "------------------"

for file in *.c; do
    if [ -f "$file" ]; then
        malloc_count=$(grep -c "malloc\|calloc" "$file" 2>/dev/null)
        free_count=$(grep -c "free(" "$file" 2>/dev/null)
        
        printf "%-25s malloc/calloc: %2d   free: %2d" "$file" "$malloc_count" "$free_count"
        
        # Add to totals
        total_malloc=$((total_malloc + malloc_count))
        total_free=$((total_free + free_count))
        
        # Show balance for this file
        if [ $malloc_count -gt $free_count ]; then
            echo "   ⚠️ Potential leak"
        elif [ $malloc_count -eq $free_count ]; then
            echo "   ✅ Balanced"
        elif [ $malloc_count -lt $free_count ]; then
            echo "   🔴 More frees than allocs"
        else
            echo "   ✅ OK"
        fi
    fi
done

echo ""
echo "Overall totals:"
echo "---------------"
echo "Total malloc/calloc calls: $total_malloc"
echo "Total free calls:          $total_free"

if [ $total_malloc -gt $total_free ]; then
    echo "🟡 WARNING: More allocations than frees - check for memory leaks"
    echo "   Difference: $((total_malloc - total_free)) potential leaks"
elif [ $total_malloc -eq $total_free ]; then
    echo "✅ GOOD: Allocation/free count is balanced"
elif [ $total_malloc -lt $total_free ]; then
    echo "🔴 ERROR: More frees than allocations - potential double free"
    echo "   Difference: $((total_free - total_malloc)) extra frees"
fi

echo ""
echo "🔍 Detailed malloc/free locations:"
echo "=================================="

echo ""
echo "malloc/calloc locations:"
grep -n "malloc\|calloc" *.c 2>/dev/null | head -10

echo ""
echo "free locations:"
grep -n "free(" *.c 2>/dev/null | head -10

echo ""
echo "💡 Memory management tips:"
echo "========================="
echo "1. Every malloc() should have a corresponding free()"
echo "2. Check return values: if ((ptr = malloc(size)) == NULL) { ... }"
echo "3. Set pointers to NULL after freeing: free(ptr); ptr = NULL;"
echo "4. Use valgrind for runtime memory checking: valgrind ./assembler test1"
