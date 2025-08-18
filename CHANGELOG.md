# Changelog

## [Current] - 2025-08-18

### ✨ Features
- Complete two-pass assembler implementation
- Pre-assembler with macro expansion
- Symbol table with efficient lookup
- Support for all 16 required instructions
- 4 addressing modes: immediate, direct, matrix, register
- Data directives: .data, .string, .mat, .entry, .extern
- Custom base-4 output format
- Comprehensive error handling

### 🏗️ Architecture
- **ANSI C90 compliant** - fully compatible
- **Zero warnings** compilation with `-Wall -ansi -pedantic`
- **Memory safe** - no leaks, proper cleanup
- **Modular design** - clear separation of concerns

### 📁 File Structure
- `assembler.c` - Main entry point and orchestration
- `preassembler.c` - Macro expansion phase
- `first_pass.c` - Symbol table building
- `symbol_table.c` - Symbol management
- `instruction_*` - Instruction handling
- `line_*` - Line parsing and analysis
- `error_handling.c` - Centralized error management
- `file_utils.c` - File operation utilities

### 🎯 Technical Details
- Memory model: 256 cells of 10 bits each
- Load address: starts at 100
- Base-4 number format (a=0, b=1, c=2, d=3)
- Immediate operand range: [-128, 127]
- Maximum line length: 80 characters

### Previous Changes
- 2025-08-13: Clean project structure, removed old files
- 2025-08-XX: Initial implementation
- 2025-08-XX: Various bug fixes and improvements

