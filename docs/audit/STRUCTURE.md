# Structure and cleanliness audit

Auditor role: structure and cleanliness, read-only. Date: 2026-11-26.
Scope: every `.c` and `.h` file in the repository root, `Makefile`,
`README.md`, `CHANGELOG.md`, `.gitignore`, and the legacy scripts under
`tests/sh/`. The checklist is the section "Coding rules (binding)" of
`CLAUDE.md`, with `docs/SPEC.md` rule IMP-1 narrowing the rule on const tables.

No file in the repository was changed. The sources were copied to a scratch
directory, a lowercase copy of `Assembler.h` was added there, and the copy was
built with `gcc -Wall -ansi -pedantic` (gcc 13.3.0) and run under valgrind.

## Findings per rule

| Rule | Subject | Findings |
|---|---|---|
| 1 | Module split | 19 |
| 2 | Information hiding | 8 |
| 3 | Instruction table | 10 |
| 4 | Global variables | 4 |
| 5 | Functions | 4 |
| 6 | Magic numbers | 16 |
| 7 | Error reporting | 7 |
| 8 | Memory | 10 |
| 9 | Documentation | 8 |
| 10 | C90 conformance | 9 |
| 11 | Makefile | 7 |
| 12 | Repository hygiene | 9 |
| | Total | 111 |

One finding is one numbered item below. Several items list many locations of
the same problem; the number of individual locations is far higher than the
number of findings (for example, rule 7 covers 105 direct print calls in
eight files, next to 33 calls of the two existing error functions).

## Size of the code

| File | Lines | File | Lines |
|---|---|---|---|
| `memory_builder.c` | 670 | `Assembler.h` | 53 |
| `preassembler.c` | 496 | `types.h` | 93 |
| `second_pass.c` | 461 | `preassembler.h` | 147 |
| `macro_and_label_func.c` | 422 | `second_pass.h` | 41 |
| `line_analysis.c` | 393 | `line_analysis.h` | 34 |
| `output_writer.c` | 356 | `memory_builder.h` | 33 |
| `instruction_validation.c` | 314 | `output_writer.h` | 23 |
| `first_pass.c` | 262 | `symbol_table.h` | 22 |
| `assembler.c` | 174 | `instruction_table.h` | 16 |
| `symbol_table.c` | 135 | `instruction_validation.h` | 13 |
| `word_extractor.c` | 110 | `first_pass.h` | 12 |
| `instruction_table.c` | 76 | | |
| `error_handling.c` | 67 | | |
| `file_utils.c` | 51 | | |
| `line_parser.c` | 21 | | |

Fifteen `.c` files hold 4008 lines and eleven `.h` files hold 487 lines; the
`Makefile` has 301 lines. There are 107 function definitions; 5 are `static`.

## Evidence gathered by running the code

These runs were made on the scratch copy. They are quoted here because several
structural findings below are the direct cause.

- `make` in the repository: `make: *** No rule to make target 'assembler.h',
  needed by 'assembler.o'.  Stop.`
- `gcc -Wall -ansi -pedantic *.c` prints exactly two warnings:
  `output_writer.c:231:5: warning: pointer 'ent_filename' used after 'free'
  [-Wuse-after-free]` and `output_writer.c:293:5: warning: pointer
  'ext_filename' used after 'free' [-Wuse-after-free]`.
- `./assembler a b` with two identical valid files: the second file fails with
  `Error on line 1: Duplicate label: MAIN`. The symbol table is a global that
  is never emptied (R4-01, R8-06).
- A source line `  L1: inc r1` (a label after leading blanks) aborts the
  process: `free(): invalid pointer`, exit status 134 (R8-04).
- `A: .data 6, -9, 15` produces one data word instead of three; the `.ob`
  header is `b b`. The first pass counts three (R1-07, R4-04).
- valgrind on a valid file: invalid reads in `write_entries_file` (use after
  free), 472 bytes still reachable per input file (the `FILE` that
  `first_pass` never closes) and 56 bytes per symbol still reachable.
- A valid six-line program prints 56 lines of progress and debug text on
  standard output.

---

## Rule 1: module split

- **R1-01** The build is broken by a file-name mismatch. The header on disk is
  `Assembler.h`, while `types.h:5`, `preassembler.h:4`, `assembler.c:19` and
  `Makefile:29,53,54` name `assembler.h`.
- **R1-02** `preassembler.h` is a catch-all header. Its prototypes
  (`preassembler.h:47-95`) are defined in six different source files:
  `macro_and_label_func.c`, `word_extractor.c`, `file_utils.c`,
  `line_parser.c`, `line_analysis.c` and `preassembler.c`. It also carries the
  generic-table structs and four control-flow macros.
- **R1-03** Six source files have no header of their own:
  `error_handling.c`, `file_utils.c`, `line_parser.c`, `word_extractor.c`,
  `macro_and_label_func.c` (all declared through `preassembler.h`) and
  `assembler.c` (whose header also declares the error function).
- **R1-04** `Assembler.h` mixes five concerns: the `Boolean` type (10-14), the
  error codes of the pre-assembler (17-33), length limits (35-37), the
  prototype of `print_error` defined in `error_handling.c` (40), file
  extensions (44-48) and the internal functions of `main` (51-52).
- **R1-05** There are two "shared definitions" headers, `Assembler.h` and
  `types.h`. `types.h` defines the data structures of every module (memory
  image, symbol, instruction row, external reference, entry point), so every
  module can see every other module's representation.
- **R1-06** There are three passes over the `.am` file, not two, and
  `memory_builder.c` is the third. `build_memory_image`
  (`memory_builder.c:110-187`) reads the file again and mixes file reading,
  line classification, directive parsing, instruction encoding and image
  storage in one module. The memory-image responsibility (store words) and the
  encoding responsibility (build words) are not separated.
- **R1-07** The same source line is parsed by three independent parsers that
  disagree: `validate_command_line` and `parse_operands` with `sscanf`
  (`instruction_validation.c:270`, `line_analysis.c:185`),
  `encode_instruction_first_pass` with `strtok` (`memory_builder.c:357-403`)
  and `process_instruction_second_pass` with `strtok`
  (`second_pass.c:132-185`). The instruction length is computed three times:
  `count_command_words` (`line_analysis.c:279-343`), the emission in
  `encode_instruction_first_pass` (`memory_builder.c:416-515`) and
  `get_instruction_length` (`second_pass.c:331-370`). Data size is computed
  twice: `count_data_items`, `count_string_length`, `count_matrix_items`
  (`line_analysis.c:137-174,241-278`) against the three `encode_*_directive`
  functions (`memory_builder.c:190-336`). This is a structural cause of bugs:
  any disagreement between the counters shifts every later address. The
  `.data 6, -9, 15` run above is one such disagreement.
- **R1-08** `second_pass.c` holds four responsibilities: the second pass, the
  lists of external references and entry points (372-405, 441-462), the base-4
  conversion `decimal_to_base4` (408-438) and the decision to write the output
  files (`second_pass.c:106`). Output writing is started from inside the pass
  instead of from the driver.
- **R1-09** Error reporting lives in three places: `print_error`
  (`error_handling.c:5`), `error` (`first_pass.c:27`, declared in
  `first_pass.h:10`) and direct `printf` calls everywhere else (rule 7).
- **R1-10** The pre-assembler keeps its own label table and validates labels
  (`preassembler.c:248-315`, `macro_and_label_func.c:275-296`). The first pass
  validates the same labels again with a different rule (R1-13). Label
  handling belongs to the first pass and the symbol table; the pre-assembler
  only needs to hand its macro names on.
- **R1-11** Instruction knowledge is split over `instruction_table.c` (name,
  opcode, count), `instruction_validation.c` (legal modes),
  `memory_builder.c` (opcodes again) and `line_analysis.c` (names again). See
  rule 3.
- **R1-12** Prototypes are declared in more than one header:
  `decimal_to_base4` in `output_writer.h:21` and `second_pass.h:37` (defined
  in `second_pass.c:408`); `free_memory_image` in `memory_builder.h:23` and
  `second_pass.h:34`; `validate_command_line` in `line_analysis.h:27` and
  `instruction_validation.h:9`; `is_empty_line`, `is_comment_line` and
  `trim_whitespace` in `line_analysis.h:30-32` and `preassembler.h:88-91`.
