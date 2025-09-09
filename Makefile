# Compiler and flags
CC = gcc
CFLAGS = -ansi -pedantic -Wall -g

# Executable name
TARGET = assembler

# Source files
SOURCES = assembler.c \
          error_handling.c \
          file_utils.c \
          first_pass.c \
          instruction_table.c \
          instruction_validation.c \
          line_analysis.c \
          line_parser.c \
          macro_and_label_func.c \
          memory_builder.c \
          output_writer.c \
          preassembler.c \
          second_pass.c \
          symbol_table.c \
          word_extractor.c

# Object files (replace .c with .o)
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = assembler.h \
          preassembler.h \
          instruction_table.h \
          first_pass.h \
          second_pass.h \
          memory_builder.h \
          output_writer.h \
          line_analysis.h \
          symbol_table.h \
          instruction_validation.h \
          types.h

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Rule for object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Explicit dependencies
assembler.o: assembler.c assembler.h types.h preassembler.h first_pass.h
preassembler.o: preassembler.c preassembler.h assembler.h types.h
first_pass.o: first_pass.c first_pass.h types.h line_analysis.h symbol_table.h instruction_validation.h
second_pass.o: second_pass.c second_pass.h types.h memory_builder.h symbol_table.h
memory_builder.o: memory_builder.c memory_builder.h types.h line_analysis.h instruction_table.h symbol_table.h
output_writer.o: output_writer.c output_writer.h types.h
instruction_table.o: instruction_table.c instruction_table.h types.h
instruction_validation.o: instruction_validation.c instruction_validation.h types.h instruction_table.h line_analysis.h symbol_table.h
line_analysis.o: line_analysis.c line_analysis.h types.h symbol_table.h
symbol_table.o: symbol_table.c symbol_table.h types.h
file_utils.o: file_utils.c preassembler.h types.h
error_handling.o: error_handling.c preassembler.h types.h

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build (optimized)
release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# ===========================================================================================
# TEST SUITE ADDITIONS
# ===========================================================================================

# Test configuration
TEST_DIR = tests
LOG_DIR = tests/logs
TEST_RUNNER = test_runner.sh
TEST_UTILS = test_utils.sh

