# Correctness audit: assembler against docs/SPEC.md

Date: 2026-11-26. Role: auditor (correctness), read-only. No file in the
repository was changed.

## How this list was produced

- All 15 `.c` files and 11 `.h` files in the repository root were read in full.
- The sources were copied to a scratch directory, a lowercase copy of the
  header was added, and two binaries were built there: `gcc -g -Wall -ansi
  -pedantic` (2 warnings, see BUG-11) and the same sources with
  `-fsanitize=address,undefined`.
- `tests/run_tests.sh` was run with the scratch binary: passed 1, failed 123,
  total 124 (same as the earlier log).
- Every valid case was re-compared after discounting the shared format causes
  (space replaced by TAB in `.ob`, `.ent`, `.ext`; blank and comment lines
  removed from the expected `.am`). After that discount 3 of the 56 valid
  cases have a correct `.ob` (`no_entry_no_extern`, `no_operand_instructions`,
  `only_code`); the other 53 still have wrong words.
- For every error case the line numbers in the program's messages (on either
  stream, in any format) were compared with `<case>.expected`.
- About 25 extra inputs that the suite does not cover were run (indented
  labels, a colon inside a string, 31-character names, 156 code words, data
  before code overflow, CR LF, missing final newline, and others).
- valgrind was run on 13 inputs (results in section 5).

Limits of this audit, stated plainly:

- The mapping of valid cases to encoding bugs (section 3) was derived by
  reading the diffs and matching each wrong word to a located root cause, and
  then generated for all cases with a script that looks for the triggering
  construct in the expected `.am`. It was not confirmed by building a fixed
  binary, because the auditor does not write fixes. A case can therefore
  still hide a further bug behind the listed ones. The fix order below is
  designed to expose such bugs early.
- The expected `.ob` files of the suite were not recomputed by hand. Where a
  diff was examined, the expected word always agreed with SPEC; that is a
  spot check of roughly 20 cases, not a certification of all 56.

Severity classes used: crash or memory error; wrong output for valid input;
valid input rejected; invalid input accepted; wrong or missing diagnostic;
format; build.

Size classes: small (a few lines), medium (one function), large (needs
restructuring a module).

Line numbers refer to the files as they are in the repository today.

---

## 1. Bug list, in recommended fix order

### Build

#### BUG-01 The program does not build: header file name differs in letter case
- Severity: build.
- Root cause: the file on disk is `Assembler.h`; `preassembler.h:4`,
  `types.h:5`, `assembler.c:19` include `"assembler.h"`, and `Makefile`
  (`HEADERS`, and the rules for `assembler.o` and `preassembler.o`) names
  `assembler.h`.
- SPEC / rules: CLAUDE.md "Build and test" (the build must work with zero
  warnings); SPEC IMP-3 (the main file and folder are named `assembler`).
- Observed: `make` stops with "No rule to make target 'assembler.h'".
  Expected: `make` builds `assembler`.
- Tests: all 124 (no binary). It is the only reason `cli/no_arguments` fails
  in the repository.
- Fix direction: rename the header to `assembler.h`. Size: small.

### Shared causes that mask everything else

#### BUG-02 Output files use a space where SPEC requires one TAB
- Severity: format.
- Root cause: `output_writer.c:103`, `:127`, `:156` (`.ob`), `:222` (`.ent`),
  `:284` (`.ext`) print `"%s %s\n"`.
- SPEC: OUT-13.
- Observed (`example1`): first line `bbc bab` with a space. Expected:
  `bbc<TAB>dd`.
- Tests: all 56 valid cases and the 6 cli cases that expect an `.ob`. It is
  the only reason for `valid/no_entry_no_extern`,
  `valid/no_operand_instructions` and `valid/only_code`.
- Fix direction: print a TAB between the two fields in the five format
  strings. Size: small.

#### BUG-03 The `.am` file drops empty lines and comment lines, also inside macro bodies
- Severity: format (wrong `.am` for valid input).
- Root cause: `preassembler.c:106-109` skips every empty or comment line
  before anything else, so the line is neither copied to the output nor
  stored in a macro body.
- SPEC: MAC-29, MAC-14, MAC-5.
- Observed (`no_macros`): the `.am` is the `.as` without its comment line.
  Expected: byte-identical copy. In `macro_blank_and_comment_lines` the
  comment and the blank line of the body are missing at every call.
- Side effect: because lines are dropped, the line numbers that the passes
  print (they count `.am` lines) are shifted even in files without macros
  (for example `errors/memory_overflow` reports line 4 for `.as` line 5).
- Tests: every case whose expected `.am` holds a blank or comment line (34
  valid cases, several error and cli cases; see section 3).
- Fix direction: copy such lines unchanged, and store them as body lines
  while a macro is being collected. Size: small.

#### BUG-04 The `.am` file loses trailing whitespace on every line that contains a colon
- Severity: format (wrong `.am` for valid input).
- Root cause: `macro_and_label_func.c:374` (`has_label`) casts away `const`
  and calls `trim_whitespace` on the caller's line buffer;
  `line_analysis.c:390` writes a NUL after the last non-blank character. The
  line that is later written (`preassembler.c:445`, `:461`) or stored in a
  macro body (`:351`) is the truncated one, and a newline is re-added.
- SPEC: MAC-29 ("copied exactly as it is, character for character").
- Observed (`line_80_chars`): `MAIN:   mov #1, r1` followed by 62 spaces is
  written without the spaces. `errors/label_alone`: `OTHER:   ` becomes
  `OTHER:`.
- Tests: `valid/line_80_chars`, `errors/label_alone` (not the only reason).
- Fix direction: make `has_label` read-only (no in-place trimming).
  Size: small.

#### BUG-05 Diagnostics do not have the SPEC format and most go to stderr
- Severity: wrong or missing diagnostic.
- Root cause: there is no single reporting function and none knows the file
  name. `error_handling.c:5-66` (`print_error`) and `first_pass.c:27-44`
  (`error`) write `Error on line N: ...` to `stderr`;
  `symbol_table.c:51,59,77,84,93` write `Error: ... at line N` to `stderr`;
  `instruction_validation.c` (17 `printf` calls), `first_pass.c:182,190,198,
  237`, `second_pass.c:144,153,251,260,267,309` and `memory_builder.c:60,82,
  98` write `Error on line N:` or `Error line N:` to `stdout`.
  `assembler.c:108` reports a missing file as `Error: File error - input
  file does not exist`, without the file name.