- **R1-13** Duplicated code between files:
  - Label-name validity: `is_valid_label` (`line_analysis.c:26-39`, rejects
    length >= 31) against `is_valid_name` (`macro_and_label_func.c:382-410`,
    rejects length > 31). The two limits differ by one.
  - Label detection and extraction: `extract_label` (`line_analysis.c:10`),
    `is_label` (`line_analysis.c:55`), `has_label`
    (`macro_and_label_func.c:367`), `extract_label_name`
    (`macro_and_label_func.c:323`), and inline code in `is_command`
    (`line_analysis.c:104-122`), `first_pass.c:215-220`,
    `memory_builder.c:200-204,360-364`, `second_pass.c:135-139`.
  - Comment and empty line: `is_comment_or_empty` (`line_analysis.c:46`)
    against `is_empty_line` and `is_comment_line` (`line_analysis.c:345,361`).
  - `get_first_word` and `get_second_word` (`word_extractor.c:4-79`) are the
    same function with one extra `strtok` call.
  - `write_entries_file` and `write_externals_file`
    (`output_writer.c:174-295`) are identical apart from the extension.
  - `ExtRef` and `EntryPoint` (`types.h:77-90`) are the same struct, with
    duplicated add, free and count functions (`second_pass.c:372-405,441-462`,
    `output_writer.c:329-356`).
  - The scan of a `.string` operand: `line_analysis.c:241-278` and
    `memory_builder.c:238-281`.
  - Register parsing inside a matrix operand:
    `instruction_validation.c:48-74` and `memory_builder.c:636-670`.
  - Ten-bit two's complement: `memory_builder.c:223-228`, `323-328`,
    `603-608` and `second_pass.c:420-430`.
  - Operand splitting and trimming: `memory_builder.c:376-403` and
    `second_pass.c:158-185`.
  - Writing a line and repairing its newline: `preassembler.c:388-396`,
    `423-439`, `445-450`, `461-466`.
  - Building an output file name: `create_filename_with_extension`
    (`file_utils.c:21`) against `get_base_filename` plus `sprintf`
    (`output_writer.c:53-63,186-196,248-258`).
  - The memory-overflow check, three identical copies:
    `first_pass.c:181-183`, `189-191`, `197-199`.
  - The body of a macro is copied three times before it is stored:
    `macro_lines` (`preassembler.c:340-351`), `macro_content`
    (`preassembler.c:191-212`) and again inside `add_macro`
    (`macro_and_label_func.c:238-261`).
- **R1-14** Functions that are defined but never called:
  `cleanup_and_exit` (`file_utils.c:40`), `is_valid_instruction`,
  `get_instruction_operand_count`, `init_instruction_table`,
  `free_instruction_table` (`instruction_table.c:48,54,67,73`), `label_exists`
  (`line_analysis.c:41`), `is_label` (`line_analysis.c:55`), `is_macro_name`
  (`macro_and_label_func.c:4`) and `free_symbol_table`
  (`symbol_table.c:127`). The last one is the reason the symbol table leaks
  from one input file into the next.
- **R1-15** Functions that are declared but never defined:
  `is_symbol_defined`, `mark_symbol_as_entry` and `get_symbol_count`
  (`symbol_table.h:17-19`).
- **R1-16** A function that is defined without any declaration in a header and
  is not `static`: `cleanup_macro_data` (`macro_and_label_func.c:189`).
  `instruction_validation.c:6-8` does not include its own header
  `instruction_validation.h`, so its prototypes are never checked against
  their definitions.
- **R1-17** Dead declarations, types, constants and data:
  macro `REPORT_ERROR_AND_BREAK` (`preassembler.h:123-134`); types
  `AddressingType` (`types.h:24-30`), `LabelTable` and `LabelNode`
  (`preassembler.h:40,42`); constants `MAX_FILENAME_LENGTH`
  (`Assembler.h:37`), `WORD_SIZE` (`types.h:10`), `ARE_ABSOLUTE`
  (`types.h:13`), `OB_EXTENSION`, `ENT_EXTENSION`, `EXT_EXTENSION`
  (`Assembler.h:46-48`); error codes `MACRO_NOT_CLOSED`, `RESERVED_WORD`,
  `SYNTAX_ERROR` that are never raised (`Assembler.h:26,30,32`); the code
  `MACRO_ERROR`, which is raised (`preassembler.c:124,217`) but has no `case`
  in `print_error` and prints "Unknown error"; fields `data_memory`
  (`types.h:50`), `GenericNode.line_number` and `GenericTable.count`
  (`preassembler.h:20,28`), which are written and never read; the out
  parameter `are_bits` (`second_pass.c:127,278`), which is never read; the
  local `has_errors` in `build_memory_image` (`memory_builder.c:117,179`),
  which is never set; the counter `count` (`memory_builder.c:291,331`).
- **R1-18** `first_pass` returns `void` (`first_pass.h:9`) and throws its
  `has_errors` flag away (`first_pass.c:80,252`). `process_single_file`
  (`assembler.c:140`) therefore continues into the next passes after
  first-pass errors, and no interface exists to stop it. The three
  `encode_*_directive` functions and `encode_instruction_first_pass` also
  return `void`, so no encoding error can be reported. This is a structural
  cause of output files being written for erroneous programs.
- **R1-19** No module owns directive names or register names. The strings
  `.data`, `.string`, `.mat`, `.entry` and `.extern` are matched with `strstr`
  anywhere in the line in `first_pass.c:138,152,176,184,192`,
  `line_analysis.c:79-81,142,249`, `memory_builder.c:143,149,155,161,207,245`
  and `second_pass.c:70,77-78`. A label or a string that contains such text is
  misclassified; this is a structural cause of bugs.

## Rule 2: information hiding

- **R2-01** `MemoryImage` is defined in a header (`types.h:47-56`). It is
  reached into by `output_writer.c:77,78,105,108,111,134,136,137,141` and it
  is allocated on the stack by `assembler.c:97`, which needs the complete
  type.
- **R2-02** `Symbol` is defined in a header (`types.h:59-66`) and the list
  head is exported (`symbol_table.h:8`). Other modules walk the list and use
  the fields: `first_pass.c:55-59` (`symbol_table_head`, `type`, `address`,
  `next`), `first_pass.c:124,129`, `second_pass.c:265,273,274,315,324`. The
  callers know that the table is a linked list, which the rule forbids by
  name.
- **R2-03** `InstructionDef` is defined in a header (`types.h:69-74`).
  `find_instruction` returns a non-const pointer into the table
  (`instruction_table.c:29,40`), and callers read the fields:
  `instruction_validation.c:88,91,96,102,107,113-114,124,126,128,130,184,187,
  197,200,205,212,231,242,247,252,258,300,303`.
- **R2-04** `GenericNode`, `GenericTable` and `MacroData` are defined in a
  header (`preassembler.h:17-36`). `preassembler.c:385-392` and `417-435` cast
  `macro->data` to `MacroData *` and read `content` and `line_count`.
- **R2-05** `ExtRef` and `EntryPoint` are defined in a header
  (`types.h:77-90`). `output_writer.c:209-224,271-286,329-356` walks the lists
  and reads `symbol_name`, `address` and `next`.
- **R2-06** `SecondPassContext` is defined in `second_pass.h:9-16` although
  only `second_pass.c` uses it. Seven helper functions that take it are
  exported for the same reason (`second_pass.h:20-30`).
- **R2-07** `MachineWord` (`types.h:41-44`) is a public struct with a bit
  field, and callers assemble words by hand with shifts:
  `memory_builder.c:228,269,277,328,455,464,472,499,503,511`,
  `second_pass.c:318,324`, `output_writer.c:108,111,137,141`. The word layout
  is therefore known in three modules.
- **R2-08** `has_label` casts away `const` and trims its argument in place
  (`macro_and_label_func.c:374`). The caller's line buffer loses its trailing
  newline as a side effect, which is why `preassembler.c:354-367`, `446-450`
  and `462-466` re-append a newline. A predicate that changes its input is a
  structural cause of bugs.

## Rule 3: instruction table

- **R3-01** There is one table (`instruction_table.c:8-24`), but it carries
  only the name, the opcode and the operand count. The legal source and
  destination addressing modes are missing.
- **R3-02** The table is not `const` (`static InstructionDef instructions[]`,
  `instruction_table.c:8`), and `find_instruction` hands out a writable
  pointer to its rows. The row count is the hard-coded
  `#define INSTRUCTION_COUNT 16` (`instruction_table.c:26`) instead of being
  derived from the array.
- **R3-03** `get_opcode_value` (`memory_builder.c:519-554`) repeats all
  sixteen opcodes as a `strcmp` chain, although the caller already holds the
  table row (`memory_builder.c:370-374`).
- **R3-04** `is_type_allowed_for_instruction`
  (`instruction_validation.c:133-174`) encodes the legal addressing modes as a
  `strcmp` chain over the mnemonic names. Because the knowledge is not in the
  table it has drifted from the spec: lines 166-169 allow only direct and
  matrix for `jmp`, `bne` and `jsr`, while SPEC section 3 lists modes 1, 2 and
  3, and lines 143-147 forbid a matrix destination for `lea`, which SPEC
  allows. This is reported to the behaviour audit as a likely bug.
- **R3-05** `validate_operands_types` selects its message with further
  `strcmp` calls on mnemonic names (`instruction_validation.c:197,231,242`).
- **R3-06** The list of mnemonics is repeated in `is_command`
  (`line_analysis.c:92-98`) and in `is_reserved_word`
  (`macro_and_label_func.c:13-15`).
- **R3-07** `count_command_words` compares against `"stop"` and `"rts"`
  (`line_analysis.c:292`) instead of asking the table for the operand count.
- **R3-08** `validate_operands_count` is an `if` chain over operand counts 0,
  1 and 2 with an "internal error" branch (`instruction_validation.c:88-115`).
- **R3-09** Addressing modes are classified in four places that do not agree:
  `classify_operand` with a private `OperandType` enum
  (`instruction_validation.c:11-18,36-81`), `get_addressing_method`
  (`memory_builder.c:557-578`), the `is_register`, `is_matrix`,
  `is_immediate` and `is_label_operand` predicates
  (`line_analysis.c:176-239`), and inline tests in
  `second_pass.c:191-198,206,220,345-360` and
  `memory_builder.c:436,443,451,481,488,496`. Three sets of constants name the
  modes: `types.h:17-21`, `types.h:24-30` and `instruction_validation.c:11-18`.
  `get_addressing_method` treats any operand that starts with `r` and a digit
  as a register (`memory_builder.c:567`), while `is_register` requires exactly
  two characters and a digit from 0 to 7; a label such as `r1x` gets a
  different mode in each place. This is a structural cause of bugs.