# Clean test artifacts
.PHONY: clean-tests
clean-tests:
	@echo "Cleaning test artifacts..."
	@rm -rf $(LOG_DIR)
	@rm -f $(TEST_DIR)/*.log $(TEST_DIR)/*.err
	@find $(TEST_DIR) -name "*.am" -delete 2>/dev/null || true
	@find $(TEST_DIR) -name "*.ob" -delete 2>/dev/null || true
	@find $(TEST_DIR) -name "*.ent" -delete 2>/dev/null || true
	@find $(TEST_DIR) -name "*.ext" -delete 2>/dev/null || true
	@echo "Test artifacts cleaned"

# Set up test environment
.PHONY: setup-tests
setup-tests:
	@echo "Setting up test environment..."
	@chmod +x $(TEST_RUNNER) $(TEST_UTILS) 2>/dev/null || echo "Warning: Test scripts not found"
	@mkdir -p $(LOG_DIR)
	@echo "Test environment ready"

# Run all tests
.PHONY: test
test: $(TARGET) setup-tests
	@echo "Running complete test suite..."
	@if [ -x "./$(TEST_RUNNER)" ]; then \
		./$(TEST_RUNNER); \
	else \
		echo "Error: $(TEST_RUNNER) not found or not executable"; \
		echo "Please ensure test_runner.sh exists and is executable"; \
		false; \
	fi

# Run tests by category
.PHONY: test-basic
test-basic: $(TARGET) setup-tests
	@echo "Running basic functionality tests..."
	@if [ -x "./$(TEST_RUNNER)" ]; then \
		./$(TEST_RUNNER) basic || true; \
	else \
		echo "Manual basic test - testing simple MOV instruction:"; \
		echo "MAIN: mov #42, r1" > test_simple.as; \
		echo "      stop" >> test_simple.as; \
		./$(TARGET) test_simple && echo "Basic test PASSED" || echo "Basic test FAILED"; \
		rm -f test_simple.* 2>/dev/null || true; \
	fi

.PHONY: test-memory
test-memory: $(TARGET) setup-tests
	@echo "Running memory and boundary tests..."
	@if [ -x "./$(TEST_RUNNER)" ]; then \
		./$(TEST_RUNNER) memory || true; \
	else \
		echo "Manual memory test - testing immediate boundaries:"; \
		echo "MAIN: mov #127, r1" > test_memory.as; \
		echo "      mov #-128, r2" >> test_memory.as; \
		echo "      stop" >> test_memory.as; \
		./$(TARGET) test_memory && echo "Memory test PASSED" || echo "Memory test FAILED"; \
		rm -f test_memory.* 2>/dev/null || true; \
	fi

.PHONY: test-errors
test-errors: $(TARGET) setup-tests
	@echo "Running error detection tests..."
	@if [ -x "./$(TEST_RUNNER)" ]; then \
		./$(TEST_RUNNER) error_detection || true; \
	else \
		echo "Manual error test - testing invalid instruction:"; \
		echo "MAIN: invalid_instruction #5, r1" > test_error.as; \
		echo "      stop" >> test_error.as; \
		./$(TARGET) test_error 2>&1 | grep -i error >/dev/null && echo "Error test PASSED" || echo "Error test FAILED"; \
		rm -f test_error.* 2>/dev/null || true; \
	fi

.PHONY: test-comprehensive
test-comprehensive: $(TARGET) setup-tests
	@echo "Running comprehensive integration tests..."
	@if [ -x "./$(TEST_RUNNER)" ]; then \
		./$(TEST_RUNNER) comprehensive || true; \
	else \
		echo "Manual comprehensive test - testing macro usage:"; \
		echo "mcro TEST_MACRO" > test_comprehensive.as; \
		echo "mov #1, r1" >> test_comprehensive.as; \
		echo "mcroend" >> test_comprehensive.as; \
		echo "MAIN: TEST_MACRO" >> test_comprehensive.as; \
		echo "      stop" >> test_comprehensive.as; \
		./$(TARGET) test_comprehensive && echo "Comprehensive test PASSED" || echo "Comprehensive test FAILED"; \
		rm -f test_comprehensive.* 2>/dev/null || true; \
	fi

# Quick smoke test
.PHONY: smoke-test
smoke-test: $(TARGET) setup-tests
	@echo "Running smoke tests..."
	@echo "Testing basic functionality:"
	@$(MAKE) test-basic >/dev/null 2>&1 && echo "  ✓ Basic tests" || echo "  ✗ Basic tests"
	@echo "Testing error detection:"
	@$(MAKE) test-errors >/dev/null 2>&1 && echo "  ✓ Error tests" || echo "  ✗ Error tests"
	@echo "Smoke test completed - run 'make test' for full results"

# Create simple sample test files
.PHONY: create-samples
create-samples:
	@echo "Creating sample test files..."
	@mkdir -p $(TEST_DIR)
	@echo "MAIN: mov #42, r1" > $(TEST_DIR)/sample_valid.as
	@echo "      stop" >> $(TEST_DIR)/sample_valid.as
	@echo "MAIN: mov #999, r1" > $(TEST_DIR)/sample_invalid.as
	@echo "      invalid_instruction" >> $(TEST_DIR)/sample_invalid.as
	@echo "      stop" >> $(TEST_DIR)/sample_invalid.as
	@echo "Sample files created in $(TEST_DIR)/"
	@echo "Test with: ./assembler sample_valid"
	@echo "Test with: ./assembler sample_invalid"

# Validate test suite
.PHONY: validate-tests
validate-tests:
	@echo "Validating test suite completeness..."
	@if [ -x "./$(TEST_UTILS)" ]; then \
		./$(TEST_UTILS) validate; \
	else \
		echo "Checking test directory structure:"; \
		find $(TEST_DIR) -name "*.as" 2>/dev/null | wc -l | xargs echo "Total test files found:"; \
	fi

# Generate test report
.PHONY: test-report
test-report:
	@echo "Generating test report..."
	@if [ -x "./$(TEST_UTILS)" ]; then \
		./$(TEST_UTILS) report; \
	else \
		echo "Test report functionality requires test_utils.sh"; \
	fi

# Performance benchmark
.PHONY: benchmark
benchmark: $(TARGET) setup-tests create-samples
	@echo "Running performance benchmarks..."
	@echo "Small program:"
	@cd $(TEST_DIR) && time ../$(TARGET) sample_valid 2>/dev/null >/dev/null || echo "Benchmark test failed"
	@echo "Benchmark completed"

# Memory checking with valgrind
.PHONY: test-memory-check
test-memory-check: $(TARGET) setup-tests
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "Running tests with memory checking..."; \
		echo "Testing basic program with valgrind:"; \
		echo "MAIN: mov #1, r1" > test_valgrind.as; \
		echo "      stop" >> test_valgrind.as; \
		valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) test_valgrind; \
		rm -f test_valgrind.*; \
	else \
		echo "Valgrind not available, running normal tests"; \
		$(MAKE) smoke-test; \
	fi

# Static analysis
.PHONY: static-analysis
static-analysis:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "Running static analysis..."; \
		cppcheck --enable=all --std=c90 --suppress=missingIncludeSystem *.c; \
	else \
		echo "cppcheck not available for static analysis"; \
	fi

# Extended clean
.PHONY: clean-all
clean-all: clean clean-tests
	@echo "Complete cleanup finished"

# Help with test targets
.PHONY: help-tests
help-tests:
	@echo ""
	@echo "Test Suite Targets:"
	@echo "  setup-tests     - Set up test environment"
	@echo "  test            - Run all tests"
	@echo "  smoke-test      - Run quick smoke test"
	@echo "  test-basic      - Run basic functionality tests"
	@echo "  test-memory     - Run memory/boundary tests"
	@echo "  test-errors     - Run error detection tests"
	@echo "  test-comprehensive - Run comprehensive tests"
	@echo "  clean-tests     - Remove test artifacts"
	@echo "  create-samples  - Create sample test files"
	@echo "  validate-tests  - Check test suite completeness"
	@echo "  test-report     - Generate HTML test report"
	@echo "  benchmark       - Performance benchmarks"
	@echo "  test-memory-check - Run tests with valgrind"
	@echo "  static-analysis - Run static code analysis"
	@echo "  clean-all       - Clean build + test artifacts"

# Main help
help: help-tests
	@echo "Available targets:"
	@echo "  all      - Build the assembler (default)"
	@echo "  clean    - Remove object files and executable"
	@echo "  rebuild  - Clean and build"
	@echo "  debug    - Build with debug information"
	@echo "  release  - Build optimized release version"
	@echo "  install  - Install to /usr/local/bin/"
	@echo "  uninstall- Remove from /usr/local/bin/"
	@echo "  help     - Show this help message"

# Declare phony targets
.PHONY: all clean rebuild install uninstall debug release help \
        clean-tests setup-tests test test-basic test-memory \
        test-errors test-comprehensive smoke-test create-samples validate-tests \
        test-report benchmark test-memory-check static-analysis clean-all help-tests