- SPEC: ERR-R2, ERR-R13, ERR-1, CLI-8, CLAUDE.md ("all error messages go
  through one function in the error module").
- Observed (`cli/missing_file_only`): `Error: File error - input file does
  not exist` on stderr. Expected: `nosuch.as: error: <text>` on stdout.
- Tests: all 59 error cases and the 6 cli cases with expected messages. It is
  the only reason for `errors/line_too_long`, `macro_call_extra_text`,
  `macro_def_extra_text`, `macro_end_extra_text`, `macro_name_invalid`,
  `macro_name_missing`, `macro_name_reserved_mcro`,
  `preassembler_error_hides_pass_errors`, `undefined_symbol`,
  `cli/as_suffix_argument` and `cli/missing_file_only`.
- Fix direction: one reporting function that receives the file name, the
  line and the text and prints `<name>.as:<line>: error: <text>` on stdout;
  route every message through it. Size: medium (many call sites, each a
  one-line change; the file name has to be passed down to every stage).

#### BUG-06 The result of the first pass is thrown away
- Severity: invalid input accepted.
- Root cause: `first_pass` returns `void` (`first_pass.h:9`,
  `first_pass.c:74`); its local `has_errors` (`:80`) is only printed
  (`:252`). `assembler.c:142` calls it and continues unconditionally with
  `build_memory_image` (`:147`), the second pass (`:157`) and the output
  files. `build_memory_image` has a `has_errors` that is never set
  (`memory_builder.c:117`).
- SPEC: ERR-R6, ERR-R7, CLI-9, section 5.3 (effect of first-pass errors).
- Observed (`errors/too_few_operands`): six first-pass messages, then
  `too_few_operands.ob` is written and the exit status is 0. Expected: no
  `.ob`, status 1. In `first_pass_error_hides_second_pass` the second pass
  runs and reports `NOWHERE`, which must stay hidden.
- Tests: 37 error cases and 2 cli cases (section 3). It is the only reason,
  together with BUG-05, for `errors/dest_mode_illegal`, `extern_then_label`,
  `label_space_before_colon`, `label_then_extern`,
  `matrix_index_not_register`, `source_mode_illegal`, `too_few_operands` and
  `memory_overflow` (the last also needs BUG-03).
- Fix direction: return the error state from `first_pass` and stop before
  memory building when it is set. Size: small.
- Note: fixing this turns the false rejections of BUG-28 and BUG-30 from a
  harmless message into a failed assembly, so fix those in the same phase.

#### BUG-07 Line numbers in pass messages are `.am` line numbers; there is no line map
- Severity: wrong or missing diagnostic.
- Root cause: `first_pass.c:99`, `memory_builder.c:134` and
  `second_pass.c:58` count the lines of the `.am` file. The pre-assembler
  builds no map from `.am` lines to `.as` lines (`preassembler.c`, whole
  function).
- SPEC: ERR-R12, MAC-35, ERR-R3, section 2.14.
- Observed (`errors/line_numbers_in_macro_body`): the bad body line (line 8
  of the `.as`) is reported as line 5. Expected: 4, 4 and 8.
- Tests: `errors/line_numbers_after_macro`, `line_numbers_in_macro_body`,
  `line_numbers_second_pass_macro`, `macro_nested_call_not_expanded`,
  `label_same_as_macro` (none of them only for this reason).
- Fix direction: let the pre-assembler record the `.as` line of every `.am`
  line (for body lines, the line inside the definition) and hand the map to
  the passes. Size: large (new data flow between three modules).

#### BUG-08 The symbol table of one file leaks into the next file
- Severity: wrong output for valid input (and a memory leak).
- Root cause: the table is a global (`symbol_table.c:13`) and
  `free_symbol_table` (`symbol_table.c:127`) is never called;
  `assembler.c:162-170` frees everything else.
- SPEC: CLI-3, ALG-4; CLAUDE.md (no globals; free everything between files).
- Observed (`cli/several_valid_files`, `alpha beta gamma`): for `beta` the
  first pass prints `Error on line 1: Duplicate label: MAIN`; the symbols
  keep the addresses of `alpha`, so `beta.ob` and `beta.ext` are wrong.
  valgrind shows the symbol nodes as still reachable at exit.
- Tests: `cli/several_valid_files`, `same_names_in_two_files`,
  `valid_then_error`, `error_then_valid`.
- Fix direction: release the table at the end of every file on every path;
  better, make it an object owned by the per-file driver. Size: small for
  the call, medium to remove the global.

#### BUG-09 The pre-assembler checks labels, which is the first pass's job, and deletes the `.am`
- Severity: wrong or missing diagnostic (wrong stage, missing follow-up
  errors, missing `.am`).
- Root cause: `preassembler.c:248-315` validates the label of every line
  (syntax, reserved word, macro-name clash, duplicate) and keeps its own
  label table (`:30`, `:303`); `:143-146` rejects a macro whose name is an
  earlier label. Each hit sets `has_errors`, so the `.am` is removed
  (`:489-492`) and the passes never run. The checks also run on macro body
  lines (they come before the collection at `:318`). The first pass, in
  turn, has no reserved-word check and no access to the macro table
  (`line_analysis.c:26-39`).
- SPEC: ERR-11 to ERR-15 (stage: first pass), OUT-11, MAC-24, LBL-21 (the
  clash is reported on the label line), LBL-23, section 5.3.
- Observed (`errors/label_on_error_line_is_defined`): only line 4 is
  reported and no `.am` exists; line 2 (`X: mov r1`, too few operands) is
  never seen. Expected: errors on 2 and 4, `.am` present.
  `errors/label_same_as_macro`: the clash of `late:` (line 1) with
  `mcro late` (line 8) is reported on line 8. Expected: line 1.
- Tests: `errors/label_duplicate`, `label_invalid_chars`,
  `label_on_error_line_is_defined`, `label_reserved_mcro`,
  `label_reserved_word`, `label_same_as_macro`, `label_too_long`.
- Fix direction: remove the label checks from the pre-assembler and do all
  of them in the first pass, which then needs the macro table (LBL-21) and
  the reserved-word list. Size: large (touches both modules and the driver;
  depends on BUG-42, BUG-43 and interacts with BUG-31).

### Crashes and memory errors

#### BUG-10 Abort on any line that has whitespace before text that contains a colon
- Severity: crash or memory error.
- Root cause: `macro_and_label_func.c:319` returns `trim_whitespace(result)`,
  which is a pointer into the middle of the allocated block when the line
  starts with whitespace. The caller frees that pointer
  (`preassembler.c:310`, and through the `REPORT_ERROR_ONLY` macro at
  `:258`, `:267`, `:276`, `:285`, `:296`; also `:454`).
- SPEC: SYN-19 (indentation before the first field is legal), DIR-28.
- Observed, minimal input `  MAIN: inc r1`: the plain binary ends with
  `free(): invalid pointer`, exit status 134; the sanitizer build reports
  `attempting free on address which was not malloc()-ed` at
  `preassembler.c:310`. The same happens for `        .string "a:b"` (at
  `:258`).
- Tests: none (every label in the suite starts in column 1). Not covered.
- Fix direction: return the start of the allocation (move the trimmed text
  to the front, as `get_content_after_char` already does). Size: small.
  The whole code path disappears if BUG-09 is fixed by removing the label
  handling from the pre-assembler.

#### BUG-11 Use after free when an `.ent` or `.ext` file is written
- Severity: crash or memory error (undefined behaviour).
- Root cause: `output_writer.c:229-231` frees `ent_filename` and then prints
  it; `:291-293` does the same with `ext_filename`.
- SPEC: none directly; CLAUDE.md (zero warnings: these are the two warnings
  gcc prints, `-Wuse-after-free`).
- Observed: `valgrind ./assembler ps` reports 23 invalid reads in 6
  contexts, all below `write_entries_file` (`:231`) and
  `write_externals_file` (`:293`).
- Tests: none fails because of it; it affects every case with entries or
  externals.
- Fix direction: print before freeing, or drop the progress message.
  Size: small.

#### BUG-12 Names of 31 characters are accepted, and a 31-character macro name overflows a stack buffer
- Severity: crash or memory error; invalid input accepted.
- Root cause: `Assembler.h:36` defines `MAX_LABEL_LENGTH` as 31 (the buffer
  size), and `macro_and_label_func.c:386` rejects only
  `strlen(name) > MAX_LABEL_LENGTH`, so 31 characters pass.
  `preassembler.c:157` then copies the name with `strcpy` into
  `current_macro_name[MAX_LABEL_LENGTH]` (`:41`), 32 bytes into 31.
- SPEC: MAC-33, MAC-17, ERR-6, LBL-3, ERR-12.
- Observed (`errors/macro_name_too_long`): no message, status 0, `.am` and
  `.ob` written. The sanitizer build reports `stack-buffer-overflow` in
  `strcpy` at `preassembler.c:157`. Expected: one error on line 5.
- Tests: `errors/macro_name_too_long`, `errors/label_too_long` (line 2).
- Fix direction: compare against the maximum name length 30 (a separate
  constant from the buffer size). Size: small.

#### BUG-13 The first pass never closes its input file
- Severity: crash or memory error (resource leak).
- Root cause: `first_pass.c:82` opens the `.am` file; there is no `fclose`
  anywhere in the function.
- SPEC: ERR-R11.
- Observed: valgrind shows one 472-byte block from `fopen` (called at
  `first_pass.c:82`) still reachable for every file that reaches the first
  pass.
- Tests: none.
- Fix direction: close the file before returning. Size: small.

#### BUG-14 Allocation failures are not handled as SPEC requires
- Severity: crash or memory error (only on allocation failure).
- Root cause, several places: `preassembler.c:24-27` uses the results of
  `create_filename_with_extension` without a check; `:168` does not check
  `malloc` for `macro_lines`; the `REPORT_CRITICAL_ERROR_AND_EXIT` macro
  (`preassembler.h:98-108`) calls `exit(1)` without freeing the tables or
  closing the files, and inside an `if` without braces only its first
  statement would be conditional; `symbol_table.c:24-27` exits without
  freeing; `second_pass.c:375-380` and `:393-398` report and continue;
  `preassembler.c:329-335` reports and continues with the next line;
  `word_extractor.c:14-17` returns NULL, which callers treat as "no word".
- SPEC: ERR-R10, CLAUDE.md (check every `malloc`, free on every path).
- Tests: none (cannot be provoked by a test).
- Fix direction: one fatal-error path that frees the per-file state and
  exits with status 1. Size: medium.

### Wrong output for valid input

#### BUG-15 The operand of a one-operand instruction is encoded as a source operand
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:406-409` puts the mode of the only operand
  into `src_method`, and `:434-450` emits its word with the source layout
  (`encode_register_operand(reg, 0)` puts the register in bits 9-6).
- SPEC: ENC-9, ADR-9.
- Observed (`no_stop`, `inc r1`): `bddaa` and `abaaa`. Expected: `bdada` and
  `aaaba`. `jmp MAIN`: `cbbaa`, expected `cbaba`.
- Tests: 42 valid cases (section 3), among them `example1` and `ps`.
- Fix direction: when the instruction has one operand, treat it as the
  destination in both the first word and the operand word. Size: small.

#### BUG-16 A destination register that does not share its word is placed in bits 9-6
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:488-494` calls
  `encode_register_operand(reg, 0)` for the destination; the register must be
  the second argument.
- SPEC: ADR-9.
- Observed (`example1`, `mov M1[r3][r3],r3`): word at 118 is `adaaa`.
  Expected: `aaada`.
- Tests: 29 valid cases.
- Fix direction: pass the register as the destination field. Size: small.

#### BUG-17 An immediate value is stored in bits 9-0 instead of bits 9-2
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:598-610` (`encode_immediate_operand`) masks
  the value to 10 bits and does not shift it left by two.
- SPEC: ADR-3, ENC-15.
- Observed (`example1`, `prn #-5`): `dddcd`. Expected: `ddcda`.
- Tests: 18 valid cases.
- Fix direction: mask to 8 bits and shift left by 2. Size: small.

#### BUG-18 The index word of a matrix destination operand is never encoded
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:496-506` emits two zero words for a matrix
  in the destination position; only the source branch (`:451-468`) calls
  `extract_matrix_registers`. The second pass patches only the address word.
- SPEC: ADR-7.
- Observed (`modes_lea`, `lea X, M[r1][r2]`): index word `aaaaa`. Expected:
  `abaca`.
- Tests: 8 valid cases.
- Fix direction: encode the index word in the destination branch as in the
  source branch. Size: small.

#### BUG-19 The second pass patches the wrong word when the source operand is a register
- Severity: wrong output for valid input.
- Root cause: `second_pass.c:193-194` sets `operand1_words = 0` for a
  register source, so `addr2` (`:202`) is the address of the source register
  word. The label word overwrites the register word, the real destination
  word stays 0, and the external reference is recorded one address too low.
- SPEC: ADR-10, ENC-11, ADR-5, OUT-9.
- Observed (`example1`, `add r2,STR`): words `bdccc`, `aaaaa`. Expected:
  `acaaa`, `bdccc`. `extern_multiple_uses`, `mov r1, W`: `.ext` line
  `W bcca`. Expected: `W<TAB>bccb`.
- Tests: 9 valid cases.
- Fix direction: count one word for a register source unless both operands
  are registers. Size: small.

#### BUG-20 Any operand that starts with `r` and a digit gets addressing mode 3
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:567` tests `operand[0] == 'r' &&
  isdigit(operand[1])`, which is true for `r8`, `r20` and `r1x`, while
  `is_register` (`line_analysis.c:176-182`) correctly accepts only
  `r0`..`r7`. The first word says "register" and the operand word is a label
  word.
- SPEC: LBL-16, LBL-20, ADR-15.
- Observed (`label_names_legal`, `mov r8, r1`): first word `aadda`.
  Expected: `aabda`.
- Tests: `valid/label_names_legal`.
- Fix direction: use `is_register` to decide the mode. Size: small.

#### BUG-21 `.data` stores only the values up to the first whitespace in the list
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:200-215` splits the line on spaces and tabs
  and then splits only the first token after `.data` on commas. The first
  pass (`line_analysis.c:137-162`) counts all values, so the data length,
  the image and all later data addresses disagree with one another.
- SPEC: DIR-5, DIR-6, DIR-7.
- Observed (`only_data`, `X: .data 1, 2, 3`): header `a b` and one data
  word. Expected: `a<TAB>d` and three words.
- Tests: 11 valid cases.
- Fix direction: split the text after `.data` on commas and trim each value.
  Size: small.

#### BUG-22 `.string` stores the quotation marks and does not use them as delimiters
- Severity: wrong output for valid input.
- Root cause: `line_analysis.c:241-278` (`count_string_length`) and
  `memory_builder.c:238-281` (`encode_string_directive`) take every
  printable character from the first non-blank character after `.string` to
  the end of the line, including both quotes and any text after the closing
  quote.
- SPEC: DIR-10, SYN-22, DIR-32.
- Observed (`string_empty`, `.string ""`): three words `aacac aacac aaaaa`.
  Expected: one word `aaaaa`. `example1`: data length `bab` (17) instead of
  `dd` (15), and every data label after `STR` is 2 too high (`LENGTH` is 131
  instead of 129), which also corrupts `ps.ent`. An unlabelled
  `.string "ab"` followed by three spaces stores the three spaces as well.
- Tests: 16 valid cases, `cli/same_names_in_two_files`.
- Fix direction: take the characters between the first and the last
  quotation mark. Size: medium (two functions must agree; better one shared
  parser).

#### BUG-23 `.mat` does not emit the cells that have no listed value
- Severity: wrong output for valid input.
- Root cause: `memory_builder.c:284-336` writes one word per listed value
  and returns early (`:310-312`) when there is none; the first pass adds
  rows x columns to DC (`line_analysis.c:164-174`). Image and symbol
  addresses disagree.
- SPEC: DIR-13, DIR-24, DIR-15.
- Observed (`mat_no_values`): header `bd b` and one data word. Expected:
  `bd<TAB>ca` (8 words).
- Tests: 12 valid cases, among them `memory_exactly_full`.
- Fix direction: pad with zero words up to rows x columns. Size: small.

#### BUG-24 `.mat` dimensions written with whitespace inside the brackets count as size 0
- Severity: wrong output for valid input.
- Root cause: `line_analysis.c:170` uses `sscanf(start, "[%d][%d]", ...)`,
  which stops at a blank before `]`, so the product is 0.
- SPEC: DIR-27.
- Observed (`mat_spacing`): data length `cd` (11). Expected: `baa` (16).
- Tests: `valid/mat_spacing`.
- Fix direction: parse the dimensions with a scanner that skips whitespace
  inside the brackets. Size: small.

#### BUG-25 A matrix operand with whitespace inside its brackets is mis-sized and loses its index registers
- Severity: wrong output for valid input.
- Root cause: `line_analysis.c:185-219` (`parse_operands`) reads each
  operand with `sscanf("%63s")`, which stops at the first blank, so
  `M1[ r2 ][ r6 ]` becomes `M1[`, is counted as one word and classified as a
  direct operand; IC and all later addresses are wrong.
  `memory_builder.c:655` and `:668` (`extract_matrix_registers`) require `r`
  directly after `[` and otherwise leave the register number 0.
- SPEC: SYN-18, ADR-7, ADR-14.
- Observed (`whitespace_matrix_brackets`, line 1): index word `aaaaa`,
  address of `M1` off by 5. Expected: `acbca`.
- Tests: `valid/whitespace_matrix_brackets`; line 1 of
  `errors/matrix_operand_whitespace`.
- Fix direction: split operands at the comma only and skip whitespace
  inside the brackets. Size: medium.

#### BUG-26 Statements are classified by searching for directive names anywhere in the line
- Severity: wrong output for valid input; invalid input accepted.
- Root cause: `strstr(line, ".data")` and its siblings in
  `first_pass.c:138`, `:152`, `:176`, `:184`, `:192`,
  `line_analysis.c:77-84`, `memory_builder.c:143-165`,
  `second_pass.c:70-78`. A directive name inside a string, or as the prefix
  of a longer word, selects the directive.
- SPEC: DIR-1, DIR-2, SYN-22, SYN-17, ERR-18, ERR-36.
- Observed (`string_punctuation`): `S6: .string "X: .data 1,2 ; mov r1, r2"`
  is handled as `.data`. `errors/missing_space_after_directive`:
  `.data1,2`, `.string"x"`, `.mat[5][2]` and `X: .data5` produce no message.
- Tests: `valid/string_punctuation`, `errors/missing_space_after_directive`.
- Fix direction: take the first field after the optional label and compare
  it as a whole word. Size: medium (four call sites; one shared classifier).

#### BUG-27 A symbol named by two `.entry` lines appears twice in `.ent`
- Severity: wrong output for valid input.
- Root cause: `second_pass.c:272-274` appends to the entry list on every
  `.entry` line without looking for an existing node.
- SPEC: DIR-34, OUT-8.
- Observed (`entry_extern_repeated`): `MAIN bcba` twice. Expected: once.
- Tests: `valid/entry_extern_repeated`.
- Fix direction: write `.ent` from the symbol table flags, or skip a name
  already in the list. Size: small.

### Valid input rejected

#### BUG-28 The table of legal addressing modes is wrong for lea, jmp, bne and jsr
- Severity: valid input rejected.
- Root cause: `instruction_validation.c:146` allows only direct and register
  as the destination of `lea` (matrix is missing); `:166-169` allows only
  direct and matrix for `jmp`, `bne`, `jsr` (register is missing). The
  legality lives in `strcmp` chains, not in the instruction table
  (`instruction_table.c:8-24` has no mode columns).
- SPEC: INS-4, section 3; CLAUDE.md (one constant table).
- Observed (`modes_lea`): `Error on line 2: Destination for 'lea' must be
  direct/register.` (`modes_one_operand`): `Error on line 15: Operand for
  'jmp' must be direct/matrix/register (not immediate).` Both are legal.
- Tests: `valid/modes_lea`, `valid/modes_one_operand` (today only a message,
  because of BUG-06).
- Fix direction: add legal source and destination mode sets to the
  instruction table and validate from them. Size: medium.

#### BUG-29 An indented label is rejected by the first pass
- Severity: valid input rejected (masked today by the crash of BUG-10).
- Root cause: `line_analysis.c:10-24` (`extract_label`) copies everything
  from the start of the line to the colon, including leading whitespace, and
  `is_valid_label` then fails on the first character. The same function
  treats a colon inside a string of an unlabelled line as a label end.
- SPEC: SYN-19, LBL-2, DIR-28.
- Observed: found by reading; `  MAIN: inc r1` aborts earlier in the
  pre-assembler (BUG-10), so the first pass is not reached today.
- Tests: none.
- Fix direction: skip leading whitespace and accept a label only when the
  first field ends with the colon. Size: small.

#### BUG-30 The memory overflow check has the wrong bound and is made per line
- Severity: valid input rejected; invalid input accepted.
- Root cause: `first_pass.c:236` reports when `IC > 255`, but IC is the next
  free address and 256 is legal. `first_pass.c:181`, `:189`, `:197` test
  `IC + DC > 256` only on data lines with the IC reached so far, so code
  that follows the data is never checked against the data already counted.
- SPEC: ALG-15, ERR-41, M-4.
- Observed: 78 lines of `inc r1` (156 words, last address 255): `Error line
  78: Memory overflow - instruction area exceeds available memory`.
  Expected: no error. A 10-value `.data` line followed by 75 lines of
  `inc r1` (160 words): no message, an `.ob` with 160 words. Expected:
  ERR-41.
- Tests: none fails only for this (`errors/memory_overflow` is detected;
  `valid/memory_exactly_full` has only 6 code words).
- Fix direction: check once after the last line that code words plus data
  words do not exceed 156. Size: small.

### Invalid input accepted

#### BUG-31 `.entry` creates a placeholder symbol in the first pass
- Severity: invalid input accepted; wrong or missing diagnostic.
- Root cause: `first_pass.c:138-149` calls `add_symbol_to_table(name, 0,
  CODE, line, 1)` for every `.entry` line; `symbol_table.c:88-101` creates a
  symbol with address 0 when the name is unknown. To let the definition
  come later, "address 0" is treated as "not yet defined"
  (`first_pass.c:129`, `symbol_table.c:68-74`).
- SPEC: DIR-19, ERR-51, ERR-52, ERR-50, LBL-5.
- Consequences: (a) `.entry` of an undefined symbol is not an error and is
  written to `.ent` with address 0; (b) an operand that names such a symbol
  resolves to address 0 instead of ERR-50; (c) `.entry Y` followed by
  `.extern Y` is reported on the `.extern` line; (d) the first data label of
  a file has address 0 until relocation, so a second definition of it is
  accepted by the first pass (found by reading; today the pre-assembler of
  BUG-09 catches the duplicate first).
- Observed (`errors/entry_undefined`): no message, status 0, `.ent` holds
  `NOWHERE aaaa` and `main aaaa`. Expected: errors on lines 2 and 5.
  `.entry GHOST` / `jmp GHOST` / `stop`: status 0, word `aaaac`.
  `errors/entry_and_extern`: message for line 7. Expected: line 5.
- Tests: `errors/entry_undefined`, `errors/entry_and_extern`,
  `errors/line_numbers_second_pass_macro` (line 9).
- Fix direction: ignore `.entry` in the first pass (ALG-9 step 9) and give
  the symbol an explicit "defined" state. Size: medium.

#### BUG-32 A line that is neither a known directive nor a known operation is dropped silently
- Severity: invalid input accepted.
- Root cause: `first_pass.c:241-243`: the final `else` only sets
  `has_errors`, prints nothing, and does not enter the label of the line
  into the symbol table. `is_command` (`line_analysis.c:86-135`) is the
  gate.
- SPEC: ERR-17, ERR-18, ERR-45, ERR-35, ERR-R3, LBL-23, MAC-32.
- Observed (`errors/unknown_operation`): no message at all, status 0, an
  `.ob` without the bad lines. Expected: five errors. The same for unknown
  directives (`.foo 1`), uppercase names (`MOV r1,r2`), a label alone on a
  line (`LABEL:`), `prn#5`, and a macro name left in a macro body.
- Tests: `errors/unknown_operation`, `unknown_directive`, `label_alone`,
  `missing_space_after_operation`, `macro_nested_call_not_expanded`,
  `first_pass_error_hides_second_pass`, `line_numbers_after_macro`,
  `line_numbers_in_macro_body`.
- Fix direction: report the line (unknown operation, unknown directive, or
  empty statement after a label) and still define its label. Size: small.

#### BUG-33 The value lists of `.data` and `.mat` are not validated
- Severity: invalid input accepted.
- Root cause: `first_pass.c:176-183` and `:192-199` only count;
  `line_analysis.c:137-162` counts non-empty comma fields;
  `memory_builder.c:220` and `:321` convert with `atoi`. There is no check
  for an empty list, a non-integer, a misplaced comma, the range or the
  number of matrix values.
- SPEC: DIR-5, DIR-6, SYN-20, SYN-21, DIR-25; ERR-28, ERR-29, ERR-30,
  ERR-39, ERR-40.
- Observed (`errors/data_no_values`, `data_value_not_integer`,
  `data_comma_errors`, `data_out_of_range`, `mat_too_many_values`): no
  message, status 0, `.ob` written. `.data 9999` is stored modulo 1024.
- Tests: those five cases.
- Fix direction: one function that parses a comma-separated integer list
  with range and count limits, used by both directives. Size: medium.

#### BUG-34 The operand of `.string` is not validated
- Severity: invalid input accepted.
- Root cause: `first_pass.c:184-191`, `line_analysis.c:241-278`: no check
  for the opening or closing quote, for text outside the quotes, or for
  characters outside ASCII 32..126 (such characters are skipped or end the
  string, `line_analysis.c:260`, `:271`).
- SPEC: DIR-9, SYN-22; ERR-31, ERR-32.
- Observed (`errors/string_malformed`, `string_illegal_char`): no message,
  status 0, `.ob` written.
- Tests: those two cases.
- Fix direction: validate the quotes and every character between them.
  Size: medium (same function as BUG-22).

#### BUG-35 The dimensions of `.mat` are not validated
- Severity: invalid input accepted.
- Root cause: `line_analysis.c:164-174`: whatever `sscanf` cannot read
  leaves 0 rows or columns; a negative product is added to DC
  (`first_pass.c:195`); `memory_builder.c:296-306` only looks for two `]`.
- SPEC: DIR-12, DIR-26, DIR-27, DIR-33; ERR-33, ERR-44.
- Observed (`errors/mat_dimensions_malformed`, `mat_dimension_invalid`): no
  message, status 0, `.ob` written.
- Tests: those two cases.
- Fix direction: parse `[digits][digits]` strictly, value at least 1,
  nothing between `]` and `[`. Size: medium (same function as BUG-24).

#### BUG-36 The operand of `.entry` and `.extern` is not validated
- Severity: invalid input accepted.
- Root cause: `first_pass.c:140` and `:154` read the second
  whitespace-separated word with `sscanf("%*s %30s")` and accept it as it
  is; `second_pass.c:246-247` does the same with `strtok`. Extra operands
  are ignored, the name is not checked (`.extern A, B` defines a symbol
  named `A,`), and a name longer than 30 characters is cut.
- SPEC: DIR-17, DIR-20, ERR-34, LBL-2.
- Observed (`errors/entry_extern_operand`): only lines 4 and 5 (no operand)
  are reported. Expected: also 7 to 12.
- Tests: `errors/entry_extern_operand`,
  `errors/missing_space_after_directive` (lines 7 and 8 are reported only by
  accident, as "missing operand").
- Fix direction: require exactly one field that is a legal label name.
  Size: small.

#### BUG-37 Immediate operands are not validated
- Severity: invalid input accepted.
- Root cause: `line_analysis.c:226-229` (`is_immediate`) tests only the
  leading `#`; `memory_builder.c:439` and `:484` convert with `atoi`. No
  check for a missing number, trailing characters or the range.
- SPEC: ADR-2, SYN-20, SYN-21; ERR-24, ERR-39.
- Observed (`errors/immediate_malformed`, `immediate_out_of_range`): no
  message, status 0, `.ob` written. `prn #5x` and `prn #` are accepted.
- Tests: those two cases.
- Fix direction: parse sign and digits strictly and check -128..127.
  Size: small.

#### BUG-38 The operand list of an instruction is not checked for commas, extra text or a third operand
- Severity: invalid input accepted.
- Root cause: `line_analysis.c:185-219` (`parse_operands`) splits at the
  first comma and reads one whitespace-delimited token on each side;
  everything else on the line is ignored. A second comma stays inside the
  second token and the token is then taken for a label.
- SPEC: SYN-13, SYN-14; ERR-19, ERR-26, ERR-27, ERR-37, ERR-43.
- Observed (`errors/too_many_operands`): `mov r1, r2, r3` and `cmp X, X, X`
  produce no message. `errors/comma_errors_operands`: lines 4, 6, 9 and 11
  produce no first-pass message. `extra_text_after_instruction`:
  `inc r1 r2` and `mov r1, r2 r3` pass the first pass.
  `matrix_operand_whitespace`: `mov M1 [r2][r7], r1` passes the first pass
  (operand read as `M1`). `semicolon_not_first`: `mov r1, r2 ; not a
  comment` and `inc r1;` pass the first pass. Some of these lines are later
  reported by the second pass as `Undefined symbol 'r1 r2'`, which is an
  accident of BUG-06 and disappears when that bug is fixed.
- Tests: `errors/comma_errors_operands`, `extra_text_after_instruction`,
  `too_many_operands`, `matrix_operand_whitespace`, `semicolon_not_first`.
- Fix direction: a real operand scanner (operand, comma, operand, end of
  line) that reports each deviation. Size: medium to large; it is the same
  parser that BUG-25 needs.

#### BUG-39 Matrix operand validation has gaps
- Severity: invalid input accepted.
- Root cause: `instruction_validation.c:48-75` checks the first two bracket
  pairs only. It does not require a label before the first `[`, does not
  require the second `[` to follow the first `]` directly, and ignores
  anything after the second `]`.
- SPEC: ADR-6, SYN-18, SYN-23; ERR-25, ERR-42.
- Observed (`errors/matrix_operand_malformed`): `inc M[r1][r2][r3]` (line 4)
  gives no message. `errors/matrix_operand_no_label`: `mov [r2][r7], r1`
  passes the first pass; the message seen today (`Undefined symbol ''`)
  comes from the second pass.
- Tests: `errors/matrix_operand_malformed`, `matrix_operand_no_label`.
- Fix direction: validate the whole operand as `label[reg][reg]` with
  nothing else. Size: small.

#### BUG-40 Missing whitespace after the colon of a label is accepted
- Severity: invalid input accepted.
- Root cause: `first_pass.c:214-221` and `line_analysis.c:104-122` skip to
  the character after the colon and accept the statement whether or not
  whitespace follows.
- SPEC: LBL-18, ERR-38.
- Observed: `X:.data 5` / `prn X` / `stop` assembles with status 0. In
  `errors/label_no_space_after_colon` lines 2 and 5 are reported only by the
  second pass (as "Missing instruction name"), line 4 not at all.
- Tests: `errors/label_no_space_after_colon`.
- Fix direction: require a whitespace character after the colon.
  Size: small.

#### BUG-41 A semicolon after leading whitespace is treated as a comment
- Severity: invalid input accepted.
- Root cause: `line_analysis.c:46-53` (`is_comment_or_empty`) and `:361-371`
  (`is_comment_line`) skip whitespace before testing for `;`. In addition
  `is_comment_or_empty` knows only space and tab as whitespace, while SPEC
  asks for `isspace`.
- SPEC: SYN-5, SYN-9, SYN-8, ERR-43.
- Observed (`errors/semicolon_not_first`): lines 3 and 6 produce no message
  and are missing from the `.am`. Expected: an error on each, and both
  lines present in the `.am`.
- Tests: `errors/semicolon_not_first`.
- Fix direction: a comment is a line whose first character is `;`.
  Size: small.

#### BUG-42 The directive names without the dot are missing from the reserved words
- Severity: invalid input accepted.
- Root cause: `macro_and_label_func.c:18-19` lists `.data`, `.string`,
  `.mat`, `.entry`, `.extern` with the dot, which can never equal a
  candidate name.
- SPEC: LBL-20, MAC-33, ERR-2, ERR-13.
- Observed (`errors/macro_name_reserved`): `mcro data` (line 12) and
  `mcro extern` (line 18) are accepted. `errors/label_reserved_word`: lines
  7, 8, 10, 11 and 12 are accepted.
- Tests: `errors/macro_name_reserved`, `errors/label_reserved_word`.
- Fix direction: list the five names without the dot. Size: small.

#### BUG-43 A label of 31 or more characters is silently ignored by the first pass
- Severity: invalid input accepted.
- Root cause: `line_analysis.c:16` returns 0 ("no label") when the text
  before the colon is 31 characters or longer, so no error is raised and no
  symbol is defined; the statement is then processed as unlabelled.
- SPEC: LBL-3, ERR-12.
- Observed (`errors/label_too_long`, line 2, 31 characters): no message
  from any stage. Expected: an error on line 2.
- Tests: `errors/label_too_long` (together with BUG-12 and BUG-09).
- Fix direction: recognise the label whatever its length and report it when
  it is too long. Size: small.

#### BUG-44 After the first pre-assembler error, later macro definitions are not recorded
- Severity: wrong or missing diagnostic.
- Root cause: `preassembler.c:155` starts collecting a macro only
  `if (!has_errors)`. Once any error has been seen, later macros never
  enter the table, so a later duplicate definition, a later call with extra
  text and a later label with that name are not detected.
- SPEC: MAC-22, MAC-28, ERR-8, ERR-7.
- Observed (`errors/macro_duplicate_name`): only line 6 is reported.
  Expected: 6 and 13.
- Tests: `errors/macro_duplicate_name`, `errors/label_same_as_macro` (line
  15).
- Fix direction: keep recording valid definitions after an error.
  Size: small.

#### BUG-45 Extra text after `mcroend` does not close the macro
- Severity: wrong or missing diagnostic (latent).
- Root cause: `preassembler.c:183-186` reports and `continue`s before
  `inside_macro = FALSE` (`:237`), so the following ordinary lines are
  collected into the body of the unfinished macro and the macro is never
  stored.
- SPEC: MAC-31, ERR-4.
- Observed: found by reading. `errors/macro_end_extra_text` reports the
  right two lines, and the `.am` is deleted anyway, so nothing visible
  differs today; a later call of that macro would not be recognised.
- Tests: none fails because of it.
- Fix direction: close and store the macro before reporting. Size: small.

#### BUG-46 A label in front of a macro call is expanded instead of reported
- Severity: invalid input accepted.
- Root cause: `preassembler.c:403-441` writes the label followed by the
  first body line.
- SPEC: MAC-34, MAC-21, ERR-9 (the reporting stage is still open, Q-28).
- Observed: `L: m` with macro `m` gives `L:  inc r1` in the `.am`, status 0.
  (`L: mcro x` is reported by the pre-assembler, `preassembler.c:292-301`.)
- Tests: none (Q-28).
- Fix direction: report it in the stage the owner chooses. Size: small.
  Blocked on Q-28.

---

## 2. Order of work

1. BUG-01, then BUG-02, BUG-03, BUG-04: after these the `.am` comparison of
   every valid case is meaningful and 3 valid cases pass.
2. BUG-05, BUG-06 (with BUG-28 and BUG-30, which BUG-06 turns into
   rejections), BUG-08: after these the error cases compare real
   diagnostics and the cli cases are isolated.
3. BUG-10 to BUG-14 (crashes and memory).
4. BUG-15 to BUG-27 (encoding). BUG-15, 16, 17 and 19 together touch 51 of
   the 53 valid cases with wrong words.
5. BUG-09 with BUG-42, BUG-43, BUG-29, BUG-31, then BUG-07.
6. BUG-32 to BUG-41, BUG-44, BUG-45; BUG-46 after Q-28.

---

## 3. The 124 tests and the bugs that make each fail

BUG-01 applies to every row when the binary is built with `make` in the
repository and is not repeated. The rows for the valid and cli cases were
generated from the expected `.am` files (see "Limits" at the top); the rows
for the error cases were assigned by hand from the observed messages.
BUG-11, 13, 14, 29, 30, 45 and 46 make no test fail.

| Test | Bugs that make it fail |
|---|---|
| `valid/all_instructions` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-17, BUG-19 |
| `valid/blank_lines` | BUG-02, BUG-03, BUG-15 |
| `valid/code_data_interleaved` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-21, BUG-22, BUG-23 |
| `valid/comment_lines` | BUG-02, BUG-03, BUG-15 |
| `valid/data_values` | BUG-02, BUG-03, BUG-16, BUG-21 |
| `valid/data_without_label` | BUG-02, BUG-15, BUG-21, BUG-22, BUG-23 |
| `valid/entry_after_definition` | BUG-02, BUG-15, BUG-16, BUG-22 |
| `valid/entry_and_extern` | BUG-02, BUG-15, BUG-16, BUG-17 |
| `valid/entry_extern_repeated` | BUG-02, BUG-15, BUG-27 |
| `valid/entry_only` | BUG-02, BUG-03, BUG-15 |
| `valid/example1` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-17, BUG-19, BUG-22 |
| `valid/extern_after_use` | BUG-02, BUG-15, BUG-16 |
| `valid/extern_declared_unused` | BUG-02, BUG-15 |
| `valid/extern_multiple_uses` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-17, BUG-18, BUG-19 |
| `valid/extern_only` | BUG-02, BUG-03, BUG-15 |
| `valid/forward_references` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-22 |
| `valid/immediate_bounds` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-17 |
| `valid/label_30_chars` | BUG-02, BUG-15, BUG-16, BUG-21 |
| `valid/label_case_sensitive` | BUG-02, BUG-15 |
| `valid/label_names_legal` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-20 |
| `valid/labels_any_statement_kind` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-22 |
| `valid/line_80_chars` | BUG-02, BUG-03, BUG-04, BUG-16, BUG-17, BUG-21, BUG-22 |
| `valid/macro_blank_and_comment_lines` | BUG-02, BUG-03, BUG-15 |
| `valid/macro_called_twice` | BUG-02, BUG-15, BUG-17 |
| `valid/macro_name_not_first_field` | BUG-02, BUG-03, BUG-15, BUG-22 |
| `valid/macro_names` | BUG-02, BUG-15 |
| `valid/macro_several` | BUG-02, BUG-15 |
| `valid/macro_single` | BUG-02, BUG-15, BUG-16, BUG-17 |
| `valid/macro_whitespace` | BUG-02, BUG-15 |
| `valid/macro_with_directives` | BUG-02, BUG-16, BUG-21, BUG-22, BUG-23 |
| `valid/mat_full` | BUG-02, BUG-03, BUG-15, BUG-16 |
| `valid/mat_no_values` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-23 |
| `valid/mat_partial` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-23 |
| `valid/mat_spacing` | BUG-02, BUG-03, BUG-15, BUG-23, BUG-24 |
| `valid/memory_exactly_full` | BUG-02, BUG-03, BUG-18, BUG-23 |
| `valid/modes_add_sub` | BUG-02, BUG-03, BUG-16, BUG-17, BUG-18, BUG-19, BUG-23 |
| `valid/modes_cmp` | BUG-02, BUG-03, BUG-16, BUG-17, BUG-18, BUG-19 |
| `valid/modes_lea` | BUG-02, BUG-03, BUG-16, BUG-18, BUG-22, BUG-23, BUG-28 |
| `valid/modes_mov` | BUG-02, BUG-03, BUG-16, BUG-17, BUG-18, BUG-19 |
| `valid/modes_one_operand` | BUG-02, BUG-03, BUG-15, BUG-17, BUG-23, BUG-28 |
| `valid/no_entry_no_extern` | BUG-02 |
| `valid/no_macros` | BUG-02, BUG-03, BUG-16, BUG-17, BUG-21 |
| `valid/no_operand_instructions` | BUG-02 |
| `valid/no_stop` | BUG-02, BUG-15 |
| `valid/numbers_leading_zeros` | BUG-02, BUG-03, BUG-15, BUG-17, BUG-21 |
| `valid/only_code` | BUG-02 |
| `valid/only_data` | BUG-02, BUG-21 |
| `valid/ps` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-17, BUG-19, BUG-22 |
| `valid/register_word_sharing` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-17, BUG-18, BUG-19, BUG-23 |
| `valid/string_basic` | BUG-02, BUG-03, BUG-15, BUG-16, BUG-22 |
| `valid/string_empty` | BUG-02, BUG-03, BUG-15, BUG-22 |
| `valid/string_inner_quote` | BUG-02, BUG-03, BUG-15, BUG-22 |
| `valid/string_punctuation` | BUG-02, BUG-03, BUG-15, BUG-22, BUG-26 |
| `valid/whitespace_commas` | BUG-02, BUG-17, BUG-21 |
| `valid/whitespace_matrix_brackets` | BUG-02, BUG-15, BUG-16, BUG-18, BUG-19, BUG-23, BUG-25 |
| `valid/whitespace_tabs` | BUG-02, BUG-15, BUG-16, BUG-17, BUG-21, BUG-22 |
| `errors/comma_errors_operands` | BUG-05, BUG-06, BUG-38 |
| `errors/data_comma_errors` | BUG-05, BUG-06, BUG-33 |
| `errors/data_no_values` | BUG-05, BUG-06, BUG-33 |
| `errors/data_out_of_range` | BUG-05, BUG-06, BUG-33 |
| `errors/data_value_not_integer` | BUG-05, BUG-06, BUG-33 |
| `errors/dest_mode_illegal` | BUG-05, BUG-06 |
| `errors/entry_and_extern` | BUG-05, BUG-31 |
| `errors/entry_extern_operand` | BUG-05, BUG-06, BUG-36 |
| `errors/entry_undefined` | BUG-05, BUG-31 |
| `errors/extern_then_label` | BUG-05, BUG-06 |
| `errors/extra_text_after_instruction` | BUG-05, BUG-06, BUG-38 |
| `errors/first_pass_error_hides_second_pass` | BUG-05, BUG-06, BUG-32 |
| `errors/immediate_malformed` | BUG-05, BUG-06, BUG-37 |
| `errors/immediate_out_of_range` | BUG-05, BUG-06, BUG-37 |
| `errors/label_alone` | BUG-04, BUG-05, BUG-06, BUG-32 |
| `errors/label_duplicate` | BUG-05, BUG-09 |
| `errors/label_invalid_chars` | BUG-05, BUG-09 |
| `errors/label_no_space_after_colon` | BUG-05, BUG-06, BUG-40 |
| `errors/label_on_error_line_is_defined` | BUG-05, BUG-09 |
| `errors/label_reserved_mcro` | BUG-05, BUG-09 |
| `errors/label_reserved_word` | BUG-05, BUG-09, BUG-42 |
| `errors/label_same_as_macro` | BUG-05, BUG-07, BUG-09, BUG-44 |
| `errors/label_space_before_colon` | BUG-05, BUG-06 |
| `errors/label_then_extern` | BUG-05, BUG-06 |
| `errors/label_too_long` | BUG-05, BUG-09, BUG-12, BUG-43 |
| `errors/line_numbers_after_macro` | BUG-03, BUG-05, BUG-06, BUG-07, BUG-32 |
| `errors/line_numbers_in_macro_body` | BUG-05, BUG-06, BUG-07, BUG-32 |
| `errors/line_numbers_second_pass_macro` | BUG-05, BUG-07, BUG-31 |
| `errors/line_too_long` | BUG-05 |
| `errors/macro_call_extra_text` | BUG-05 |
| `errors/macro_def_extra_text` | BUG-05 |
| `errors/macro_duplicate_name` | BUG-05, BUG-44 |
| `errors/macro_end_extra_text` | BUG-05 |
| `errors/macro_name_invalid` | BUG-05 |
| `errors/macro_name_missing` | BUG-05 |
| `errors/macro_name_reserved` | BUG-05, BUG-42 |
| `errors/macro_name_reserved_mcro` | BUG-05 |
| `errors/macro_name_too_long` | BUG-05, BUG-12 |
| `errors/macro_nested_call_not_expanded` | BUG-05, BUG-06, BUG-07, BUG-32 |
| `errors/mat_dimension_invalid` | BUG-05, BUG-06, BUG-35 |
| `errors/mat_dimensions_malformed` | BUG-05, BUG-06, BUG-35 |
| `errors/mat_too_many_values` | BUG-05, BUG-06, BUG-33 |
| `errors/matrix_index_not_register` | BUG-05, BUG-06 |
| `errors/matrix_operand_malformed` | BUG-05, BUG-06, BUG-39 |
| `errors/matrix_operand_no_label` | BUG-05, BUG-06, BUG-39 |
| `errors/matrix_operand_whitespace` | BUG-05, BUG-06, BUG-25, BUG-38 |
| `errors/memory_overflow` | BUG-03, BUG-05, BUG-06 |
| `errors/missing_space_after_directive` | BUG-05, BUG-06, BUG-26, BUG-36 |
| `errors/missing_space_after_operation` | BUG-05, BUG-06, BUG-32 |
| `errors/preassembler_error_hides_pass_errors` | BUG-05 |
| `errors/semicolon_not_first` | BUG-03, BUG-05, BUG-06, BUG-38, BUG-41 |
| `errors/source_mode_illegal` | BUG-05, BUG-06 |
| `errors/string_illegal_char` | BUG-05, BUG-06, BUG-34 |
| `errors/string_malformed` | BUG-05, BUG-06, BUG-34 |
| `errors/too_few_operands` | BUG-05, BUG-06 |
| `errors/too_many_operands` | BUG-05, BUG-06, BUG-38 |
| `errors/undefined_symbol` | BUG-05 |
| `errors/unknown_directive` | BUG-05, BUG-06, BUG-32 |
| `errors/unknown_operation` | BUG-05, BUG-06, BUG-32 |
| `cli/as_suffix_argument` | BUG-05 |
| `cli/error_then_valid` | BUG-02, BUG-03, BUG-05, BUG-06, BUG-08, BUG-16, BUG-17 |
| `cli/missing_file_last` | BUG-02, BUG-03, BUG-05, BUG-16, BUG-17 |
| `cli/missing_file_only` | BUG-05 |
| `cli/no_arguments` | passes with the scratch build; fails in the repository only because of BUG-01 |
| `cli/preassembler_error_then_valid` | BUG-02, BUG-03, BUG-05, BUG-16, BUG-17 |
| `cli/same_names_in_two_files` | BUG-02, BUG-08, BUG-15, BUG-22 |
| `cli/several_valid_files` | BUG-02, BUG-08, BUG-15 |
| `cli/valid_then_error` | BUG-02, BUG-05, BUG-06, BUG-08, BUG-15 |

---

## 4. Behaviour that is correct today and must not regress

Each item was observed in a run, not only read in the code.

- Opcodes and operand counts of all 16 operations (`instruction_table.c`).
- Base-4 conversion with the digits `a` to `d`; addresses with four digits,
  words with five; the header without leading `a`; zero printed as `a`
  (`only_code`, `only_data` after the separator is discounted).
- IC starts at 100; the data image follows the code with no gap; data labels
  are relocated by ICF.
- First word layout of two-operand instructions (opcode, source mode,
  destination mode).
- Two register operands share one word; a source register goes to bits 9-6;
  the index word of a source matrix operand is correct.
- A label word is `address << 2 | 2`; an external word is `aaaab`; `.ext`
  gets one line per reference (addresses correct when the source is not a
  register); no `.ent` or `.ext` is created when it would be empty; `.extern`
  declared and unused gives no `.ext`.
- Forward references, `.extern` after its use, `.entry` before and after the
  definition, repeated `.extern`.
- Case-sensitive labels; `PSW`, `PC`, `R1`, `MOV`, `r8`, `r20` accepted as
  label names; a 30-character label works in direct and matrix operands and
  in `.entry`.
- Macro expansion: definition lines removed, calls replaced by the body,
  several calls of one macro, a body of more than 10 lines, an empty body,
  whitespace around `mcro`, the name, `mcroend` and the call; a macro name
  that is not the first field is not expanded; a macro name inside another
  body is not expanded.
- Pre-assembler errors with correct `.as` line numbers: reserved macro name
  (operations, registers, `mcro`, `mcroend`), extra text after the macro
  name, extra text after `mcroend`, missing name, invalid name, extra text
  on a call line, the first duplicate definition, lines longer than 80
  characters (80 accepted, 81 rejected, also in a macro body and in a
  comment). The scan continues after an error, the `.am` is removed, the
  passes do not run, the next file is processed.
- First-pass detection that exists and reports the right `.am` line: wrong
  operand count for too few operands, immediate destination, illegal source
  of `lea`, matrix index that is not `r0`..`r7`, whitespace before the colon
  of a label, label after `.extern` of the same name, `.extern` after a
  label of the same name.
- Second-pass detection of undefined operand symbols (direct, matrix, `r8`,
  wrong letter case) and of `.entry` on an external symbol.
- Command line: no arguments gives status 0; a missing file gives status 1
  and the other files are still processed; the argument `prog.as` looks for
  `prog.as.as`.
- No leak and no invalid access on the pre-assembler error paths (valgrind
  clean for `macro_name_reserved`, `label_duplicate`, `line_too_long`).

---

## 5. valgrind results

Command: `valgrind --leak-check=full --show-leak-kinds=all ./assembler
<args>` in a directory holding only the `.as` files, with the scratch `-g`
build.

| Arguments | Errors | Leaks |
|---|---|---|
| `example1` | 0 | 864 bytes in 8 blocks still reachable (symbol table, BUG-08; first-pass `FILE`, BUG-13) |
| `ps` | 23 errors in 6 contexts (BUG-11) | 976 bytes in 10 blocks still reachable |
| `macro_called_twice` | 0 | 528 bytes in 2 blocks still reachable |
| `macro_name_reserved` | 0 | none |
| `label_duplicate` | 0 | none |
| `line_too_long` | 0 | none |
| `undefined_symbol` | 0 | 584 bytes in 3 blocks still reachable |
| `too_few_operands` | 0 | 640 bytes in 4 blocks still reachable |
| `alpha beta gamma` (cli/several_valid_files) | 28 errors in 5 contexts (BUG-11) | 1,584 bytes in 6 blocks still reachable |
| `nosuch` | 0 | none |
| 25-line macro body (probe) | 0 | 472 bytes in 1 block still reachable |
| `  MAIN: inc r1` (probe) | 4 errors in 4 contexts, invalid free (BUG-10), abort | 14 bytes in 2 blocks definitely lost |
| 31-character macro name (probe) | 0 reported (the one-byte stack overflow of BUG-12 is found only by the sanitizer build) | 472 bytes in 1 block still reachable |

Sanitizer build (`-fsanitize=address,undefined`): bad free at
`preassembler.c:310` and `:258` (BUG-10), heap use after free at
`output_writer.c:231` (BUG-11), stack buffer overflow in `strcpy` at
`preassembler.c:157` (BUG-12). No undefined-behaviour report from UBSan on
the 25 probes.

---

## 6. State of the modules

The pre-assembler, the symbol table, the output writer and the driver can be
repaired bug by bug: their bugs are local (BUG-02 to BUG-04, BUG-08, BUG-10
to BUG-13, BUG-27, BUG-42, BUG-44, BUG-45).

The statement-handling cluster (`first_pass.c`, `line_analysis.c`,
`instruction_validation.c`, `memory_builder.c`, and the operand code in
`second_pass.c`) is in a different state. 27 of the 46 bugs (BUG-15 to
BUG-26, BUG-28 to BUG-41, BUG-43) are in it, and most share one cause: there
is no parser. The same line is tokenised three times in three incompatible
ways (`sscanf("%s")` in `line_analysis.c:185-219` and
`instruction_validation.c:278`, `strtok` on blanks and commas in
`memory_builder.c:357-403`, and again in `second_pass.c:132-185`), the
statement kind is found with `strstr`, numbers are read with `atoi`, and
the first pass computes lengths with one set of rules while the memory
builder emits words with another, so addresses and image drift apart
(BUG-21, 22, 23, 25). Validation of directives does not exist at all
(BUG-33 to BUG-36). Fixing these one at a time means writing the same
scanner piece by piece inside five functions and keeping three copies in
step. One line parser that produces a parsed statement (label, kind,
operands with mode and value), used by both passes, would close most of
these bugs at once and is what the coding rules in CLAUDE.md ask for. This
is a rewrite of a module group and therefore needs the owner's approval
(CLAUDE.md, phase 3).

---

## 7. Remarks on SPEC and on the tests

No expected file was found to contradict SPEC in the roughly 20 cases whose
diffs were examined word by word. The following points are open or worth a
decision; none is a proven error.

1. Q-28 (stage of ERR-9) blocks BUG-46. Today the program is inconsistent:
   `L: mcro x` is a pre-assembler error and `L: m` is expanded.
2. Q-32, observed behaviour for the record: a last line without a newline
   gets one in the `.am`; a CR LF file fails in the second pass with
   `Unknown instruction 'stop<CR>'`; a file with only comments produces an
   `.ob` holding `a a`. The first and third are harmless, the second will be
   a real user problem for a public demo, so a decision would help.
3. Q-26: the program prints a large amount of progress and debug text on
   stdout (`Processing file`, `DEBUG ...`, the whole symbol table). The
   runner ignores it, but lines that begin `Error on line` appear on stdout
   too. When BUG-05 is fixed the debug output should be removed in the same
   step, otherwise a test cannot tell a stray debug line from a diagnostic.
4. ERR-R9 and Q-29: several lines get two messages today (first pass and
   second pass for the same line). This disappears with BUG-06; no test
   change is needed.
5. LBL-23 says a label on an erroneous line is still defined. The suite
   checks this only for an instruction with too few operands
   (`label_on_error_line_is_defined`). There is no case for a label on a
   line with an unknown operation or a bad directive, which is exactly
   where the present code does not define the label (BUG-32).
6. Not covered by any test, and all broken today: an indented label
   (BUG-10, BUG-29); an unlabelled `.string` whose text contains a colon
   (BUG-10); 156 words of code only (BUG-30); overflow when the data comes
   before the code (BUG-30); a second definition of the first data label
   (BUG-31 d); a symbol that is only named by `.entry` and used as an
   operand (BUG-31 b). The test-engineer should add cases for them.
7. The repository root holds two empty files, `This` and
   `instruction_count`, and a pair `formal_tester.as` / `formal_tester.ob`
   whose `.ob` was produced by the buggy program. They are not correctness
   bugs but should not be mistaken for expected output.
