# Assembly Language Assembler

This project is a two-pass assembler for a custom assembly language. It translates assembly source files into machine code and generates appropriate output files.

## Project Structure

### Core Files

- **assembler.c** - Main entry point and file processing orchestrator
- **assembler.h** - Main header file with common definitions
- **types.h** - Type definitions and constants used throughout the project

### Processing Phases

1. **preassembler.h** - Pre-processing phase (macro expansion)
2. **first_pass.c/.h** - First pass analysis and symbol table building
3. **second_pass.c/.h** - Second pass code generation and address resolution
4. **memory_builder.c/.h** - Memory image construction
5. **output_writer.c/.h** - Final output file generation

### Support Modules

- **symbol_table.c/.h** - Symbol table management
- **instruction_table.c/.h** - Instruction definitions and validation
- **instruction_validation.c/.h** - Instruction syntax validation
- **line_analysis.c/.h** - Line parsing and analysis utilities
- **line_parser.c** - Line parsing implementation
- **word_extractor.c** - Word extraction utilities
- **macro_and_label_func.c** - Macro and label processing functions
- **file_utils.c** - File handling utilities
- **error_handling.c** - Error reporting and management

## Assembly Process

### Phase 1: Pre-assembler
- Processes macro definitions and expansions
- Generates .am files for further processing

### Phase 2: First Pass
- Analyzes syntax and semantics
- Builds symbol table with labels and their addresses
- Counts instructions and data declarations
- Reports syntax errors

### Phase 3: Memory Image Building
- Constructs the memory representation
- Allocates space for instructions and data
- Prepares data structures for code generation

### Phase 4: Second Pass
- Resolves symbol references
- Generates final machine code
- Processes .entry and .extern directives
- Creates output files

## Output Files

The assembler generates several output files:
- **.ob** - Object file with machine code
- **.ent** - Entry symbols file
- **.ext** - External symbols file

## Usage

```bash
./assembler <source_file1> [source_file2] ...
```

## Build

```bash
make
```

## Key Data Structures

- **SymbolTable** - Stores labels, addresses, and symbol attributes
- **MemoryImage** - Represents the final memory layout
- **InstructionTable** - Contains instruction definitions and opcodes
- **MachineWord** - Represents encoded machine instructions

## Error Handling

The assembler performs comprehensive error checking:
- Syntax validation
- Symbol redefinition detection
- Address range validation
- Instruction format verification
- Operand type checking

## Memory Layout

- **Instructions** - Start at address 100
- **Data** - Allocated after instructions
- **External references** - Tracked separately for linking
