# Modules

This file describes the modules of the assembler as they are. The table was
generated from the headers and checked against the symbols of the object
files by the final review of Phase 4 (`docs/audit/REVIEW_FINAL.md`). Update
it whenever a public function is added, removed or renamed.

## Why the split differs from the first plan

`CLAUDE.md` lists twelve expected modules and allows the split to be adapted
with a recorded reason. The adaptations are:

- `statement` is separate from `syntax`: the parsed line (an opaque type, its
  builders and accessors, and the layout of an instruction in words) is a
  different responsibility from the parser that fills it, and the passes need
  only the former.
- `am_reader` holds the loop that reads the `.am` file, parses each line and
  hands the statement to a pass. Both passes had the same loop; it needs the
  parser, so it cannot live in `util`, and neither pass may depend on the
  other.
- `line_map` maps `.am` line numbers to `.as` line numbers, because errors
  are reported with the line of the original source file (SPEC ERR-R12).
- `reference_list` keeps the uses of external symbols for the `.ext` file,
  which is not a property of a symbol and so does not belong in the symbol
  table.
- `encoder` and `base4` are two modules: the first knows the bit layout of a
  machine word, the second only the letter notation of the output files.
- `util` gathers the language-neutral helpers (white space, checked copies,
  the growing array, reading a line of any length) that several modules use.

## Dependencies

Direct dependencies (project headers included by the `.c` or the `.h` file).
The graph has no cycle.

