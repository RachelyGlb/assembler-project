# Makefile for the two-pass assembler of the imaginary 10-bit machine.
#
# It builds the `assembler` binary from every module in this directory and
# runs the test suite. There are exactly three targets:
#
#   make          Build the `assembler` binary (the default target, `all`).
#   make test     Build the binary and run the test suite in tests/.
#   make clean    Remove everything that the build and the tests generate.

CC = gcc
CFLAGS = -Wall -ansi -pedantic

# The options below make gcc write one dependency file (.d) next to every
# object file. Each .d file lists all headers that the source file includes,
# directly or indirectly, so a change to a header rebuilds exactly the
# objects that depend on it. -MP adds an empty rule for every header, so that
# deleting or renaming a header does not stop the build.
DEPFLAGS = -MMD -MP

TARGET = assembler

SOURCES = assembler.c \
          base4.c \
          encoder.c \
          errors.c \
          first_pass.c \
          instruction_table.c \
          line_map.c \
          macro_table.c \
          memory_image.c \
          output_writer.c \
          preassembler.c \
          reference_list.c \
          second_pass.c \
          symbol_table.c \
          syntax.c \
          util.c

OBJECTS = $(SOURCES:.c=.o)
DEPENDS = $(SOURCES:.c=.d)

# The directory in which the test runner writes its results.
TEST_OUTPUT_DIR = tests/out

.PHONY: all test clean

# Build the assembler. This is the default target.
all: $(TARGET)

# Link all object files into the binary.
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

# Compile one source file and write its dependency file as a side effect.
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Run the real binary on the test cases and compare with the expected files.
test: $(TARGET)
	ASSEMBLER=./$(TARGET) sh tests/run_tests.sh

# Remove the objects, the dependency files, the binary and the directory of
# test results. The expected files under tests/valid, tests/errors and
# tests/cli are tracked sources and are never removed here.
clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET)
	rm -rf $(TEST_OUTPUT_DIR)

# Read the generated dependency files. The leading dash makes their absence
# on the first build, or after `make clean`, not an error.
-include $(DEPENDS)
