# Test suite

Black-box tests for the assembler. Every case runs the real binary on input
files and compares what it leaves behind with expected files. All expected
results are derived from `docs/SPEC.md`, never from the behaviour of the
binary under test, so a failing case points at the assembler (or at SPEC),
not at the test. Rule IDs below refer to `docs/SPEC.md`.

## Running

```
make test                              # build ./assembler, then run everything
sh tests/run_tests.sh                  # run everything against ./assembler
sh tests/run_tests.sh ps               # run one case by name
sh tests/run_tests.sh errors/label_alone cli/no_arguments
ASSEMBLER=/path/to/binary sh tests/run_tests.sh
```

The runner is written in POSIX sh. It prints one line per case,
`PASS <group>/<case>` or `FAIL <group>/<case>: <short reason>`, and ends with
`passed N, failed M, total T`. Its exit status is 0 only if nothing failed
(1 if a case failed, 2 if there is no binary to run).

Each case runs in a fresh scratch directory, `tests/out/<group>/<case>/`,
that receives only the `.as` files of the case; this keeps the source tree
clean and makes "this file must not be created" checkable (SPEC Q-31). The
scratch directory of a passing case is deleted. For a failing case it is
kept and holds the files the assembler wrote, `stdout.txt`, `stderr.txt` and
`diff.txt` with every check that failed. `tests/out/` is ignored by git. A
run is limited to 5 seconds when the `timeout` command exists; a timeout or a
crash is a FAIL of that case and the run goes on.

## Layout

```
tests/
  run_tests.sh             the runner
  valid/<case>/            programs that must assemble
    <case>.as              input
    <case>.am              expected pre-assembler output (always present)
    <case>.ob              expected object file (always present)
    <case>.ent, <case>.ext expected only when SPEC says the file has content
  errors/<case>/           programs with errors
    <case>.as              input
    <case>.expected        expected diagnostics
    <case>.am              present only when the errors are found by the passes
  cli/<case>/              cases about the command line
    args                   the command-line arguments, separated by whitespace
    expected_status        the expected exit status
    expected_messages      expected diagnostics; no such file means none
    *.as                   inputs
    *.am *.ob *.ent *.ext  expected outputs
  out/                     scratch directories (generated, ignored by git)
  sh/                      older ad-hoc scripts; not used by this suite
```

The name of a case directory is also the name of its source file and the
argument given to the assembler (CLI-2).

## What is compared

Valid cases (`tests/valid`): the exit status is 0 (CLI-9); `.am` and `.ob`
are byte-identical to the expected files (MAC-29, OUT-13); `.ent` and `.ext`
hold exactly the expected lines, in any order (OUT-10), and end with a
newline; an `.ent` or `.ext` with no expected file must not exist (OUT-2);
no line of `stdout` is an error message.

Error cases (`tests/errors`): the exit status is 1 (CLI-9); no `.ob`, `.ent`
or `.ext` exists (ERR-R6); the `.am` file is byte-identical to the expected
one when the case has one (errors found by the passes, OUT-11) and does not
exist otherwise (errors found by the pre-assembler, MAC-23); and the
diagnostics match `<case>.expected`.

Diagnostics: only the lines of `stdout` that have the format of ERR-R13 or
ERR-R14 are looked at, `<name>.as:<line>: error: <text>`,
`<name>.as: error: <text>`, and the same with `warning:`. Each is reduced to
`<line> <severity>`, or `- <severity>` when it has no line number, and the
sorted list is compared with the sorted `<case>.expected`. The wording of a
message is never compared, other lines of `stdout` are ignored (Q-26), and
the order of the messages does not matter (Q-29). A message that names
another file than `<case>.as` never matches. The line number is the line of
the `.as` file (ERR-R12). A line `* error` in `<case>.expected` switches the
case to severity-only comparison: the line numbers, and whether a message
has one, are ignored. It is used where SPEC has not yet fixed the line
(Q-30).

Command-line cases (`tests/cli`): the exit status equals `expected_status`;
every expected output file matches as above; no other `.am`, `.ob`, `.ent`
or `.ext` file exists in the scratch directory; and the diagnostics, reduced
to `<file> <line> <severity>`, match `expected_messages`.

## Conventions for writing cases

- Every input ends with a newline, uses LF line endings and holds at least
  one instruction or data directive (Q-32).
- A line holds at most one error (Q-29). An error case mixes erroneous lines
  of one category with valid lines, so that "does not stop at the first
  error" (ERR-R4) is checked as well.