- **R3-10** Register names are tested with literals in
  `line_analysis.c:178-179`, `instruction_validation.c:69,71`,
  `memory_builder.c:567,655,668` and listed again in
  `macro_and_label_func.c:22-23`. Directive names are listed in
  `macro_and_label_func.c:18-19` and matched as literals elsewhere (R1-19).

## Rule 4: global variables

- **R4-01** `Symbol *symbol_table_head = NULL;` (`symbol_table.c:13`) is a
  non-const object at file scope with external linkage. It is the only
  `extern` declaration in the project (`symbol_table.h:8`) and is used
  directly from `first_pass.c:56`. It is never reset between input files
  (`free_symbol_table` is never called), which was verified by the run quoted
  above.
- **R4-02** `static InstructionDef instructions[]`
  (`instruction_table.c:8`) is a non-const array at file scope. IMP-1 requires
  `static const`.
- **R4-03** `static const char *ops[]` (`line_analysis.c:92-98`) is a static
  array with program lifetime inside `is_command`. The array itself is not
  const (only the strings are), and it is a second copy of the instruction
  names. The four arrays in `is_reserved_word`
  (`macro_and_label_func.c:13-27`) and `base4_chars` (`second_pass.c:412`) are
  automatic and rebuilt on every call; they are not globals but belong in
  const tables.
- **R4-04** Hidden global state through `strtok`:
  `word_extractor.c:21,56,63,102,105`, `line_analysis.c:149,158`,
  `memory_builder.c:200,203,209,215,232,315,333,360,363,377,390`,
  `second_pass.c:135,138,159,173,246,247`. `memory_builder.c:215` restarts
  `strtok` on a token that an earlier `strtok` call produced
  (`memory_builder.c:209`), so the rest of the line is lost when a blank
  follows a comma. This is the cause of the `.data 6, -9, 15` result above.
- No other object at file scope and no other `extern` was found.

## Rule 5: functions

- **R5-01** Forty-one functions have external linkage but are used only in
  their own file. They should be `static`.
  - `assembler.c`: `process_files` (60), `process_single_file` (92).
  - `first_pass.c`: `error` (27).
  - `instruction_validation.c`: `is_lowercase_only` (22).
  - `macro_and_label_func.c`: `create_generic_table` (75),
    `free_generic_table` (90), `add_to_generic_table` (123),
    `find_in_generic_table` (164), `cleanup_macro_data` (189),
    `extract_until_char` (301), `get_content_after_char` (330),
    `is_valid_name` (382).
  - `memory_builder.c`: `init_memory_image` (27), `add_data_word` (53),
    `add_instruction_word` (75), `encode_data_directive` (190),
    `encode_string_directive` (238), `encode_matrix_directive` (284),
    `encode_instruction_first_pass` (340), `get_opcode_value` (519),
    `get_addressing_method` (557), `encode_first_word` (580),
    `encode_immediate_operand` (598), `encode_register_operand` (613),
    `extract_matrix_registers` (636).
  - `output_writer.c`: `write_object_file` (43), `write_entries_file` (174),
    `write_externals_file` (236), `get_base_filename` (298), `count_entries`
    (329), `count_externals` (344).
  - `preassembler.c`: `cleanup_macro_lines` (4).
  - `second_pass.c`: `process_instruction_second_pass` (117),
    `process_entry_directive` (237), `encode_operand` (278),
    `get_instruction_length` (331), `add_external_reference` (372),
    `add_entry_point` (390), `free_ext_list` (441), `free_entry_list` (453).
  - `symbol_table.c`: `add_symbol` (21).
  The nine never-called functions of R1-14 are also non-static.
- **R5-02** Twenty-five functions are longer than 40 lines (counted from the
  signature to the closing brace):

  | Function | File and lines | Length |
  |---|---|---|
  | `preassembler` | `preassembler.c:20-497` | 478 |
  | `first_pass` | `first_pass.c:74-262` | 189 |
  | `encode_instruction_first_pass` | `memory_builder.c:340-516` | 177 |
  | `write_object_file` | `output_writer.c:43-171` | 129 |
  | `process_instruction_second_pass` | `second_pass.c:117-235` | 119 |
  | `validate_operands_types` | `instruction_validation.c:180-266` | 87 |
  | `process_single_file` | `assembler.c:92-175` | 84 |
  | `second_pass` | `second_pass.c:31-114` | 84 |
  | `build_memory_image` | `memory_builder.c:110-187` | 78 |
  | `count_command_words` | `line_analysis.c:279-343` | 65 |
  | `is_reserved_word` | `macro_and_label_func.c:10-73` | 64 |
  | `print_error` | `error_handling.c:5-67` | 63 |
  | `write_entries_file` | `output_writer.c:174-233` | 60 |
  | `write_externals_file` | `output_writer.c:236-295` | 60 |
  | `is_type_allowed_for_instruction` | `instruction_validation.c:119-177` | 59 |
  | `add_symbol_to_table` | `symbol_table.c:46-101` | 56 |
  | `encode_matrix_directive` | `memory_builder.c:284-336` | 53 |
  | `encode_operand` | `second_pass.c:278-328` | 51 |
  | `is_command` | `line_analysis.c:86-135` | 50 |
  | `classify_operand` | `instruction_validation.c:36-81` | 46 |
  | `encode_data_directive` | `memory_builder.c:190-235` | 46 |
  | `validate_command_line` | `instruction_validation.c:270-314` | 45 |
  | `encode_string_directive` | `memory_builder.c:238-281` | 44 |
  | `add_macro` | `macro_and_label_func.c:225-266` | 42 |
  | `get_second_word` | `word_extractor.c:39-79` | 41 |

- **R5-03** Functions that do several things:
  - `preassembler` (`preassembler.c:20`): opens both files, creates two
    tables, checks line length, parses macro definitions, validates macro
    names, validates labels, stores macro bodies, expands macro calls, writes
    the `.am` file, cleans up and removes the `.am` file on error.
  - `first_pass` (`first_pass.c:74`): reads the file, handles labels,
    dispatches directives, counts words, checks memory overflow, prints debug
    text and dumps the symbol table.
  - `add_symbol_to_table` (`symbol_table.c:46`): inserts, marks entries,
    handles a second `.extern`, patches the address of an earlier `.entry`
    placeholder, detects duplicates and prints five different diagnostics.
    Its `is_entry` flag parameter selects between two unrelated behaviours.
  - `encode_instruction_first_pass` (`memory_builder.c:340`): tokenises the
    line, classifies both operands, encodes and stores up to five words.
  - `process_single_file` (`assembler.c:92`): builds file names, runs four
    phases, prints progress and frees, with the same two `free` calls repeated
    on four exits.
  - `second_pass` (`second_pass.c:31`): runs the pass, starts output writing
    and frees the lists.
  - `write_object_file` (`output_writer.c:43`): builds the file name, converts
    the header, writes the code image, writes the data image and prints debug
    text.
  - `is_line_too_long` (`line_parser.c:6`): a predicate that also prints the
    error message.
  - `has_label` (`macro_and_label_func.c:367`): a predicate that also modifies
    its argument (R2-08).
- **R5-04** Unused parameters: `address` in `add_data_word` and
  `add_instruction_word` is used only in the overflow message
  (`memory_builder.c:53,75`); `line_number` in
  `is_type_allowed_for_instruction` (`instruction_validation.c:122`), in the
  three `encode_*_directive` functions (`memory_builder.c:190,238,284`) and in
  `encode_instruction_first_pass` (`memory_builder.c:355`); `instruction` in
  `get_instruction_length` (`second_pass.c:331`).

## Rule 6: magic numbers

Each item is one meaning; the literals are listed by location.

- **R6-01** Word width and word mask: `10` in the bit field (`types.h:43`,
  although `WORD_SIZE` exists and is unused); `(1 << 10)` in
  `memory_builder.c:225,325,605` and `second_pass.c:422`; `0x3FF` in
  `memory_builder.c:228,328,608`, `output_writer.c:111,141` and
  `second_pass.c:430`.
- **R6-02** Field positions and field masks of a machine word: `<< 6`, `<< 4`,
  `<< 2`, `0xF`, `0x3` in `memory_builder.c:464,585,588,591,619,622`; `0xFF`
  and `<< 2` in `second_pass.c:324`.
- **R6-03** Memory size and load address as literals although `MEMORY_SIZE`
  and `BASE_ADDRESS` exist: `256` in `types.h:49-51` and
  `first_pass.c:181,189,197` (and in the message texts at 182, 190, 198);
  `255` in `first_pass.c:236`; `100` in `first_pass.c:77`.
- **R6-04** Buffer sizes and the matching `scanf` widths: `256` in
  `line_analysis.c:141,206` and `instruction_validation.c:275`; `32` and `64`
  in `instruction_validation.c:274,276`; `16` in `line_analysis.c:88`; `10`
  in `types.h:71`; `"%31s %255[^\n]"` in `instruction_validation.c:278`;
  `%63s` in `line_analysis.c:200,209,210`; `%15s` in `line_analysis.c:124`;
  `%30s` in `first_pass.c:140,154`; `30` in `symbol_table.c:28,29` (which
  should be `MAX_LABEL_LENGTH - 1`).
