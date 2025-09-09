##### B"H

# Assembly Language Assembler

**⚠️ Status: Work in Progress - Encoding Bug Under Repair**

This project is a two-pass assembler for a custom assembly language. It translates assembly source files into machine code and generates appropriate output files.

**Current Issue:** The assembler is 95% complete. There's a minor bug in the encoding algorithm for the special base format in the second pass. The logic is correct, but the encoding needs debugging.

---

## 📁 Project Structure

### Core Files
- **assembler.c** - Main entry point and file processing orchestrator
- **assembler.h** - Main header file with common definitions
- **types.h** - Type definitions and constants used throughout the project

### Processing Phases

**Phase 1: Pre-processing**
- **preassembler.c/.h** - Macro expansion and pre-processing
- **macro_and_label_func.c** - Macro and label processing functions

**Phase 2: First Pass** ✅
- **first_pass.c/.h** - First pass analysis and symbol table building
- **line_analysis.c/.h** - Line parsing and analysis utilities
- **line_parser.c** - Line parsing implementation
- **word_extractor.c** - Word extraction utilities
- **symbol_table.c/.h** - Symbol table management

**Phase 3: Validation & Processing** ✅
- **instruction_validation.c/.h** - Instruction syntax validation
- **instruction_table.c/.h** - Instruction definitions and validation
- **error_handling.c** - Error reporting and management
- **file_utils.c** - File handling utilities

**Phase 4: Memory & Output** ⚠️
- **memory_builder.c/.h** - Memory image construction
- **second_pass.c/.h** - Second pass code generation *(encoding bug here)*
- **output_writer.c/.h** - Final output file generation

### Build & Testing
- **Makefile** - Build configuration
- **assembler** - Compiled executable
- **tests/** - Test files directory
- **formal_tester.as** - Formal test input
- **formal_tester.am** - Macro-expanded output
- **formal_tester.ob** - Object file output
- **instruction_count** - Instruction counting utility

### Development Files
- **CHANGELOG.md** - Version history
- **.gitignore** - Git ignore rules
- **.vscode/** - VS Code configuration
- **.git/** - Git repository data

---

## 🔄 Assembly Process

### ✅ Phase 1: Pre-assembler (Working)
- Processes macro definitions and expansions
- Generates .am files for further processing

### ✅ Phase 2: First Pass (Working)
- Analyzes syntax and semantics
- Builds symbol table with labels and their addresses
- Counts instructions and data declarations
- Reports syntax errors

### ✅ Phase 3: Memory Image Building (Working)
- Constructs the memory representation
- Allocates space for instructions and data
- Prepares data structures for code generation

### ⚠️ Phase 4: Second Pass (Bug in Progress)
- Resolves symbol references
- **Issue:** Encoding to special base format has a bug
- Processes .entry and .extern directives
- **Status:** Logic is correct, encoding algorithm needs fixing

---

## 📊 Current Status

| Component | Status | Notes |
|-----------|--------|-------|
| Pre-assembler | ✅ Complete | Macro expansion working |
| First Pass | ✅ Complete | Symbol table & validation |
| Memory Builder | ✅ Complete | Memory allocation working |
| Symbol Table | ✅ Complete | All operations working |
| Error Handling | ✅ Complete | Comprehensive error reporting |
| Second Pass Logic | ✅ Complete | Address resolution working |
| **Encoding Algorithm** | ⚠️ **Bug** | **Special base format issue** |
| Testing Framework | ✅ Complete | Multiple test cases |

**Overall Progress: 95% Complete**

---

## 🚀 Usage

```bash
# Build the project
make

# Run the assembler
./assembler <source_file1> [source_file2] ...

# Example
./assembler formal_tester.as
```

## 📤 Output Files

The assembler generates several output files:
- **.ob** - Object file with machine code
- **.ent** - Entry symbols file  
- **.ext** - External symbols file
- **.am** - Macro-expanded source file

---

## 🏗️ Key Data Structures

- **SymbolTable** - Stores labels, addresses, and symbol attributes
- **MemoryImage** - Represents the final memory layout
- **InstructionTable** - Contains instruction definitions and opcodes
- **MachineWord** - Represents encoded machine instructions

---

## ⚠️ Error Handling

The assembler performs comprehensive error checking:
- Syntax validation
- Symbol redefinition detection
- Address range validation
- Instruction format verification
- Operand type checking

---

## 🗺️ Memory Layout

- **Instructions** - Start at address 100
- **Data** - Allocated after instructions
- **External references** - Tracked separately for linking

---

## 🔧 Current Development Focus

**Immediate Priority:**
- [ ] Debug encoding algorithm in `second_pass.c`
- [ ] Fix special base format conversion
- [ ] Validate output format correctness
- [ ] Run comprehensive integration tests

**Next Steps:**
- [ ] Performance optimization
- [ ] Enhanced error messages
- [ ] Code cleanup and documentation

---

## 📞 Contact

**RachelyGlb**  
📧 Rachely.dev@gmail.com

**Note:** This is an academic project in development. Feedback and suggestions are welcome!

---