- A case holds errors of one stage only, because a pre-assembler error hides
  the errors of the passes and a first-pass error hides those of the second
  pass (SPEC 5.2, 5.3). Three cases check exactly this hiding.
- Expected `.ob`, `.ent` and `.ext` files are computed from SPEC sections 2
  and 3. The two booklet examples are copied from SPEC section 4.
- After a faulty `mcro` line the cases still close the definition with
  `mcroend` (MAC-7) and expect no further message for the body lines or for
  the `mcroend` line.

## Not tested yet (undecided in SPEC 6.1)

| Input or behaviour | Open question | What the suite does |
|---|---|---|
| ERR-9: a label in front of `mcro`, `mcroend` or a macro call | Q-28 (reporting stage) | No case contains such a line. |
| A label in front of `.entry` or `.extern` (DIR-18, DIR-22, the only warning in SPEC) | Q-27 | No case contains such a line; no case expects a warning. |
| ERR-41, memory overflow: line number of the message | Q-30 (a) | `errors/memory_overflow` asserts one error, without its line. |
| ERR-53 when the label comes before the `.extern` line: stage and line | Q-30 (b) | `errors/label_then_extern` asserts one error, without its line. The other order is fully asserted in `errors/extern_then_label`. |
| Text on `stdout` when there is no error; notice for a run with no arguments | Q-26 | Only lines in the diagnostic format are read. |
| Several messages for one line; order of the messages | Q-29 | One error per line; messages compared as an unordered collection. |
| Output files left from an earlier run | Q-31 | Every case starts in an empty directory. |
| Last line without a newline, CR LF line endings, a file with no statements | Q-32 | No such input. |
| A file that cannot be opened followed by other files | Q-33 | The missing file is the only or the last argument. |

Also outside the suite: the inputs that SPEC declares undefined (nested macro
definitions, a missing `mcroend`, a macro used before its definition, text
before `mcroend`; MAC-6, MAC-7, MAC-8, MAC-20) and allocation failure
(ERR-R10), which a test cannot provoke.

## Coverage

### Error categories (SPEC section 5)

| ERR | Cases |
|---|---|
| ERR-1 | `cli/missing_file_last`, `cli/missing_file_only`, `cli/as_suffix_argument` |
| ERR-2 | `errors/macro_name_reserved`, `errors/macro_name_reserved_mcro` |
| ERR-3 | `errors/macro_def_extra_text`, `errors/preassembler_error_hides_pass_errors` |
| ERR-4 | `errors/macro_end_extra_text` |
| ERR-5 | `errors/macro_name_missing` |
| ERR-6 | `errors/macro_name_invalid`, `errors/macro_name_too_long` |
| ERR-7 | `errors/macro_call_extra_text` |
| ERR-8 | `errors/macro_duplicate_name` |
| ERR-9 | not tested (Q-28) |
| ERR-10 | `errors/line_too_long` |
| ERR-11 | `errors/label_invalid_chars` |
| ERR-12 | `errors/label_too_long` |
| ERR-13 | `errors/label_reserved_word`, `errors/label_reserved_mcro` |
| ERR-14 | `errors/label_same_as_macro` |
| ERR-15 | `errors/label_duplicate`, `errors/label_on_error_line_is_defined` |
| ERR-16 | `errors/label_space_before_colon` |
| ERR-17 | `errors/unknown_operation`, `errors/macro_nested_call_not_expanded`, `errors/first_pass_error_hides_second_pass`, `errors/line_numbers_after_macro`, `errors/line_numbers_in_macro_body` |
| ERR-18 | `errors/unknown_directive` |
| ERR-19 | `errors/too_many_operands` |
| ERR-20 | `errors/label_on_error_line_is_defined`, `errors/too_few_operands`, `errors/line_numbers_after_macro`, `errors/line_numbers_in_macro_body` |
| ERR-21 | `errors/source_mode_illegal` |
| ERR-22 | `errors/dest_mode_illegal` |
| ERR-23 | `errors/matrix_index_not_register` |
| ERR-24 | `errors/immediate_malformed` |
| ERR-25 | `errors/matrix_index_not_register`, `errors/matrix_operand_malformed` |
| ERR-26 | `errors/comma_errors_operands` |
| ERR-27 | `errors/extra_text_after_instruction` |
| ERR-28 | `errors/data_no_values` |
| ERR-29 | `errors/data_value_not_integer` |
| ERR-30 | `errors/data_comma_errors` |
| ERR-31 | `errors/string_malformed` |
| ERR-32 | `errors/string_illegal_char` |
| ERR-33 | `errors/mat_dimensions_malformed` |
| ERR-34 | `errors/entry_extern_operand` |
| ERR-35 | `errors/missing_space_after_operation` |
| ERR-36 | `errors/missing_space_after_directive` |
| ERR-37 | `errors/matrix_operand_whitespace` |
| ERR-38 | `errors/label_no_space_after_colon` |
| ERR-39 | `errors/immediate_out_of_range`, `errors/data_out_of_range` |
| ERR-40 | `errors/mat_too_many_values` |
| ERR-41 | `errors/memory_overflow` |
| ERR-42 | `errors/matrix_operand_no_label` |
| ERR-43 | `errors/semicolon_not_first` |
| ERR-44 | `errors/mat_dimension_invalid` |
| ERR-45 | `errors/label_alone` |
| ERR-50 | `errors/undefined_symbol`, `errors/line_numbers_second_pass_macro` |
| ERR-51 | `errors/entry_undefined`, `errors/line_numbers_second_pass_macro` |
| ERR-52 | `errors/entry_and_extern` |
| ERR-53 | `errors/extern_then_label`, `errors/label_then_extern` |

