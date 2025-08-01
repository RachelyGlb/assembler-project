# Makefile for Assembler Project - Phase 1 (Pre-assembler)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -ansi -pedantic

# Source files
SOURCES = assembler.c preprocessor.c macro.c utils.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = assembler.h

# Target executable
TARGET = assembler

# Default target
all: $(TARGET)

# Build the assembler
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile individual object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Clean all generated files (including test outputs)
cleanall: clean
	rm -f *.am *.ob *.ent *.ext

# Install (copy to system directory)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Run tests (you can add test files here)
test: $(TARGET)
	@echo "Running basic tests..."
	@echo "Test 1: No input files"
	./$(TARGET)
	@echo "Test 2: Non-existent file"
	./$(TARGET) nonexistent
	@echo "Tests completed."

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build (optimized)
release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# Check for memory leaks (requires valgrind)
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) test_file

# Print variables (for debugging makefile)
print-%:
	@echo $* = $($*)

# Phony targets
.PHONY: all clean cleanall install uninstall test debug release memcheck

# Dependencies
assembler.o: assembler.c assembler.h
preprocessor.o: preprocessor.c assembler.h
macro.o: macro.c assembler.h
utils.o: utils.c assembler.h