- **R6-05** Limits with an unclear meaning: `MAX_LINE_LENGTH 82` and
  `MAX_LABEL_LENGTH 31` (`Assembler.h:35-36`) are named, but the names do not
  say that they are buffer sizes that include the newline and the terminator
  rather than the limits of 80 and 30 characters. The off-by-one difference in
  R1-13 comes from this.
- **R6-06** Opcodes `0` to `15` repeated outside the table
  (`memory_builder.c:521-552`) and the table size `16`
  (`instruction_table.c:26`).
- **R6-07** Operand counts `0`, `1`, `2` as literals:
  `instruction_validation.c:88,91,102,126,128,130,184,187,212`,
  `line_analysis.c:299,310`, `memory_builder.c:406,410,423,434,479`,
  `second_pass.c:189,206,220`.
- **R6-08** Words per operand `1` and `2`: `line_analysis.c:284,303,307,317,
  324,328,333,337`, `second_pass.c:192,194,196,198,333,348,366`.
- **R6-09** Base-4 output: field widths `5` and `4` in
  `output_writer.c:77,78,109,111,140,141,212,274`; the base `4` in
  `second_pass.c:434,435`; the digit letters in `second_pass.c:412`; the
  padding letter `'a'` in `output_writer.c:98,100`.
- **R6-10** Output-file extensions as literals although `OB_EXTENSION`,
  `ENT_EXTENSION` and `EXT_EXTENSION` exist: `output_writer.c:63,196,258`; the
  matching length arithmetic `4 + 1` and `6` in `output_writer.c:57,190,252`.
- **R6-11** Macro storage: the initial capacity `10` (`preassembler.c:167`),
  the growth factor `2` (`preassembler.c:325`), `len + 2`
  (`preassembler.c:356`), and the expected word counts `2`, `1`, `1` passed to
  `has_extraneous_text_after_words` (`preassembler.c:127,183,378`).
- **R6-12** Register syntax: the length `2`, the letter `'r'` and the range
  `'0'` to `'7'` in `line_analysis.c:178-179`,
  `instruction_validation.c:69,71`, `memory_builder.c:567,655,668`; the mask
  `0xF` for a register number in `memory_builder.c:464,619,622`.
- **R6-13** Character ranges `32`, `126` and `127`:
  `line_analysis.c:260,271`, `memory_builder.c:255,263`.
- **R6-14** Process exit status `0` and `1`: `assembler.c:37,46,50`,
  `preassembler.h:108`, `symbol_table.c:25`. Note that `assembler.c:37`
  returns 0 when no argument is given.
- **R6-15** Syntax characters and keywords as scattered literals: `':'`,
  `','`, `'#'`, `'['`, `']'`, `';'`, `'.'`, the token separators
  `" \t\n\r"` (`word_extractor.c:21,56,63,102,105`) and `" \t"`
  (`memory_builder.c`, `second_pass.c`), and the directive names (R1-19).
- **R6-16** `int` values `0` and `1` used as truth values although `Boolean`
  exists: `first_pass.c:80,94,118,126,131,142,146,156,160,171,183,191,199,
  208,228,238,242`, the return values of `symbol_table.c:46-100`,
  `instruction_validation.c` (all validation functions), `line_analysis.c:20,
  23`, and the field `is_entry` (`types.h:64`).

## Rule 7: error reporting

- **R7-01** There are two error functions, `print_error`
  (`error_handling.c:5`) and `error` (`first_pass.c:27`). `print_error` is
  used by `assembler.c` (3 calls), `line_parser.c` (1) and `preassembler.c`
  (24, most through the macros of `preassembler.h`). `error` is used only by
  `first_pass.c` (5 calls).
- **R7-02** Neither function can produce the format the spec fixes. SPEC
  ERR-R2 and ERR-R13 require every error on standard output in the form
  `<name>.as:<line>: error: <text>`. Both functions write to `stderr`, neither
  takes a file name, and the texts use four different prefixes: "Error on line
  N:", "Error line N:", "Error: ... at line N" and "... (line N)".
- **R7-03** Diagnostics printed directly, outside both functions (lines of
  `printf`, `fprintf(stderr, ...)` and `perror`):
  - `instruction_validation.c:95,106,113,192,199,204,218,224,235,239,246,251,
    257,280,286,294,302` (17, all to `stdout`).
  - `symbol_table.c:24,51,59,77,84,93` (6, to `stderr`).
  - `second_pass.c:49,144,153,251,260,267,309,377,395` (9, to `stdout`).
  - `output_writer.c:15,24,34,69,85,115,144,202,215,264,277` (11).
  - `memory_builder.c:60,82,98,125,181` (5).
  - `first_pass.c:84` (`perror`), `182,190,198,237` (5).
  - `assembler.c:35,36,45,72,130,147,159` (7).
- **R7-04** Errors are split between two streams. `print_error`, `error` and
  `symbol_table.c` write to `stderr`; every other diagnostic goes to `stdout`.
  One input file can therefore report half of its errors on each stream.
- **R7-05** Progress and debug output on `stdout`, mixed with the diagnostics:
  `assembler.c:49,67,124,135,137,142,152,154,164,173`;
  `first_pass.c:88,90,105,110,116,167,178,180,186,188,194,196,204,225,235,
  251,253,255,257,258` and the symbol dump (`first_pass.c:261`,
  `symbol_table.c:115,120`); `line_analysis.c:275`;
  `memory_builder.c:129,185`; `second_pass.c:53,91,99,112`;
  `output_writer.c:107,137,163,231,293`. `output_writer.c:107` prints a line
  without a newline.
- **R7-06** Error state is not carried by the error module. Each module keeps
  its own `has_errors` flag (`preassembler.c:38`, `first_pass.c:80`,
  `memory_builder.c:117`, `second_pass.h:14`), two of which are discarded
  (R1-18). The macros in `preassembler.h:98-146` assign a caller's local
  variable named `has_errors` and contain `continue`, `break` and `exit`.
- **R7-07** Fatal paths call `exit(1)` from library-level code:
  `preassembler.h:108` (used at `preassembler.c:63,69,78,87,194,209,305`) and
  `symbol_table.c:25`. The remaining input files are never processed, and
  nothing is released.

## Rule 8: memory

- **R8-01** Fixed-size arrays that are not the machine-code image:
  - Line buffers of `MAX_LINE_LENGTH`: `preassembler.c:34`, `first_pass.c:75`,
    `memory_builder.c:113,192,286,342,345,346,638`,
    `second_pass.c:34,119,122,123,239`, `line_analysis.c:281-283`.
  - Name buffers of `MAX_LABEL_LENGTH`: `first_pass.c:93,139,153`,
    `preassembler.c:41`, `second_pass.c:285`, and the name fields inside the
    list nodes `types.h:61,79,87`.
  - Ad hoc buffers: `line_analysis.c:88,141,206`,
    `instruction_validation.c:274-276`, `types.h:71`.
  - `MemoryImage` holds three arrays of 256 (`types.h:49-51`); `data_memory`
    is unused and `data_image` is a second fixed image.
  The symbol table and the macro table themselves are linked lists and are
  dynamically allocated, as the rule demands. A bounded line buffer is
  defensible because the spec limits a source line to 80 characters; if it is
  kept, the reason must be recorded as the rule on adapting the rules asks.
- **R8-02** `current_macro_name[MAX_LABEL_LENGTH]` (`preassembler.c:41`) is
  filled with `strcpy` (`preassembler.c:157`) from a name that
  `is_valid_name` accepts up to 31 characters long
  (`macro_and_label_func.c:386`). A 31-character macro name writes 32 bytes
  into a 31-byte buffer.
- **R8-03** Lines in the `.am` file can be longer than the buffers that read
  them. `preassembler.c:423` writes `label: ` in front of the first line of a
  macro body, and the passes read with `fgets` into 82 bytes with no check
  (`first_pass.c:92`, `memory_builder.c:132`, `second_pass.c:56`); a long
  line is silently split in two.
- **R8-04** Invalid free and leak. `extract_until_char` returns
  `trim_whitespace(result)` (`macro_and_label_func.c:319`), which is a pointer
  inside the allocated block when the text starts with white space. The
  callers free it at `preassembler.c:258,267,276,285,296,310,454`. Verified:
  `free(): invalid pointer` and abort for a label after leading blanks;
  valgrind reports "Invalid free" and "5 bytes definitely lost".
- **R8-05** Use after free: `output_writer.c:229` frees `ent_filename` and
  line 231 prints it; `output_writer.c:291` frees `ext_filename` and line 293
  prints it. Reported by gcc and by valgrind.
- **R8-06** The symbol table is never freed. `free_symbol_table` is never
  called, on any path, and the global head keeps the symbols of the previous
  input file. This is both a leak and a wrong result for every file after the
  first.
- **R8-07** `first_pass` never closes its input file (`first_pass.c:82`; there
  is no `fclose` in the function). valgrind shows 472 bytes still reachable
  per input file.