### Valid programs (`tests/valid`)

| Case | SPEC rules | What it checks |
|---|---|---|
| `example1` | SPEC 4.1, MAC-5, MAC-29, OUT-2, OUT-13, INS-1 | Booklet example 1 (macro, no .entry and no .extern), with the corrected prn word. |
| `ps` | SPEC 4.2, SPEC 4.3, ENC-8, OUT-8, OUT-9, OUT-13 | Booklet example 2 (ps): entries and externals, with the corrected words. |
| `all_instructions` | INS-1, INS-3, ENC-3, ENC-9, ENC-10 | Each of the 16 operations exactly once. |
| `modes_mov` | INS-4, ADR-3, ADR-5, ADR-7, ADR-9, ADR-10, ADR-14 | mov with every legal source and destination mode (12 pairs). |
| `modes_cmp` | INS-4, ADR-3, ADR-5, ADR-7, ADR-9, ADR-10, ADR-11 | cmp with all 16 pairs of addressing modes. |
| `modes_add_sub` | INS-4, ADR-14 | add and sub with every legal pair of modes. |
| `modes_lea` | INS-4, ADR-13 | lea with source modes 1,2 and destination modes 1,2,3. |
| `modes_one_operand` | INS-3, INS-4, ENC-9 | Every one-operand operation with destination modes 1,2,3; prn also with mode 0. |
| `no_operand_instructions` | ENC-10, INS-3 | rts and stop: one word each, mode fields zero. |
| `register_word_sharing` | ADR-9, ADR-10, ADR-11, ADR-12, ADR-13 | Two registers share one word; a register next to any other mode, or matrix index registers, do not. |
| `data_values` | DIR-5, DIR-7, DIR-8, DIR-3, ENC-15, SYN-20, SYN-21 | .data with signs, zero and the boundary values -512 and 511. |
| `numbers_leading_zeros` | SYN-20 | Leading zeros are decimal digits: 010 is ten, not eight. |
| `immediate_bounds` | ADR-2, ADR-3, ENC-15, SYN-21 | Immediate operands -128, 127, 0, with and without a plus sign. |
| `string_basic` | DIR-9, DIR-10, DIR-11 | A labelled .string and its terminating zero word. |
| `string_empty` | DIR-32 | An empty string allocates exactly one zero word. |
| `string_punctuation` | SYN-22, DIR-10 | Spaces, punctuation, a backslash (no escapes) and statement-like text inside strings. |
| `string_inner_quote` | SYN-22 | A quotation mark inside a string is an ordinary character; the string ends at the last one. |
| `mat_full` | DIR-12, DIR-14, DIR-15, DIR-16 | .mat with a value for every cell, stored in row order. |
| `mat_partial` | DIR-24 | .mat with fewer values than cells: the rest are zero. |
| `mat_no_values` | DIR-13 | .mat without values still takes rows x columns zero words. |
| `mat_spacing` | DIR-27, DIR-33, DIR-6 | Whitespace inside the dimension brackets, no whitespace before the first value, leading zeros, boundary values. |
| `entry_only` | DIR-17, DIR-19, DIR-29, OUT-2, OUT-8 | Entries (code and data labels, declared before the definition) and no externals: .ent only. |
| `entry_after_definition` | DIR-17, OUT-8, ALG-6 | .entry lines that come after the definitions; data entry value includes ICF. |
| `extern_only` | DIR-20, DIR-21, ENC-8, OUT-2, OUT-9 | An external and no entries: .ext only. |
| `no_entry_no_extern` | OUT-2 | Neither .entry nor .extern: only .am and .ob are written. |
| `entry_and_extern` | OUT-2, OUT-8, OUT-9 | Both .ent and .ext are written. |
| `extern_multiple_uses` | OUT-9, ENC-8, ADR-5, ADR-7, LBL-11 | External symbols used many times, in direct and matrix operands, as source and destination. |
| `extern_declared_unused` | OUT-2, OUT-13 | .extern without any reference: the .ext file would be empty and is not created. |
| `extern_after_use` | LBL-10, LBL-11 | The .extern line comes after the instructions that use the symbol. |
| `entry_extern_repeated` | DIR-34 | The same .entry and the same .extern twice: accepted, one .ent line. |
| `forward_references` | LBL-10, ALG-1, ALG-6 | Code, data, string and matrix labels used before they are defined. |
| `code_data_interleaved` | ALG-4, ALG-5, ALG-6, ALG-14, LBL-9 | Data lines between instructions: data goes after all code, with no gap. |
| `data_without_label` | DIR-28 | .data, .string and .mat without labels still allocate memory. |
| `no_stop` | ALG-16 | A program with no stop instruction is legal. |
| `only_code` | OUT-13, OUT-4 | No data: the data length in the header is the single digit a. |
| `only_data` | OUT-13, OUT-4, ALG-14 | No instructions: the code length is a and the data starts at address 100. |
| `memory_exactly_full` | ALG-15, M-4 | Exactly 156 words: the last word is at address 255. |
| `labels_any_statement_kind` | LBL-17 | A .data label used as a matrix, a code label and a .string label used in direct and matrix operands. |
| `label_30_chars` | LBL-3 | A label of exactly 30 characters, defined, used directly, used as a matrix and declared entry. |
| `label_case_sensitive` | LBL-6 | Labels that differ only in letter case are different symbols. |
| `label_names_legal` | LBL-2, LBL-6, LBL-16, LBL-20, M-2, ENC-14 | Legal names that resemble reserved words: PSW, PC, r8, r20, R1, MOV, Stop, Data, movx. |
| `line_80_chars` | SYN-2 | Lines of exactly 80 characters: a comment, an instruction, a .data list and a .string. |
| `whitespace_tabs` | SYN-12, SYN-15, SYN-19 | Tabs as separators, indentation with tabs and spaces, trailing whitespace. |
| `whitespace_commas` | SYN-13, SYN-15, DIR-6 | Any amount of whitespace, or none, on either side of a comma. |
| `whitespace_matrix_brackets` | SYN-18 | Whitespace inside the brackets of a matrix operand. |
| `blank_lines` | SYN-4, SYN-8, MAC-29 | Empty lines and lines of spaces and tabs, also first and last in the file; copied to .am. |
| `comment_lines` | SYN-5, SYN-9, MAC-29 | Comment lines with statement-like text; copied to .am. |
| `no_macros` | MAC-14, MAC-29 | No macros: the .am file is identical to the .as file, byte for byte. |
| `macro_single` | MAC-2, MAC-3, MAC-5, MAC-29 | One macro, called once. |
| `macro_several` | MAC-5, MAC-29, MAC-27 | Three macros defined at different places, each called once. |
| `macro_called_twice` | MAC-5, MAC-29 | One macro called three times; other macro defined and never called. |
| `macro_blank_and_comment_lines` | MAC-29 | Blank and comment lines inside a macro body are body lines; those outside are copied. |
| `macro_names` | MAC-17, MAC-33, LBL-20 | Legal macro names: underscores, digits, 30 characters, names that only resemble reserved words. |
| `macro_whitespace` | MAC-19, MAC-30, MAC-29 | Whitespace before and after mcro, the macro name, mcroend and a macro call. |
| `macro_with_directives` | MAC-24, MAC-29, MAC-4 | Macro bodies holding labels and directives; the passes see only the expanded text. |
| `macro_name_not_first_field` | MAC-3, MAC-16 | A macro name inside a comment or a string is not a call. |