```
assembler         -> common, errors, util, macro_table, line_map, symbol_table,
                     memory_image, reference_list, preassembler, first_pass,
                     second_pass, output_writer
preassembler      -> common, errors, util, instruction_table, macro_table, line_map
first_pass        -> common, errors, am_reader, statement, instruction_table,
                     encoder, macro_table, line_map, symbol_table, memory_image
second_pass       -> common, errors, am_reader, statement, encoder, line_map,
                     symbol_table, memory_image, reference_list
output_writer     -> common, errors, util, base4, memory_image, symbol_table,
                     reference_list
am_reader         -> common, errors, util, line_map, statement, syntax
syntax            -> common, errors, util, instruction_table, statement
statement         -> common, util, instruction_table
macro_table       -> common, util
line_map          -> common, util
symbol_table      -> common, util
reference_list    -> common, util
instruction_table -> common
memory_image      -> common
encoder           -> common
base4             -> common
errors            -> common
util              -> common

## The modules

| Module (files) | Single responsibility | Opaque types it owns | Public functions | Lines |
|---|---|---|---|---|
| `assembler.c` | Program entry and per-file driver: removes stale outputs, runs the stages in order, frees everything, sets the exit status. | none | `main` | 275 |
| `common.h` | Shared plain definitions: limits, addresses, extensions, keywords, shared enumerations (`Boolean`, `StageResult`, `AddressingMode`). | none | none | 107 |
| `errors.c/.h` | The one place that prints diagnostics, in the ERR-R13 and ERR-R14 formats; counts errors per file. | `ErrorContext` | `errors_create`, `errors_report`, `errors_report_list`, `errors_warn`, `errors_report_no_memory`, `errors_count`, `errors_destroy` | 276 |
| `util.c/.h` | Language-neutral helpers: white space, string copies, file names, the growing array, reading a line of any length. Other type: `ReadStatus`. | none | `util_is_space`, `util_skip_spaces`, `util_skip_field`, `util_is_blank`, `util_copy_string`, `util_copy_text`, `util_make_file_name`, `util_reserve`, `util_read_line` | 356 |
| `instruction_table.c/.h` | The words of the language: the single instruction table, the registers, the directives, the reserved words. Other type: `DirectiveKind`. | `Instruction` | `instruction_find_text`, `instruction_opcode`, `instruction_operand_count`, `instruction_allows_source`, `instruction_allows_destination`, `register_number`, `register_number_text`, `directive_find_text`, `is_reserved_word` | 311 |
| `macro_table.c/.h` | The macros of one source file: names and body lines with their `.as` line numbers. | `MacroTable`, `Macro` | `macro_table_create`, `macro_table_destroy`, `macro_table_add`, `macro_add_line`, `macro_table_find`, `macro_table_find_text`, `macro_name`, `macro_line_count`, `macro_line`, `macro_line_source` | 307 |
| `line_map.c/.h` | The map from `.am` line numbers to `.as` line numbers. | `LineMap` | `line_map_create`, `line_map_destroy`, `line_map_append`, `line_map_source_line` | 123 |
| `preassembler.c/.h` | The pre-assembler: line checks, macro definitions and expansion, the `.am` file and the line map. | none | `preassemble` | 649 |
| `statement.c/.h` | The model of one parsed line and the layout of an instruction in words. Other types: `StatementKind`, `MatrixIndex`. | `Statement` | `statement_create`, `statement_destroy`, `statement_mark_invalid`, `statement_set_kind`, `statement_set_label`, `statement_ignore_label`, `statement_set_instruction`, `statement_add_immediate_operand`, `statement_add_direct_operand`, `statement_add_matrix_operand`, `statement_add_register_operand`, `statement_add_data_value`, `statement_extend_data`, `statement_set_directive_symbol`, `statement_is_valid`, `statement_kind`, `statement_stores_data`, `statement_label`, `statement_has_ignored_label`, `statement_instruction`, `statement_operand_count`, `statement_operand_is_source`, `statement_operand_mode`, `statement_operand_value`, `statement_operand_symbol`, `statement_operand_matrix_register`, `statement_operand_word_offset`, `statement_operand_index_word_offset`, `statement_word_count`, `statement_data_count`, `statement_data_value`, `statement_directive_symbol` | 735 |
| `syntax.c/.h` | The parser: one line of the `.am` file in, one `Statement` out, with every syntax error of the line. | none | `syntax_parse` | 1,271 |
| `am_reader.c/.h` | The walk over the `.am` file shared by both passes: read, parse, translate the line number, call the pass. Other type: `StatementHandler`. | none | `am_reader_for_each` | 133 |
| `symbol_table.c/.h` | The symbols of one source file: name, value, kind, entry attribute. Other types: `SymbolKind`, `SymbolVisitor`. | `SymbolTable`, `Symbol` | `symbol_table_create`, `symbol_table_destroy`, `symbol_table_add`, `symbol_table_find`, `symbol_table_mark_entry`, `symbol_table_relocate_data`, `symbol_table_for_each`, `symbol_name`, `symbol_address`, `symbol_kind`, `symbol_is_entry` | 306 |
| `memory_image.c/.h` | The code image and the data image with IC, DC and the memory limit. | `MemoryImage` | `memory_image_create`, `memory_image_destroy`, `memory_image_free_words`, `memory_image_add_code`, `memory_image_add_data`, `memory_image_merge_code`, `memory_image_instruction_counter`, `memory_image_data_counter`, `memory_image_code_count`, `memory_image_code_word`, `memory_image_data_word` | 218 |
| `encoder.c/.h` | The layout of the 10-bit machine words: every field position and width. Other type: `AreKind`. | none | `encode_first_word`, `encode_immediate`, `encode_registers`, `encode_matrix_indices`, `encode_address`, `encode_data` | 180 |
| `base4.c/.h` | The "unique base 4" notation of the output files. | none | `base4_format`, `base4_format_shortest` | 101 |
| `reference_list.c/.h` | The ordered list of uses of external symbols (name, address). | `ReferenceList` | `reference_list_create`, `reference_list_destroy`, `reference_list_append`, `reference_list_count`, `reference_list_name`, `reference_list_address` | 181 |
| `first_pass.c/.h` | The first pass: symbols, data image, code image without symbol addresses, memory limit, WRN-1. | none | `first_pass` | 487 |
| `second_pass.c/.h` | The second pass: entries, symbol addresses, external references. | none | `second_pass` | 216 |
| `output_writer.c/.h` | Writing `.ob`, `.ent`, `.ext` and removing them. | none | `write_output_files`, `remove_output_files` | 357 |

Totals: 18 modules with a `.c` file plus `common.h`; 10 opaque types; 115