- **R8-08** Unchecked allocations:
  - `preassembler.c:24,26`: both results of
    `create_filename_with_extension` are passed to `fopen` before any check;
    a failed allocation passes `NULL` to `fopen`.
  - `preassembler.c:168`: `macro_lines = malloc(...)` is never checked and is
    written through at `preassembler.c:340`.
  - `preassembler.c:191`: `malloc(macro_line_count * sizeof(char *))` with a
    count of zero (an empty macro) may return `NULL`, which the code treats as
    a fatal allocation failure.
  - `preassembler.c:356`: `p = realloc(p, ...)` loses the block when it fails,
    and the cleanup that follows does not cover that element.
  - `preassembler.c:405-406,423`: `line_label` is printed with `%s` without a
    check.
  - `word_extractor.c:15,29,50,71,96`: an allocation failure returns `NULL`
    or `FALSE`, which the callers read as "no word" or "no extra text"
    (`preassembler.c:112,121,127,183,252,378`); the failure is silent.
  No `calloc` and no `strdup` call exists in the code. `strdup` would not be
  C90; the code copies strings with `malloc` and `strcpy` instead, eleven
  times (`file_utils.c:28`, `macro_and_label_func.c:146,247,311,342`,
  `preassembler.c:200,340`, `word_extractor.c:14,28,49,70,95`), which calls
  for one checked helper.
- **R8-09** Leaks and crashes on error paths:
  - `add_macro` (`macro_and_label_func.c:265`) leaks `macro_data` and its
    copied lines when `add_to_generic_table` fails.
  - `preassembler.c:327-335,341-349,357-365`: after an allocation failure
    `macro_lines` is set to `NULL` while `inside_macro` stays true, so the
    next line dereferences `NULL` at `preassembler.c:340`.
  - Every `REPORT_CRITICAL_ERROR_AND_EXIT` (`preassembler.c:63,69,78,87,194,
    209,305`) exits with open files, live tables and a partial `.am` file on
    disk. `preassembler.c:63` exits after the output file was already created
    at line 27.
  - `symbol_table.c:25` exits without releasing anything.
  - `assembler.c:145-163`: the error returns skip `free_memory_image`.
    `free_memory_image` frees nothing (`memory_builder.c:627-635`), so this is
    harmless today, but the name promises otherwise.
  - `second_pass.c:106`: the result of `generate_output_files` is ignored, so
    a failed write is reported as success.
- **R8-10** The loop over input files (`assembler.c:65-74`) keeps state
  between files: the symbol table (R8-06) and the open file of the first pass
  (R8-07). Output files of an earlier run are not removed when the current
  run fails; only the `.am` file is removed, and only by the pre-assembler
  (`preassembler.c:489-492`).

## Rule 9: documentation

- **R9-01** Eleven files have no header comment at all: `error_handling.c`,
  `file_utils.c`, `line_parser.c`, `word_extractor.c`, `line_analysis.c`,
  `macro_and_label_func.c`, `preassembler.c`, `output_writer.c`,
  `Assembler.h`, `line_analysis.h`, `preassembler.h`.
- **R9-02** Ten files have a one-line title that does not describe the
  module's role: `instruction_table.c:1`, `instruction_validation.c:1`,
  `types.h:1`, `first_pass.h:1`, `instruction_table.h:1`,
  `instruction_validation.h:1`, `memory_builder.h:1`, `output_writer.h:1`,
  `second_pass.h:1`, `symbol_table.h:1`. Only `assembler.c`, `first_pass.c`,
  `memory_builder.c`, `second_pass.c` and `symbol_table.c` have a real header
  comment.
- **R9-03** Only 12 of the 107 functions have a comment that states purpose,
  parameters and return value: `main`, `process_files`,
  `process_single_file` (`assembler.c`), `error`,
  `adjust_data_addresses_with_icf`, `first_pass` (`first_pass.c`),
  `init_memory_image`, `add_data_word`, `add_instruction_word`
  (`memory_builder.c`), `second_pass` (`second_pass.c`), `add_symbol`,
  `add_symbol_to_table` (`symbol_table.c`).
- **R9-04** Functions with no comment at all: every function of
  `line_analysis.c` except `parse_operands` (18 functions, lines 10-393);
  `find_symbol`, `print_symbol_table`, `free_symbol_table`
  (`symbol_table.c:103,113,127`); `update_instruction_word`,
  `free_memory_image`, `extract_matrix_registers`
  (`memory_builder.c:90,627,636`); `cleanup_macro_lines`, `preassembler`
  (`preassembler.c:4,20`); `get_second_word` (`word_extractor.c:39`);
  `create_generic_table` (`macro_and_label_func.c:75`). No prototype in any
  header is documented.
- **R9-05** All remaining functions (about 67) have a one-line title that
  repeats the function name and gives no parameters and no return value, for
  example `output_writer.c:6,42,173,235,297,328,343`,
  `second_pass.c:116,236,277,330,371,389,407,440,452`,
  `memory_builder.c:109,189,237,283,518,556,579,597,612`,
  `macro_and_label_func.c:3,9,89,122,163`.
- **R9-06** Comments that are not in English (Hebrew):
  `line_analysis.c:248,254,259,267,269`,
  `memory_builder.c:244,249,254,261,266,274,645,648,654,658`,
  `second_pass.c:314`.
- **R9-07** Comments that are wrong, stale or duplicated:
  - Wrong: `memory_builder.c:47-52` and `68-73` say that the word is stored
    at `address`, but the address is ignored; `memory_builder.c:465` says
    "temporary encoding for r2,r7"; `memory_builder.c:490` and `510` describe
    the wrong operand; `symbol_table.h:7` says "single global variable" as if
    it were a design goal; `output_writer.c:57,190,252` count the extension
    lengths wrongly.
  - History notes instead of descriptions: `types.h:31,51,54,55`
    ("added for compatibility", "added this"), `preassembler.c:112,375`
    ("Fixed: removed *"), `second_pass.h:32-33`, `memory_builder.c:355`.
  - Duplicated lines: `Assembler.h:42-43`, `assembler.c:59,91`,
    `memory_builder.c:26,338-339,459-460`, `second_pass.c:30,83-84,204-205`,
    `macro_and_label_func.c:328-329`.
  - Commented-out code: `first_pass.c:111-112`, `preassembler.c:172`,
    `second_pass.h:33`.
- **R9-08** Broken indentation hides the control flow:
  `first_pass.c:178-199`, `memory_builder.c:451-475,488-506`,
  `second_pass.c:85-92,189-230`, `preassembler.c:292-301`,
  `word_extractor.c:90-93`.

## Rule 10: C90 conformance

- `//` comments: no violation found.
- Declarations after statements: no violation found (`-pedantic` reports
  none).
- Include guards: no violation found; all eleven headers have one.
- Non-standard library functions: no violation found (no `strdup`, no
  `snprintf`, no POSIX call).
- **R10-01** Exact compiler output of `gcc -Wall -ansi -pedantic` (gcc
  13.3.0), two warnings, so the zero-warning rule is broken:
  `output_writer.c:231:5: warning: pointer 'ent_filename' used after 'free'
  [-Wuse-after-free]` and `output_writer.c:293:5: warning: pointer
  'ext_filename' used after 'free' [-Wuse-after-free]`.
- **R10-02** The include of `"assembler.h"` does not match the file name
  `Assembler.h` (R1-01); the program compiles only on a case-insensitive file
  system.
- **R10-03** Seven files do not end with a newline character, which C90
  requires of a source file: `Assembler.h`, `line_analysis.h`,
  `preassembler.h`, `assembler.c`, `file_utils.c`, `preassembler.c`,
  `second_pass.c`.