### Programs with errors (`tests/errors`)

| Case | SPEC rules | What it checks |
|---|---|---|
| `macro_name_reserved` | ERR-2, MAC-9, MAC-33, LBL-20, MAC-22, MAC-23 | Macro names that are operation, directive and register names. |
| `macro_name_reserved_mcro` | ERR-2, LBL-14, LBL-20 | mcro and mcroend as macro names. |
| `macro_def_extra_text` | ERR-3, MAC-10, MAC-22 | Extra text after the macro name on the mcro line. |
| `macro_end_extra_text` | ERR-4, MAC-10, MAC-31 | Extra text after mcroend; the definition is closed there, so the following lines are ordinary. |
| `macro_name_missing` | ERR-5, MAC-22 | mcro with no name, with and without trailing whitespace. |
| `macro_name_invalid` | ERR-6, MAC-17, MAC-33 | Macro names that start with a digit or an underscore, or contain an illegal character. |
| `macro_name_too_long` | ERR-6, MAC-17, MAC-33 | A macro name of 31 characters (30 is legal). |
| `macro_call_extra_text` | ERR-7, MAC-30 | Extra text after a macro name on a call line. |
| `macro_duplicate_name` | ERR-8, MAC-28 | A second definition of the same macro name, reported on the second mcro line. |
| `line_too_long` | ERR-10, SYN-2, SYN-2a, SYN-2b | Lines of 81 and more characters: comment, instruction, .data, macro body; 80-character lines stay legal. |
| `preassembler_error_hides_pass_errors` | ERR-3, MAC-11, MAC-23, SPEC 5.2 (effect) | After a pre-assembler error the passes do not run: the later first-pass error is not reported. |
| `label_invalid_chars` | ERR-11, LBL-2, ERR-R4 | Labels that start with a digit or contain an illegal character. |
| `label_too_long` | ERR-12, LBL-3 | A label of 31 characters (30 is legal). |
| `label_reserved_word` | ERR-13, LBL-7, LBL-20 | Operation, directive and register names used as labels. |
| `label_reserved_mcro` | ERR-13, LBL-14 | mcro and mcroend used as labels. |
| `label_same_as_macro` | ERR-14, LBL-8, LBL-15, LBL-21 | A label with the name of a macro, defined after and before the macro definition. |
| `label_duplicate` | ERR-15, LBL-5 | Labels defined twice (code and code, data and code, code and data); the later line is reported. |
| `label_space_before_colon` | ERR-16, LBL-4, SYN-16 | Whitespace between a label and its colon. |
| `label_no_space_after_colon` | ERR-38, LBL-18 | No whitespace between the colon of a label and the statement. |
| `label_alone` | ERR-45, LBL-22 | A label definition with nothing after the colon. |
| `label_on_error_line_is_defined` | LBL-23, ERR-15, ERR-20 | A legal label on an erroneous line is still defined, so a later definition of it is a duplicate. |
| `unknown_operation` | ERR-17, SYN-11, ENC-14 | Unknown operation names, including uppercase spellings. |
| `macro_nested_call_not_expanded` | ERR-17, MAC-32, MAC-35, ERR-R12 | A macro name inside another macro body is not expanded and is an unknown operation, reported on the body line. |
| `unknown_directive` | ERR-18, DIR-2, DIR-4 | Unknown directive names, including uppercase spellings. |
| `too_many_operands` | ERR-19, SYN-14 | More operands than the operation takes. |
| `too_few_operands` | ERR-20, SYN-14 | Fewer operands than the operation takes. |
| `extra_text_after_instruction` | ERR-27, SYN-10 | Text after the last operand, or after an operation that takes no operands. |
| `source_mode_illegal` | ERR-21, INS-4 | lea with an immediate or a register as source. |
| `dest_mode_illegal` | ERR-22, INS-4 | An immediate destination for every operation except cmp and prn. |
| `matrix_index_not_register` | ERR-23, ERR-25, ADR-6, LBL-16 | Matrix indices that are not one of r0 to r7. |
| `matrix_operand_malformed` | ERR-25, ADR-6 | Matrix operands with missing or unbalanced brackets or a wrong number of indices. |
| `matrix_operand_whitespace` | ERR-37, SYN-18 | Whitespace between the label and the first bracket, or between the two bracket pairs. |
| `matrix_operand_no_label` | ERR-42, SYN-23 | A matrix operand that starts with a bracket. |
| `immediate_malformed` | ERR-24, ADR-2, SYN-16, SYN-20 | A # that is not followed immediately by a legal integer. |
| `immediate_out_of_range` | ERR-39, SYN-21 | Immediate operands outside -128..127. |
| `missing_space_after_operation` | ERR-35, SYN-12 | No whitespace between the operation name and its first operand. |
| `comma_errors_operands` | ERR-26, SYN-13 | Missing comma, two commas, a comma before the first operand or after the last. |
| `data_no_values` | ERR-28, DIR-5 | .data with no values. |
| `data_value_not_integer` | ERR-29, SYN-20 | .data and .mat values that are not legal integers. |
| `data_comma_errors` | ERR-30, DIR-6 | Comma errors in .data and .mat lists. |
| `data_out_of_range` | ERR-39, SYN-21 | .data and .mat values outside -512..511. |
| `string_malformed` | ERR-31, DIR-9, SYN-22 | Missing quotes, text outside the quotes, a missing operand. |
| `string_illegal_char` | ERR-32, SYN-22 | Characters outside ASCII 32..126 inside a string: TAB, 0x01, DEL (0x7F), 0xE9. |
| `mat_dimensions_malformed` | ERR-33, DIR-12, DIR-26, DIR-27 | .mat with missing brackets, whitespace between the bracket pairs, or non-numeric dimensions. |
| `mat_dimension_invalid` | ERR-44, DIR-33 | .mat dimensions that are zero, negative or written with a sign. |
| `mat_too_many_values` | ERR-40, DIR-25 | .mat with more values than cells. |
| `entry_extern_operand` | ERR-34, DIR-17, DIR-20 | .entry and .extern with no operand, two operands or an ill-formed name. |
| `missing_space_after_directive` | ERR-36, SYN-17 | No whitespace between a directive name and its operand. |
| `semicolon_not_first` | ERR-43, SYN-9 | A semicolon after leading whitespace or after a statement does not start a comment. |
| `memory_overflow` | ERR-41, ALG-15 | 157 words: one word more than the memory holds. Only the severity is asserted (Q-30). |
| `extern_then_label` | ERR-53, DIR-30 | A symbol declared .extern and then defined as a label; reported in the first pass. |
| `label_then_extern` | ERR-53, DIR-30 | A label defined and then declared .extern. Only the severity is asserted (Q-30). |
| `first_pass_error_hides_second_pass` | ERR-R7, ERR-17, SPEC 5.3 | After a first-pass error the second pass does not run: the undefined symbol is not reported. |
| `line_numbers_after_macro` | ERR-R12, MAC-35, ERR-R3, ERR-17, ERR-20 | First-pass errors on lines after macro definitions and after macro calls carry .as line numbers. |
| `line_numbers_in_macro_body` | ERR-R12, MAC-35, MAC-13, ERR-17, ERR-20 | First-pass errors inside macro bodies: the body line number, once per call. |
| `undefined_symbol` | ERR-50, LBL-13, LBL-16, LBL-6 | Operand symbols that are not defined: direct, matrix, out-of-range register name, wrong letter case. |
| `entry_undefined` | ERR-51, DIR-19 | .entry naming a symbol that is not in the symbol table. |
| `entry_and_extern` | ERR-52, DIR-23 | The same symbol declared .entry and .extern, in both orders; reported on the .entry line. |
| `line_numbers_second_pass_macro` | ERR-R12, MAC-35, ERR-50, ERR-51 | Second-pass errors after and inside macro expansions carry .as line numbers; a body line once per call. |

