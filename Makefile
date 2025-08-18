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
          word_extractor.c \

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

# Help
help:
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
.PHONY: all clean rebuild install uninstall debug release help