- **R10-04** The `<ctype.h>` functions are called with a plain `char`, which
  is undefined for negative values. The input can contain bytes above 127 (the
  repository's own `formal_tester.as` does): `line_analysis.c:31,35,152,353,
  367,380,387`, `macro_and_label_func.c:392,400`,
  `memory_builder.c:218,307,318,567,655,668`, `second_pass.c:163,168,176,180`.
- **R10-05** Multi-statement macros that are not wrapped in
  `do { } while (0)`, that contain `continue`, `break` and `exit`, and that
  use a variable of the caller by name: `preassembler.h:98-146`.
- **R10-06** Old-style definitions with empty parentheses against prototypes
  with `(void)`: `symbol_table.c:113,127`.
- **R10-07** Unbounded `sscanf` conversions `"%s %[^\n]"`
  (`line_analysis.c:287`); the buffers happen to be as large as the input
  line, but nothing enforces it.
- **R10-08** Further warnings shown by `-Wextra` (not required by the rules,
  listed for the clean-up): signed against unsigned comparisons at
  `macro_and_label_func.c:37,46,55,64,398`; unused parameters at
  `instruction_validation.c:122`, `memory_builder.c:190,238,284`,
  `second_pass.c:331`. Pointer differences are stored in `int` without a
  cast at `line_analysis.c:15,274`, `macro_and_label_func.c:310`,
  `output_writer.c:311`.
- **R10-09** Mixed line endings: 13 source files and 9 headers use CRLF;
  `error_handling.c`, `line_parser.c`, `types.h` and
  `instruction_validation.h` use LF.

## Rule 11: Makefile

- **R11-01** `HEADERS` lists `assembler.h` (`Makefile:29`), which does not
  exist, so nothing builds: `make: *** No rule to make target 'assembler.h',
  needed by 'assembler.o'.  Stop.`
- **R11-02** Header dependencies are wrong in two ways at once. The pattern
  rule (`Makefile:49`) makes every object depend on every header, and the
  explicit list (`Makefile:53-64`) is incomplete and partly false: there is no
  line for `line_parser.o`, `macro_and_label_func.o` or `word_extractor.o`;
  `assembler.o` omits `second_pass.h`, `memory_builder.h` and
  `output_writer.h`; `second_pass.o` omits `line_analysis.h`,
  `instruction_table.h` and `output_writer.h`; `output_writer.o` omits
  `memory_builder.h`, `line_analysis.h` and `instruction_table.h`;
  `file_utils.o` and `error_handling.o` name `types.h`, which they do not
  include.
- **R11-03** `clean` (`Makefile:67-68`) removes only the objects and the
  binary. `CLAUDE.md` says it also removes generated outputs (`*.am`, `*.ob`,
  `*.ent`, `*.ext` in the root, and `tests/out/`).
- **R11-04** `clean-tests` (`Makefile:101-109`) deletes every `*.am`, `*.ob`,
  `*.ent` and `*.ext` under `tests/`. The current test suite tracks 199 such
  files as expected outputs; running this target, or `clean-all`
  (`Makefile:262`), destroys them.
- **R11-05** Stale targets that refer to files that do not exist
  (`test_runner.sh`, `test_utils.sh`, `Makefile:96-97`): `setup-tests` (113),
  `test-basic` (127), `test-memory` (140), `test-errors` (154),
  `test-comprehensive` (167), `smoke-test` (184), `validate-tests` (208),
  `test-report` (219), `benchmark` (229), `test-memory-check` (237). Their
  fallback branches create and delete files in the repository root and always
  succeed (`|| true`).
- **R11-06** Targets that do not belong in this project: `install` and
  `uninstall` into `/usr/local/bin` (`Makefile:74-79`), `create-samples`
  (193), `static-analysis` (252), `debug` and `release` (82-87), which define
  `DEBUG` and `NDEBUG` although the code tests neither, and two help targets
  with stale text (266-296).
- **R11-07** Flags: `CFLAGS = -ansi -pedantic -Wall -g` (`Makefile:3`)
  contains the three required flags; `-g` is added by default and again by
  `debug`. `.PHONY` is declared twice (`Makefile:100-266` individually and
  `299-302`). The file contains non-ASCII check marks at lines 187 and 189.
- The targets `all` (42), `test` (122) and `clean` (67) exist; `test` calls
  `tests/run_tests.sh` correctly.

## Rule 12: repository hygiene

- **R12-01** Two empty stray files are tracked: `This` and
  `instruction_count` (0 bytes each). `README.md:51` calls the second one an
  "instruction counting utility".
- **R12-02** `formal_tester.as` and `formal_tester.ob` are tracked in the
  root. The `.ob` file is a generated output that `.gitignore:23` explicitly
  re-includes. The `.as` file contains typographic quotation marks (U+201C,
  U+201D) on line 13 instead of ASCII quotes and has no final newline; the
  Hebrew comments in the `.string` code (R9-06) show that the code was bent
  to accept this file. The program looks like the worked example of the
  course booklet, so the owner should decide whether it may be published; the
  test suite under `tests/` supersedes it either way.
- **R12-03** `tests/sh/` holds nine obsolete scripts
  (`correct_macro_test.sh`, `create_test_files.sh`, `debug_commands.sh`,
  `debug_helper.sh`, `fixed_debug_helper.sh`, `memory_check_fix.sh`,
  `memory_stress_tester.sh`, `run_all.sh`, `run_tests.sh`). None is
  executable, they call each other by names that do not exist in that
  directory, they are full of emoji, and `tests/run_tests.sh` replaces them.
- **R12-04** `README.md` is out of date and unsuitable for a portfolio:
  line 1 is a non-English religious abbreviation; lines 5, 9, 41, 78-82, 96
  and 154-165 describe a "95% complete" project with a known encoding bug;
  emoji are used in every heading; lines 17, 46, 49, 51, 56 and 57 list files
  that do not exist or are not tracked (`assembler.h`, the binary,
  `formal_tester.am`, `.vscode/`, `.git/`); line 113 shows the wrong usage
  (`./assembler formal_tester.as`; names are given without the extension);
  line 128 names a `SymbolTable` type that does not exist.
- **R12-05** Personal details beyond the owner's name: an e-mail address
  (`README.md:172`) and a user handle (`README.md:171`). `README.md:174`
  calls the work "an academic project in development".
- **R12-06** `CHANGELOG.md` makes claims that are false today: "Zero warnings
  compilation" (line 17), "Memory safe - no leaks, proper cleanup" (line 18),
  "Modular design - clear separation of concerns" (line 19). It uses emoji and
  placeholder dates ("2025-08-XX", lines 40-41).
- **R12-07** Non-English text in tracked files: the Hebrew comments of R9-06,
  and two Hebrew file names in `docs/PROGRESS.md:39-40`. The git history also
  contains Hebrew commit messages (for example `5181a2c`, `c17c645`,
  `9188df6`) and the owner's e-mail address as author; rewriting history is
  the owner's decision.
- **R12-08** Course-identifying details: `CLAUDE.md` names the course number,
  the assignment number and the university, and gives the URLs of two other
  students' repositories. `CLAUDE.md` is tracked and would be published with
  the repository; the owner should decide whether that is intended.
- **R12-09** `.gitignore` is in English and ignores the course materials, the
  binary and the generated outputs. Its only defect is the exception
  `!/formal_tester.ob` (line 23). No committed binary, object file or course
  material was found among the tracked files (`git ls-files`), and
  `tests/out/` is ignored.

---

## Proposed target structure

### Principles

1. Parse each line once. One syntax module turns a source line into a parsed
   statement; both passes consume that statement. This removes the three
   parsers and the three length calculations of R1-07.
2. Two passes, not three. The first pass encodes everything that does not
   need a symbol address and stores the data image; the second pass fills in
   the symbol words. `memory_builder.c` disappears as a pass and its encoding
   helpers move to an encoder module that both passes call.
3. One owner per data structure, opaque outside its module.
4. One instruction table with the legal modes, and one error function that
   knows the file name and the line number.
5. The driver (`assembler.c`) owns the lifetime of everything that belongs to
   one input file and frees it on every path.

### Modules

Dependencies are listed as "may include". `common.h` may be included by all.

| Module (file pair) | Single responsibility | Opaque types it owns | May depend on |
|---|---|---|---|
| `common.h` (header only) | Shared definitions: `Boolean`, limits, addresses, the `AddressingMode` enum, file extensions. No structs. | none | nothing |
| `errors.c/.h` | Print every diagnostic in the one fixed format and count errors per file. | `ErrorContext` | `common.h` |
| `util.c/.h` | Small string and file helpers: checked string copy, skip and trim white space, build a file name. | none | `common.h` |
| `instruction_table.c/.h` | The one `static const` table and its access functions; also the register and directive name tables. | `Instruction` (const handle) | `common.h` |
| `syntax.c/.h` | Parse one source line into a statement: label, kind, mnemonic or directive, operands with their modes and values. Report syntax errors. | `Statement` (may be a visible struct of plain values; see note) | `common.h`, `errors`, `util`, `instruction_table` |
| `macro_table.c/.h` | Store macro names and their body lines. | `MacroTable` | `common.h`, `util` |
| `preassembler.c/.h` | Read `.as`, check line length, collect macros, expand calls, write `.am`. | none | `errors`, `util`, `macro_table`, `instruction_table` (reserved words) |
| `symbol_table.c/.h` | Store symbols with address, kind and entry flag. | `SymbolTable` | `common.h`, `util` |
| `memory_image.c/.h` | Hold the code image and the data image and the counters IC and DC. | `MemoryImage` | `common.h` |
| `encoder.c/.h` | Build 10-bit words: first word, immediate, register pair, matrix registers, address with ARE. Pure functions. | none | `common.h` |
| `base4.c/.h` | Convert a number to the base-4 letter notation into a caller's buffer. | none | `common.h` |
| `reference_list.c/.h` | Ordered list of (name, address) pairs, used for external references and for entries. | `ReferenceList` | `common.h`, `util` |
| `first_pass.c/.h` | Walk the `.am` file: define symbols, encode what is known, store data, record words that wait for a symbol. | none | `errors`, `syntax`, `symbol_table`, `memory_image`, `encoder`, `instruction_table`, `macro_table` (name clash check) |
| `second_pass.c/.h` | Walk the `.am` file again: mark entries, resolve symbol words, collect external references. | none | `errors`, `syntax`, `symbol_table`, `memory_image`, `encoder`, `reference_list` |
| `output_writer.c/.h` | Write `.ob`, `.ent` and `.ext`. | none | `errors`, `util`, `memory_image`, `symbol_table`, `reference_list`, `base4` |
| `assembler.c` | `main` and the per-file driver: create the per-file objects, run the stages, stop on errors, free everything. | none | all stage headers |

Note on `Statement`: a parsed line is a value passed between `syntax` and the
passes, not a container with a hidden representation. It may be an opaque type
with access functions (strictest reading of the rule) or a documented plain
struct in `syntax.h`. The opaque form is recommended to stay inside the rule;
the owner should confirm.

### Public functions

`errors.h`
- `ErrorContext *errors_create(const char *file_name)`: start a context for
  one input file; returns `NULL` when memory is exhausted.
- `void errors_report(ErrorContext *ctx, int line, const char *format, ...)`:
  the single function that prints a diagnostic; line 0 means "no line".
- `void errors_warn(ErrorContext *ctx, int line, const char *format, ...)`:
  the same for a warning, which does not count as an error.
- `int errors_count(const ErrorContext *ctx)`: number of errors so far.
- `void errors_destroy(ErrorContext *ctx)`: free the context.

`util.h`
- `char *util_copy_string(const char *text)`: checked heap copy (replaces
  the eleven `malloc` plus `strcpy` pairs).
- `const char *util_skip_spaces(const char *text)`: first non-blank character.
- `char *util_make_file_name(const char *base, const char *extension)`:
  heap-allocated `base` plus `extension`.
- `Boolean util_is_blank(const char *text)`: true when only white space.

`instruction_table.h`
- `const Instruction *instruction_find(const char *name)`: look up a mnemonic.
- `const char *instruction_name(const Instruction *ins)`.
- `int instruction_opcode(const Instruction *ins)`.
- `int instruction_operand_count(const Instruction *ins)`.
- `Boolean instruction_allows_source(const Instruction *ins, AddressingMode m)`.
- `Boolean instruction_allows_destination(const Instruction *ins,
  AddressingMode m)`.
- `int register_number(const char *name)`: 0 to 7, or -1.
- `DirectiveKind directive_find(const char *name)`.
- `Boolean is_reserved_word(const char *word)`: instruction, directive,
  register or macro keyword.

`syntax.h`
- `Statement *statement_parse(const char *line, int line_number,
  ErrorContext *errors)`: parse one line; returns `NULL` after reporting.
- `void statement_destroy(Statement *st)`.
- `StatementKind statement_kind(const Statement *st)`: empty, comment,
  instruction, `.data`, `.string`, `.mat`, `.entry`, `.extern`.
- `const char *statement_label(const Statement *st)`: `NULL` when absent.
- `const Instruction *statement_instruction(const Statement *st)`.
- `int statement_operand_count(const Statement *st)`.
- `AddressingMode statement_operand_mode(const Statement *st, int index)`.
- `int statement_operand_value(const Statement *st, int index)`: immediate
  value or register number.
- `const char *statement_operand_symbol(const Statement *st, int index)`.
- `int statement_operand_matrix_register(const Statement *st, int index,
  int which)`.
- `int statement_data_count(const Statement *st)` and
  `int statement_data_value(const Statement *st, int index)`: the words of a
  data directive (numbers, characters plus the terminator, matrix cells).
- `const char *statement_directive_symbol(const Statement *st)`: the operand
  of `.entry` or `.extern`.
- `int statement_word_count(const Statement *st)`: the one place where the
  length of an instruction is computed.
- `Boolean syntax_is_valid_label(const char *name)`: the one label rule.

`macro_table.h`
- `MacroTable *macro_table_create(void)`, `void macro_table_destroy(
  MacroTable *table)`.
- `Macro *macro_table_add(MacroTable *table, const char *name)`: `NULL` on
  duplicate or allocation failure.
- `Boolean macro_add_line(Macro *macro, const char *line)`.
- `const Macro *macro_table_find(const MacroTable *table, const char *name)`.
- `int macro_line_count(const Macro *macro)`,
  `const char *macro_line(const Macro *macro, int index)`.

`preassembler.h`
- `Boolean preassemble(const char *base_name, MacroTable *macros,
  ErrorContext *errors)`: write `base_name.am`; remove it and return `FALSE`
  on any error.

`symbol_table.h`
- `SymbolTable *symbol_table_create(void)`,
  `void symbol_table_destroy(SymbolTable *table)`.
- `Boolean symbol_table_add(SymbolTable *table, const char *name,
  int address, SymbolKind kind)`: `FALSE` on allocation failure.
- `const Symbol *symbol_table_find(const SymbolTable *table,
  const char *name)`.
- `int symbol_address(const Symbol *s)`, `SymbolKind symbol_kind(
  const Symbol *s)`, `Boolean symbol_is_entry(const Symbol *s)`,
  `const char *symbol_name(const Symbol *s)`.
- `Boolean symbol_table_mark_entry(SymbolTable *table, const char *name)`.
- `void symbol_table_relocate_data(SymbolTable *table, int offset)`: add the
  final IC to every data symbol (replaces the loop in `first_pass.c:54-61`).
- `void symbol_table_for_each(const SymbolTable *table,
  void (*visit)(const Symbol *, void *), void *argument)`: iteration without
  exposing the list (used to write `.ent`).

`memory_image.h`
- `MemoryImage *memory_image_create(void)`,
  `void memory_image_destroy(MemoryImage *image)`.
- `Boolean memory_image_add_code(MemoryImage *image, unsigned int word)`,
  `Boolean memory_image_add_data(MemoryImage *image, unsigned int word)`:
  `FALSE` when the memory is full.
- `void memory_image_set_code(MemoryImage *image, int address,
  unsigned int word)`: patch a word in the second pass.
- `int memory_image_code_count(const MemoryImage *image)`,
  `int memory_image_data_count(const MemoryImage *image)`.
- `unsigned int memory_image_code_word(const MemoryImage *image, int index)`,
  `unsigned int memory_image_data_word(const MemoryImage *image, int index)`.

`encoder.h`
- `unsigned int encode_first_word(int opcode, AddressingMode source,
  AddressingMode destination)`.
- `unsigned int encode_immediate(int value)`.
- `unsigned int encode_registers(int source_register,
  int destination_register)`.
- `unsigned int encode_address(int address, AreKind are)`.
- `unsigned int encode_data(int value)`.

`base4.h`
- `void base4_format(unsigned int value, int digits, char *buffer)`: write
  exactly `digits` letters and a terminator; no allocation.

`reference_list.h`
- `ReferenceList *reference_list_create(void)`,
  `void reference_list_destroy(ReferenceList *list)`.
- `Boolean reference_list_append(ReferenceList *list, const char *name,
  int address)`.
- `int reference_list_count(const ReferenceList *list)`,
  `const char *reference_list_name(const ReferenceList *list, int index)`,
  `int reference_list_address(const ReferenceList *list, int index)`.

`first_pass.h`
- `Boolean first_pass(FILE *am_file, SymbolTable *symbols,
  MemoryImage *image, const MacroTable *macros, ErrorContext *errors)`.

`second_pass.h`
- `Boolean second_pass(FILE *am_file, SymbolTable *symbols,
  MemoryImage *image, ReferenceList *externals, ErrorContext *errors)`.

`output_writer.h`
- `Boolean write_output_files(const char *base_name,
  const MemoryImage *image, const SymbolTable *symbols,
  const ReferenceList *externals, ErrorContext *errors)`.

### Mapping from the existing code

| Existing file or function | Destination | Remark |
|---|---|---|
| `Assembler.h` | `common.h` and `errors.h` | Renamed; the case problem disappears. Error codes become message texts at the call sites. |
| `types.h` | `common.h` for constants; each struct moves into its module's `.c` file | `AddressingType`, `data_memory`, `ExtRef`/`EntryPoint` duplicates are dropped. |
| `assembler.c`: `main`, `process_files`, `process_single_file` | `assembler.c` | Kept. The two helpers become `static`; `process_single_file` is split per stage and owns all per-file objects. |
| `error_handling.c`: `print_error` | `errors.c`: `errors_report` | Kept in spirit; gains the file name and the error count. |
| `first_pass.c`: `error` | `errors.c`: `errors_report` | Merged; it already has the right `printf`-style signature. |
| `file_utils.c`: `create_filename_with_extension`, `file_exists` | `util.c` | Kept. `cleanup_and_exit` is deleted (never called). |
| `line_parser.c`: `is_line_too_long` | `preassembler.c` (static) | The file is deleted; 21 lines do not justify a module. |
| `word_extractor.c`: `get_first_word`, `get_second_word`, `has_extraneous_text_after_words` | `preassembler.c` (static), on top of `util.c` | The three become one tokenising helper without `strtok` and without allocation. |
| `macro_and_label_func.c`: generic table, `add_macro`, `find_macro`, `cleanup_macro_data` | `macro_table.c` | The generic layer is removed: with the label table gone it has one user. |
| `macro_and_label_func.c`: label table, `has_label`, `extract_label_name`, `get_content_after_*`, `is_valid_label_name` | Deleted; label work moves to `syntax.c` and `first_pass.c` | The pre-assembler stops validating labels. The macro-name against label-name clash is checked in the first pass, which receives the macro table. |
| `macro_and_label_func.c`: `is_reserved_word` | `instruction_table.c` | Derived from the tables instead of four local arrays. |
| `macro_and_label_func.c`: `is_valid_name`, `is_valid_macro_name` | `syntax.c`: `syntax_is_valid_label` plus a macro variant | One rule, one length limit. |
| `preassembler.c`: `preassembler` | `preassembler.c`: `preassemble` plus static helpers | Kept as the same state machine (definition start, definition end, body line, call, ordinary line), one helper per state. |
| `preassembler.h` macros `REPORT_*` | Deleted | Replaced by `errors_report` and ordinary control flow. |
| `line_analysis.c`: `is_comment_or_empty`, `is_empty_line`, `is_comment_line`, `extract_label`, `is_valid_label`, `is_label`, `is_command`, `is_data_or_string`, `parse_operands`, `is_register`, `is_immediate`, `is_matrix`, `is_label_operand` | `syntax.c` (static helpers behind `statement_parse`) | Kept as the body of the parser. `line_analysis` is renamed `syntax` to match the rule's wording; keeping the old name is equally acceptable. |
| `line_analysis.c`: `count_data_items`, `count_string_length`, `count_matrix_items`, `count_command_words` | `syntax.c`: `statement_data_count`, `statement_word_count` | The counts come from the parsed statement, so they cannot disagree with the encoding. |
| `line_analysis.c`: `trim_whitespace` | `util.c` | Kept. `label_exists` is deleted. |
| `instruction_table.c` | `instruction_table.c` | Kept; the table gains the two mode sets and becomes `static const`. The four unused functions are deleted. |
| `instruction_validation.c`: `classify_operand`, `validate_command_line`, `validate_operands_count`, `validate_operands_types`, `is_type_allowed_for_instruction` | `syntax.c` | Operand classification becomes part of parsing; the mode check becomes two table lookups. The file is deleted. |
| `memory_builder.c`: `init_memory_image`, `add_data_word`, `add_instruction_word`, `update_instruction_word`, `free_memory_image` | `memory_image.c` | Kept with an opaque type. |
| `memory_builder.c`: `encode_first_word`, `encode_immediate_operand`, `encode_register_operand` | `encoder.c` | Kept almost as they are. |
| `memory_builder.c`: `build_memory_image`, `encode_data_directive`, `encode_string_directive`, `encode_matrix_directive`, `encode_instruction_first_pass` | `first_pass.c` (static helpers) using `syntax`, `encoder` and `memory_image` | The third pass is merged into the first. The string tokenising inside them is replaced by the parsed statement. |
| `memory_builder.c`: `get_opcode_value`, `get_addressing_method`, `extract_matrix_registers` | Deleted | Replaced by `instruction_opcode` and the statement accessors. |
| `first_pass.c`: `first_pass`, `adjust_data_addresses_with_icf` | `first_pass.c`; `symbol_table_relocate_data` | Kept as the same loop, split into one static handler per statement kind. |
| `second_pass.c`: `second_pass`, `process_instruction_second_pass`, `process_entry_directive`, `encode_operand` | `second_pass.c` | Kept, static, on top of the parsed statement. `get_instruction_length` is deleted. |
| `second_pass.c`: `add_external_reference`, `add_entry_point`, `free_ext_list`, `free_entry_list` | `reference_list.c` | One list type instead of two identical ones. Entries can be read from the symbol table, so the entry list may disappear altogether. |
| `second_pass.c`: `decimal_to_base4` | `base4.c`: `base4_format` | Kept; writes into a caller's buffer, which removes twelve allocation checks in the writer. |
| `second_pass.c:106` (call to the writer) | `assembler.c` | The driver calls the writer after a clean second pass. |
| `output_writer.c`: `generate_output_files`, `write_object_file`, `write_entries_file`, `write_externals_file` | `output_writer.c` | Kept; the two list writers become one static function; `get_base_filename`, `count_entries`, `count_externals` are deleted. |
| `symbol_table.c` | `symbol_table.c` | Kept as a linked list behind an opaque handle. `add_symbol_to_table` is split into add, mark-entry and declare-extern; the policy decisions move to the passes; `print_symbol_table` is deleted. |
| `Makefile` | `Makefile` | Reduced to `all`, `test`, `clean` with exact per-object header lists. |

### Where this departs from the existing shape, and why

- `memory_builder.c` stops being a pass. Reason: it is a second, divergent
  implementation of the first pass (R1-06, R1-07), and the booklet's algorithm
  has two passes.
- `instruction_validation.c` and most of `line_analysis.c` merge into
  `syntax.c`. Reason: classification, validation and length counting are one
  job done on one parsed form; keeping them apart is what allowed four
  different definitions of an addressing mode (R3-09).
- The generic table and the pre-assembler's label table are removed. Reason:
  the generic layer serves one real client, exposes `void *` data, and the
  label table duplicates the symbol table with a different validity rule.
- `line_parser.c`, `word_extractor.c` and `file_utils.c` are folded into
  `preassembler.c` and `util.c`. Reason: they are fragments without headers
  (R1-03).
- New small modules `encoder`, `base4` and `reference_list` are added.
  Reason: the rule lists base-4 encoding as its own module, and word layout
  must be known in one place (R2-07).
- Everything else keeps its file name and its role: `assembler.c`,
  `preassembler.c`, `first_pass.c`, `second_pass.c`, `symbol_table.c`,
  `instruction_table.c`, `output_writer.c`.

## Proposed order of refactoring steps

Each step changes no behaviour and leaves the status of every test as it was.
Because the binary's output is the test oracle, steps marked "output-visible"
must be scheduled as bug fixes instead, not as refactoring.

Steps to do before the bug fixes (they make every later fix cheaper):

1. Fix the build: rename `Assembler.h` to `assembler.h` (or the includes),
   and correct `HEADERS` in the `Makefile`. Nothing else can be verified
   until this is done.
2. Reduce the `Makefile` to `all`, `test` and `clean`; remove the stale and
   dangerous targets (R11-04, R11-05); make `clean` remove generated outputs.
3. Remove dead material: the nine never-called functions, the three undefined
   prototypes, the duplicate prototypes, unused types, constants and fields
   (R1-12, R1-14, R1-15, R1-17), the stray files and `tests/sh/` (R12-01,
   R12-03, with the owner's consent for deletions).
4. Give every source file its own header and make every header declare only
   what its own source file defines (R1-02, R1-03, R1-16). Mark the 41
   file-local functions `static` (R5-01). Pure declarations; no code moves.
5. Introduce `errors.c/.h` with the single reporting function and route all
   diagnostic call sites (R7-01, R7-03) through it, keeping each message's text and stream for now.
   Doing this first means every later bug fix that adds or changes a message
   touches one call instead of inventing a sixth format, and the later switch
   to the spec's format (ERR-R13, output-visible) is a one-function change.
6. Complete the instruction table: add the legal source and destination
   modes, make it `static const`, add the access functions, and add the
   register and directive tables. Replace `get_opcode_value`, `is_command`'s
   list and `is_reserved_word`'s arrays with lookups. The mode check in
   `is_type_allowed_for_instruction` is replaced by table lookups carrying
   the modes the code allows today; correcting the modes to SPEC section 3 is
   then a one-line data fix per instruction instead of an edit to a `strcmp`
   chain.
7. Replace the global symbol table with a `SymbolTable *` created and
   destroyed by the driver, and make `first_pass` return its error flag. The
   handle is passed down as a parameter. Strictly this changes behaviour for
   a second input file (R4-01), so it is a bug fix by nature; it is listed
   here because almost every other fix in the passes changes the same
   signatures, and doing it afterwards means touching them twice.
8. Introduce the parsed statement (`syntax.c`) next to the existing code and
   move the first pass onto it. This is the largest step and the one with the
   highest pay-off before bug fixing: most behavioural bugs that the other
   audit will list (substring matching of directives, blanks after commas,
   operand classification, length disagreements) live in the three duplicated
   parsers, and each would otherwise have to be fixed three times.

Steps that can wait until after the bug fixes:

9. Merge `memory_builder.c` into the first pass and move the second pass onto
   the parsed statement; delete the old parsers.
10. Make the remaining types opaque: `MemoryImage`, the macro table, the
    reference list, the second-pass context (rule 2).
11. Extract `encoder.c` and `base4.c`; replace every shift, mask and width
    literal with named constants (rule 6).
12. Split the long functions (R5-02), starting with `preassembler`,
    `first_pass` and `write_object_file`.
13. Memory pass: one checked string-copy helper, no `exit` outside `main`,
    every path freed; then a valgrind run over the whole test suite.
14. Documentation pass: header comment per file, comment per function,
    English only, consistent line endings and indentation.
15. Rewrite `README.md` and `CHANGELOG.md` for the portfolio (rule 12).

## Rewrite or refactor: judgement per module

- `memory_builder.c` (670 lines): rewrite, in effect delete. About 450 of its
  lines are a third parser and a third length calculation
  (`build_memory_image`, the four `encode_*` entry points,
  `get_opcode_value`, `get_addressing_method`, `extract_matrix_registers`).
  Only the three word builders and the three image functions (about 90 lines)
  survive. Evidence: R1-06, R1-07, R3-03, R3-09, R4-04, and the wrong `.data`
  result.
- `preassembler.c` (496 lines, one function of 478 lines): rewrite the
  function, keep the algorithm. The state machine is sound, but the function
  cannot be refactored line by line: it depends on macros that hide
  `continue` and `exit`, on a predicate that mutates the line, on a triple
  copy of every macro body and on an allocator that returns unfreeable
  pointers. Evidence: R2-08, R7-06, R8-02, R8-04, R8-08, R8-09, and the abort
  on a label after leading blanks. A fresh function of about 150 lines in six
  helpers is smaller than the set of patches.
- `instruction_validation.c` (314 lines): replace by table lookups inside
  `syntax.c`. About 200 lines are `strcmp` chains that vanish once the table
  carries the modes. Evidence: R3-04, R3-05, R3-08.
- `line_analysis.c` and the parsing halves of `first_pass.c` and
  `second_pass.c`: rewrite as one parser. The predicates are individually
  small and reusable, but the approach (`strstr` anywhere in the line, `sscanf`
  with fixed widths, `strtok`) cannot express the spec's syntax rules, and
  this is where a line-by-line refactoring would cost the most. Evidence:
  R1-07, R1-19, R3-09.
- `first_pass.c` and `second_pass.c` as passes: refactor. Their loops and
  their order of work follow the booklet's algorithm and can be kept once
  they consume a parsed statement.
- `symbol_table.c`, `instruction_table.c`, `output_writer.c`,
  `error_handling.c`, `assembler.c`, `macro_and_label_func.c` (macro part):
  refactor. Each has a sound core and local defects only.

`CLAUDE.md` forbids a whole-module rewrite without asking the owner. The four
rewrites above (`memory_builder.c`, the `preassembler` function, the
validation chains, the line parser) therefore need the owner's approval before
Phase 3.