### Command line (`tests/cli`)

| Case | SPEC rules | What it checks |
|---|---|---|
| `several_valid_files` | CLI-1, CLI-2, CLI-3, CLI-9, OUT-1 | Three valid files in one run; each gets its own outputs. |
| `same_names_in_two_files` | CLI-3, ALG-4 | Two files that reuse the same label and macro names with different meanings: no state leaks between files. |
| `error_then_valid` | CLI-3, CLI-9, ERR-R6, OUT-11 | A file with a first-pass error followed by a valid file: the valid one is still assembled, status 1. |
| `valid_then_error` | CLI-3, CLI-9, ERR-R6 | A valid file followed by a file with a first-pass error: the outputs of the valid one stay, status 1. |
| `preassembler_error_then_valid` | MAC-11, MAC-23, CLI-9 | A file with a pre-assembler error followed by a valid file: no .am for the first, the second is assembled. |
| `missing_file_last` | ERR-1, CLI-6, CLI-9, ERR-R13 | A valid file followed by a name with no .as file: message without a line number, status 1. |
| `missing_file_only` | ERR-1, CLI-6, CLI-9 | The only argument names a file that does not exist. |
| `as_suffix_argument` | ERR-1, CLI-8 | The argument prog.as while prog.as exists: the assembler looks for prog.as.as and reports it. |
| `no_arguments` | CLI-5, CLI-9 | No file arguments: not an error, status 0. |
