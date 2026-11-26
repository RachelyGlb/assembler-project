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
that receives only the `.as` files of the case and, if the case has a
directory `stale/`, the files in it (a command-line case may also run a
script there, see "Setup scripts"); this keeps the source tree clean and
makes "this file must not exist after the run" checkable (OUT-14). The
scratch directory of a passing case is deleted. For a failing case it is
kept and holds the files the assembler wrote, `stdout.txt`, `stderr.txt` and
`diff.txt` with every check that failed. `tests/out/` is ignored by git. A
run is limited to 5 seconds when the `timeout` command exists; a timeout or a
crash is a FAIL of that case and the run goes on.

## Layout

```
tests/
  run_tests.sh             the runner
  .gitattributes           "* -text": git must not touch line endings here
  valid/<case>/            programs that must assemble
    <case>.as              input
    <case>.am              expected pre-assembler output (always present)
    <case>.ob              expected object file (always present)
    <case>.ent, <case>.ext expected only when SPEC says the file has content
    <case>.expected        expected warnings; no such file means none
    stale/                 optional: files present before the run (see below)
  errors/<case>/           programs with errors
    <case>.as              input
    <case>.expected        expected diagnostics, in order
    <case>.am              present only when the errors are found by the passes
    stale/                 optional
  cli/<case>/              cases about the command line
    args                   the command-line arguments, separated by whitespace
    expected_status        the expected exit status
    expected_messages      expected diagnostics, in order; no such file: none
    expected_other_lines   number of stdout lines that are not diagnostics;
                           no such file means 0 (only `no_arguments` has one)
    *.as                   inputs
    *.am *.ob *.ent *.ext  expected outputs
    stale/                 optional
    setup.sh               optional: run in the scratch directory before
                           the assembler (see "Setup scripts")
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
and `stdout` holds exactly the warnings of `<case>.expected`, which for most
cases means that it is empty (ERR-R15).

Error cases (`tests/errors`): the exit status is 1 (CLI-9); no `.ob`, `.ent`
or `.ext` exists (ERR-R6); the `.am` file is byte-identical to the expected
one when the case has one (errors found by the passes, OUT-11) and does not
exist otherwise (errors found by the pre-assembler, MAC-23); and `stdout`
holds exactly the diagnostics of `<case>.expected`.

Command-line cases (`tests/cli`): the exit status equals `expected_status`;
every expected output file matches as above; no other `.am`, `.ob`, `.ent`
or `.ext` file exists in the scratch directory; and `stdout` holds exactly
the diagnostics of `expected_messages`.

### Standard output

`stdout` is specified completely (ERR-R15), so all of it is checked, in every
group:

- Every line must have one of the four forms of ERR-R13 and ERR-R14:
  `<name>.as:<line>: error: <text>`, `<name>.as: error: <text>`, and the same
  two with `warning:`. Any other line (a success notice, progress text, an
  empty line) fails the case. The one exception is `cli/no_arguments`, whose
  `expected_other_lines` allows exactly one such line, the usage line of
  CLI-10.
- Each diagnostic is reduced to `<line> <severity>` (valid and error cases)
  or `<file> <line> <severity>` (command-line cases); `<line>` is `-` for a
  message without a line number. The wording is never compared (ERR-R13). In
  a valid or error case a message that names another file than `<case>.as`
  never matches.
- The reduced list is compared with the expected file **line by line, in
  order** (ERR-R17): files in command-line order; within a file the
  pre-assembler messages in `.as` order, or else the first-pass messages
  (errors and warnings) followed by the second-pass messages, each group in
  the order of the `.am` file. The numbers are `.as` line numbers (ERR-R12),
  so the expected files are not always ascending: a macro body line repeats
  its number at every call. Because a line yields exactly one message
  (ERR-R16), the number of lines is fixed as well.

### Stale files

A case may have a directory `stale/`. Its files are copied into the scratch
directory before the run and stand for output files left over from an
earlier run (OUT-14). Nothing else is special about them: the ordinary
checks then prove the rule. A stale file with no expected counterpart must
have been removed ("exists but must not exist"), and a stale file with an
expected counterpart must have been replaced (it is compared byte for byte,
and the stale content is always different from the expected content).

### Setup scripts

A command-line case may have a file `setup.sh`. After the `.as` files and
the stale files were copied, and before the assembler is started, the runner
executes it with `sh`, with the scratch directory as the current directory.
It exists for what plain input files cannot express: a directory in the
place of a source file (CLI-12), or a source file inside a subdirectory. The
rules are deliberately narrow: only the script of the case's own directory
is run, it is not copied into the scratch directory, its output is
discarded, and it must touch nothing outside the current directory. If the
script fails, the case fails with `setup.sh failed` and the assembler is not
run. Cases in `valid/` and `errors/` have no such script.

The output files are compared in the scratch directory itself only. For an
argument with a directory part (`cli/argument_with_directory`) the runner
therefore checks the messages and the exit status, not the files written
into the subdirectory.

## Conventions for writing cases

- Every input ends with a newline, uses LF line endings and holds at least
  one instruction or data directive. **The cases listed below break this
  convention on purpose** (SYN-24, SYN-25, SYN-26, SYN-8, OUT-15); their bytes are
  exact, they were written with `printf`, and they must not be "repaired" by
  an editor or by git. `tests/.gitattributes` (`* -text`) turns off every
  line-ending conversion under `tests/`.

  | Case | What is unusual in the `.as` file |
  |---|---|
  | `valid/last_line_no_newline` | No newline after the last line. |
  | `valid/last_line_cr_no_newline` | The file ends with a CR and no newline. |
  | `valid/comment_only_no_newline` | One comment line, no newline. |
  | `valid/crlf_line_endings` | CR LF line endings (the expected `.am` has LF). |
  | `valid/cr_inside_line` | Lone CR characters inside lines; one line ends CR CR LF. |
  | `valid/empty_file` | Zero bytes (the expected `.am` is empty too). |
  | `valid/comments_only` | No statement at all. |
  | `valid/whitespace_vt_ff` | Vertical tabs (0x0B) and form feeds (0x0C). |
  | `errors/crlf_line_too_long` | CR LF line endings; one line ends CR CR LF. |
  | `errors/cr_inside_string` | CR LF line endings and a CR inside a string. |
  | `errors/string_illegal_char` | Control and non-ASCII bytes inside strings. |
  | `errors/nul_byte` | NUL bytes (0x00) inside seven lines; one of them is a comment line of 82 bytes. Not a text file: recreate it with `printf` only. |
  | `errors/macro_unclosed_no_newline` | The last line is `mcro m1` with no newline after it. |
  | `errors/line_too_long_mcro`, `errors/line_too_long_mcroend`, `errors/line_too_long_macro_call` | One line each is made longer than 80 characters by 80 trailing spaces. |

- A line holds at most one error, except in the three `one_message_per_line*`
  cases and in the cases about faulty `mcro` lines, which exist to check
  that such a line still yields exactly one message (ERR-R16). Since the
  wording is free, nothing depends on which of the errors is reported.
- An error case mixes erroneous lines with valid lines, so that "does not
  stop at the first error" (ERR-R4) is checked as well.
- A case holds errors of one stage only, because a pre-assembler error hides
  the errors of the passes and a first-pass error hides those of the second
  pass (SPEC 5.2, 5.3). Three cases check exactly this hiding. Warnings are
  first-pass messages and may stand together with second-pass errors; the
  `order_*` cases use this to check the order of the stages (ERR-R17).
- Expected `.ob`, `.ent` and `.ext` files are computed from SPEC sections 2
  and 3. The two booklet examples are copied from SPEC section 4.
- Expected diagnostics are written by hand from SPEC, in the order of
  ERR-R17: for the passes, expand the macros first and then walk the `.am`
  lines.
- After a faulty `mcro` line the cases still close the definition with
  `mcroend` and expect no further message for the body lines or for the
  `mcroend` line (MAC-38). The `macro_unclosed*` cases are the exception:
  they leave a definition open on purpose (MAC-39), and since everything
  after the open `mcro` line is a body line, a file can hold only one such
  definition, as its last one.
- A pre-assembler case has no `.am` file. That absence is the check that no
  `.am` is left (MAC-23), also for a definition that is not closed (MAC-39).
- ERR-41 is reported once per file (ALG-17), so each `.mat` line with huge
  dimensions has a case of its own. A line with another first-pass error
  adds no words (ALG-18); `errors/memory_overflow_with_other_error` counts
  its words on that basis.
- A reserved word, an ill-formed name or an over-long name as an operand is
  a first-pass error (LBL-24). The three older `errors/operand_*` cases hold
  no other erroneous line; `errors/operand_not_label_first_pass`,
  `errors/entry_extern_operand_not_label` and `errors/extern_macro_name`
  add a line with an undefined symbol, which must stay unreported because
  the second pass does not run (ERR-R7). That is how the stage is observed.
- A line is made longer than 80 characters with trailing spaces when it must
  stay an otherwise correct `mcro` line, `mcroend` line or macro call
  (MAC-19, MAC-40).

## Not tested (undefined in SPEC, or not observable)

SPEC 6.1 has no open question left: Q-34 to Q-40 were decided on 2026-11-27
and each of them has cases now.

| Decision | Rules | Cases |
|---|---|---|
| Q-34 | DIR-35, ERR-R16, ERR-R17 | `errors/warning_and_second_pass_error` |
| Q-35 | MAC-40 | `errors/line_too_long_mcro`, `errors/line_too_long_mcroend`, `errors/line_too_long_macro_call` |
| Q-36 | MAC-41 | `errors/mcroend_without_definition` |
| Q-37 | ALG-18 | `errors/memory_overflow_with_other_error` |
| Q-38 | MAC-39, ERR-48 | the five `errors/macro_unclosed*` cases |
| Q-39 | LBL-24, ERR-49, ERR-34, ERR-14 | the three older `errors/operand_*` cases, `errors/operand_not_label_first_pass`, `errors/entry_extern_operand_not_label`, `errors/extern_macro_name` |
| Q-40 | SYN-26, CLI-12 | `errors/nul_byte`, `cli/source_is_directory`, `cli/source_is_directory_stale` (the rules were confirmed as written) |

Decided but without a case of its own: a NUL byte on a `mcro` line, a
`mcroend` line or a macro call (MAC-40 treats it like an over-long line; the
three `line_too_long_m*` cases stand for both), and an unclosed definition
whose `mcro` line was reported for a label in front of it, for its length or
for a NUL byte (MAC-39; `errors/macro_unclosed_faulty_mcro` stands for all
kinds of `mcro` line that already have a message).

Outside the suite: the inputs that SPEC declares undefined (nested macro
definitions, a macro used before its definition, text before `mcroend`;
MAC-6, MAC-8, MAC-20) and allocation failure
(ERR-R10), which the runner cannot provoke (it would need a memory limit
around the binary, and the point of failure depends on the platform). A read
error in the middle of a file (the second half of CLI-12) cannot be produced
portably either; the directory cases stand for it.

Rules whose effect cannot be seen from outside, and what is checked instead:

- MAC-38, "for a duplicate name the first definition remains in force": a
  file with a duplicate definition gets no `.am` file, so *which* body a
  later call would expand is invisible. The cases check what is visible:
  the name is still a macro after the faulty second definition (a later call
  with extra text is ERR-7, a label in front of a call is ERR-9).
- MAC-37, "`L: m` is not expanded": no `.am` file is written for such a file.
  The cases check the single message and the state of the macro table.
- ERR-R16, "the first error found": which error of a line is reported is
  free; only the count (one) is checked.

## Coverage

### Error categories (SPEC section 5)

| ERR | Cases |
|---|---|
| ERR-1 | `cli/missing_file_last`, `cli/missing_file_only`, `cli/missing_file_first`, `cli/missing_file_middle`, `cli/missing_files_several`, `cli/as_suffix_argument`, `cli/order_several_files`, `cli/stale_files_of_missing_file`, `cli/missing_file_long_name`, `cli/argument_with_directory`, `cli/dash_arguments` |
| ERR-2 | `errors/macro_name_reserved`, `errors/macro_name_reserved_mcro`, `errors/faulty_mcro_body_is_silent`, `errors/macro_unclosed_faulty_mcro` |
| ERR-3 | `errors/macro_def_extra_text`, `errors/preassembler_error_hides_pass_errors`, `errors/faulty_mcro_body_is_silent`, `errors/faulty_mcro_name_is_not_a_macro` |
| ERR-4 | `errors/macro_end_extra_text`, `errors/one_message_per_line_preassembler` |
| ERR-5 | `errors/macro_name_missing`, `errors/faulty_mcro_body_is_silent` |
| ERR-6 | `errors/macro_name_invalid`, `errors/macro_name_too_long`, `errors/faulty_mcro_body_is_silent` |
| ERR-7 | `errors/macro_call_extra_text`, and as the probe for the state of the macro table in the `label_before_*` and `faulty_mcro_*` cases and in `errors/line_too_long_mcro` and `errors/line_too_long_mcroend` |
| ERR-8 | `errors/macro_duplicate_name`, `errors/faulty_mcro_body_is_silent`, `errors/faulty_mcro_name_is_not_a_macro`, `errors/label_before_mcro` |
| ERR-9 | `errors/label_before_mcro`, `errors/label_before_mcro_faulty`, `errors/label_before_mcroend`, `errors/label_before_macro_call`, `errors/faulty_mcro_name_is_not_a_macro`; not ERR-9: `errors/label_not_attached_macro_lines` |
| ERR-10 | `errors/line_too_long`, `errors/crlf_line_too_long`, `errors/nul_byte` (a line that is too long and holds a NUL byte), `errors/line_too_long_mcro`, `errors/line_too_long_mcroend`, `errors/line_too_long_macro_call`, `errors/macro_unclosed_long_body_line` |
| ERR-11 | `errors/label_invalid_chars` |
| ERR-12 | `errors/label_too_long` |
| ERR-13 | `errors/label_reserved_word`, `errors/label_reserved_mcro` |
| ERR-14 | `errors/label_same_as_macro`, `errors/extern_macro_name` |
| ERR-15 | `errors/label_duplicate`, `errors/label_on_error_line_is_defined`, `errors/label_on_err38_err45_line_is_defined` |
| ERR-16 | `errors/label_space_before_colon` |
| ERR-17 | `errors/unknown_operation`, `errors/macro_nested_call_not_expanded`, `errors/first_pass_error_hides_second_pass`, `errors/line_numbers_after_macro`, `errors/line_numbers_in_macro_body`, `errors/mcroend_without_definition`, `errors/memory_overflow_with_other_error` |
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
| ERR-29 | `errors/data_value_not_integer`, `errors/memory_overflow_with_other_error` |
| ERR-30 | `errors/data_comma_errors` |
| ERR-31 | `errors/string_malformed` |
| ERR-32 | `errors/string_illegal_char`, `errors/cr_inside_string` |
| ERR-33 | `errors/mat_dimensions_malformed` |
| ERR-34 | `errors/entry_extern_operand`, `errors/warning_line_with_error`, `errors/entry_extern_operand_not_label` |
| ERR-35 | `errors/missing_space_after_operation` |
| ERR-36 | `errors/missing_space_after_directive` |
| ERR-37 | `errors/matrix_operand_whitespace` |
| ERR-38 | `errors/label_no_space_after_colon`, `errors/label_on_err38_err45_line_is_defined` |
| ERR-39 | `errors/immediate_out_of_range`, `errors/data_out_of_range`, `errors/number_wraps_around` |
| ERR-40 | `errors/mat_too_many_values` |
| ERR-41 | `errors/memory_overflow`, `errors/memory_overflow_reported_once`, `errors/memory_overflow_data_before_code`, `errors/memory_overflow_code_only`, `errors/memory_overflow_in_macro`, `errors/memory_overflow_with_other_error`, `errors/mat_dimension_huge`, `errors/mat_dimensions_product_huge`, `errors/mat_dimensions_product_wraps_32`, `errors/mat_dimension_wraps_32`, `errors/mat_dimension_wraps_64` |
| ERR-42 | `errors/matrix_operand_no_label` |
| ERR-43 | `errors/semicolon_not_first` |
| ERR-44 | `errors/mat_dimension_invalid` |
| ERR-45 | `errors/label_alone`, `errors/label_on_err38_err45_line_is_defined` |
| ERR-46 | `errors/nul_byte` |
| ERR-47 | `cli/source_is_directory`, `cli/source_is_directory_stale` |
| ERR-48 | `errors/macro_unclosed`, `errors/macro_unclosed_after_valid`, `errors/macro_unclosed_long_body_line`, `errors/macro_unclosed_no_newline`; not ERR-48: `errors/macro_unclosed_faulty_mcro`, `errors/line_too_long_mcroend` |
| ERR-49 | `errors/operand_reserved_word`, `errors/operand_ill_formed_name`, `errors/operand_name_too_long`, `errors/operand_not_label_first_pass` |
| ERR-50 | `errors/undefined_symbol`, `errors/line_numbers_second_pass_macro`, `errors/one_message_per_line_second_pass`, `errors/order_first_pass_before_second_pass`, `errors/warning_label_is_not_defined`, `errors/macro_name_not_first_word`, `errors/order_macro_warning_and_second_pass` |
| ERR-51 | `errors/entry_undefined`, `errors/line_numbers_second_pass_macro`, `errors/warning_and_second_pass_error` |
| ERR-52 | `errors/entry_and_extern`, `errors/warning_and_second_pass_error` |
| ERR-53 | `errors/extern_then_label`, `errors/label_then_extern`, `errors/extern_label_conflict_data`, `errors/warning_line_with_error` |
| WRN-1 | `valid/warning_label_before_entry`, `valid/warning_label_before_extern`, `valid/warning_label_not_in_symbol_table`, `valid/warning_in_macro_body`, `errors/warning_label_is_not_defined`, `errors/warning_line_with_error`, `errors/order_first_pass_before_second_pass`, `errors/order_macro_defined_late`, `errors/warning_and_second_pass_error`, `cli/warnings_only_status_zero`, `cli/missing_file_middle`, `cli/order_several_files` |

### Rules about the output as a whole

| Rule | Cases |
|---|---|
| ERR-R15 (nothing but diagnostics on `stdout`) | every case, through the runner; `cli/no_arguments` for the usage line (CLI-10) |
| ERR-R16 (one message per line) | `errors/one_message_per_line`, `errors/one_message_per_line_preassembler`, `errors/one_message_per_line_second_pass`, `errors/warning_line_with_error`, `errors/warning_and_second_pass_error` (a warning and a second-pass error on one line are both printed), `errors/macro_unclosed_faulty_mcro`, `errors/memory_overflow_with_other_error`, and the `label_before_*` and `faulty_mcro_*` cases |
| ERR-R17 (order) | every case, through the runner; specifically `errors/order_first_pass_before_second_pass`, `errors/order_macro_body_called_twice`, `errors/order_macro_defined_late`, `errors/line_numbers_in_macro_body`, `errors/line_numbers_second_pass_macro`, `errors/warning_and_second_pass_error`, `errors/macro_unclosed_long_body_line` (ERR-48 last, with the smaller line number), `cli/order_several_files`, `cli/missing_file_middle` |
| OUT-14 (stale files) | `valid/stale_ent_ext_removed`, `valid/stale_files_replaced`, `valid/stale_ext_removed_extern_unused`, `errors/stale_files_preassembler_error`, `errors/stale_files_first_pass_error`, `errors/stale_files_second_pass_error`, `cli/stale_files_of_missing_file`, `cli/source_is_directory_stale` |
| SYN-24, SYN-25, OUT-15 | `valid/last_line_no_newline`, `valid/last_line_cr_no_newline`, `valid/comment_only_no_newline`, `valid/crlf_line_endings`, `valid/cr_inside_line`, `valid/empty_file`, `valid/comments_only`, `errors/crlf_line_too_long`, `errors/cr_inside_string` |
| CLI-10, CLI-11 | `cli/no_arguments`; `cli/missing_file_first`, `cli/missing_file_middle`, `cli/missing_files_several`, `cli/missing_file_last`; CLI-12: `cli/source_is_directory`, `cli/source_is_directory_stale` |

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
| `string_two_quoted_parts` | SYN-22, DIR-10 | `.string "a" "b"` is one string of five characters (a, quotation mark, space, quotation mark, b), because the string ends at the last quotation mark. |
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
| `warning_label_before_entry` | WRN-1, DIR-35, DIR-18, ERR-R14 | A label in front of .entry: a warning on that line, all outputs written, status 0. |
| `warning_label_before_extern` | WRN-1, DIR-35, DIR-22 | A label in front of .extern: a warning, the .ext file is still written. |
| `warning_label_not_in_symbol_table` | DIR-35, WRN-1 | The ignored label is not a symbol: the same name is defined elsewhere, twice in front of directives, and names that are invalid, reserved, too long, empty or macro names are accepted. |
| `warning_in_macro_body` | DIR-35, ERR-R12, ERR-R17 | A warning line inside a macro body is reported once per call, with the body line number. |
| `stale_ent_ext_removed` | OUT-14, OUT-2 | Stale .ent and .ext are removed when the successful run does not produce them; stale .am and .ob are replaced. |
| `stale_files_replaced` | OUT-14 | All four stale files are replaced by the files of this run. |
| `stale_ext_removed_extern_unused` | OUT-14, OUT-13 | A stale .ext is removed when .extern is declared but never used; a stale .ent is replaced. |
| `last_line_no_newline` | SYN-24, MAC-29 | The last line has no newline: it is a complete line and the .am file ends with a newline. |
| `last_line_cr_no_newline` | SYN-25, SYN-24 | A CR at the end of a last line without a newline is ordinary whitespace and is copied to .am. |
| `comment_only_no_newline` | SYN-24, OUT-15 | One comment line without a newline: .am gets the newline, .ob is the header a TAB a. |
| `crlf_line_endings` | SYN-25, MAC-29, SYN-2 | CR LF input with a macro: the .am file has LF endings; lines of 80 characters plus CR LF are legal. |
| `cr_inside_line` | SYN-25, SYN-8 | A lone CR inside a line is whitespace and is copied to .am; of CR CR LF only the last CR is dropped. |
| `empty_file` | OUT-15 | A file of zero bytes: empty .am, .ob with the single line a TAB a, status 0. |
| `comments_only` | OUT-15, MAC-29 | Only comment lines and empty lines: .am is a copy, .ob is a TAB a. |
| `whitespace_vt_ff` | SYN-8 | Vertical tab and form feed as whitespace between all kinds of fields and as an empty line. |
| `label_indented` | SYN-19, LBL-4 | Label definitions after leading spaces and tabs. |
| `string_colon_unlabelled` | SYN-22, DIR-28 | A colon inside a string on a line without a label is not a label definition. |
| `memory_full_code_only` | ALG-15, OUT-13 | Exactly 156 code words and no data. |
| `memory_full_data_before_code` | ALG-15, ALG-5 | Exactly 156 words where the data lines come before the code lines. |

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
| `memory_overflow` | ERR-41, ALG-15, ALG-17 | 157 words: one word more than the memory holds; reported on the line that adds the 157th word. |
| `extern_then_label` | ERR-53, DIR-30, DIR-36 | A symbol declared .extern and then defined as a label: reported on the label line. |
| `label_then_extern` | ERR-53, DIR-30, DIR-36 | A label defined and then declared .extern: reported on the .extern line. |
| `first_pass_error_hides_second_pass` | ERR-R7, ERR-17, SPEC 5.3 | After a first-pass error the second pass does not run: the undefined symbol is not reported. |
| `line_numbers_after_macro` | ERR-R12, MAC-35, ERR-R3, ERR-17, ERR-20 | First-pass errors on lines after macro definitions and after macro calls carry .as line numbers. |
| `line_numbers_in_macro_body` | ERR-R12, MAC-35, MAC-13, ERR-17, ERR-20 | First-pass errors inside macro bodies: the body line number, once per call. |
| `undefined_symbol` | ERR-50, LBL-13, LBL-16, LBL-6 | Operand symbols that are not defined: direct, matrix, out-of-range register name, wrong letter case. |
| `entry_undefined` | ERR-51, DIR-19 | .entry naming a symbol that is not in the symbol table. |
| `entry_and_extern` | ERR-52, DIR-23 | The same symbol declared .entry and .extern, in both orders; reported on the .entry line. |
| `line_numbers_second_pass_macro` | ERR-R12, MAC-35, ERR-50, ERR-51 | Second-pass errors after and inside macro expansions carry .as line numbers; a body line once per call. |
| `extern_label_conflict_data` | ERR-53, DIR-36 | Both orders with data labels (.data, .string, .mat) and a code label; each reported once, on the second of the two lines. |
| `memory_overflow_reported_once` | ERR-41, ALG-17, ERR-R4 | A multi-word line crosses the limit; later lines add words without a second report; later unrelated errors are still reported. |
| `memory_overflow_data_before_code` | ERR-41, ALG-17 | 153 data words come first; an instruction line crosses the limit and is the line reported. |
| `memory_overflow_code_only` | ERR-41, ALG-17 | 157 code words and no data. |
| `memory_overflow_in_macro` | ERR-41, ALG-17, ERR-R12 | The line that crosses the limit comes from a macro body: its body line number, once, although later calls add more words. |
| `warning_label_is_not_defined` | DIR-35, ERR-50, ERR-R17 | The label in front of .extern is not a symbol: using it is an undefined symbol; the warning comes before the second-pass error. |
| `warning_line_with_error` | DIR-35, ERR-R16, ERR-34, ERR-53, ERR-38, ERR-16 | A labelled .entry or .extern line with an error of its own prints only the error; `L:.extern` and `L : .extern` are errors, not warnings. |
| `label_before_mcro` | ERR-9, MAC-36, MAC-37 | `L: mcro x`: one message; the definition is opened (its body is silent), closed by mcroend, and x is in the macro table. |
| `label_before_mcro_faulty` | ERR-9, MAC-37, MAC-38 | `L: mcro` with a reserved name, extra text or a duplicate name: one message, body discarded silently, the name is not entered. |
| `label_before_mcroend` | ERR-9, MAC-36, MAC-37 | `L: mcroend`, also with extra text: one message; the definition is closed and the macro is defined. |
| `label_before_macro_call` | ERR-9, MAC-36, MAC-37 | `L: m`, also with extra text and with an invalid label: one message each; `L : m` and `L:m` are not recognised. |
| `label_not_attached_macro_lines` | MAC-36, ERR-16, ERR-38 | `L : m`, `L:m`, `L : mcro x`, `L:mcro y` are ordinary lines: copied to .am and rejected by the first pass. |
| `faulty_mcro_body_is_silent` | MAC-38, ERR-2, ERR-3, ERR-5, ERR-6, ERR-8 | After each kind of faulty mcro line the body is silent: over-long lines, calls with extra text, text after mcroend, a label before mcroend. |
| `faulty_mcro_name_is_not_a_macro` | MAC-38, ERR-3, ERR-8, ERR-7, ERR-9 | A name whose definition was faulty is not a macro; after a duplicate definition the name is still a macro. |
| `one_message_per_line` | ERR-R16 | First-pass lines with two or three errors each yield one message. |
| `one_message_per_line_preassembler` | ERR-R16, MAC-38 | Pre-assembler lines with several errors each yield one message. |
| `one_message_per_line_second_pass` | ERR-R16, ERR-50 | Lines with two undefined symbols yield one message. |
| `order_first_pass_before_second_pass` | ERR-R17, WRN-1 | Warnings of the first pass come before second-pass errors that stand on earlier lines. |
| `order_macro_body_called_twice` | ERR-R17, ERR-R12 | Errors in a macro body called twice, between other errors: the order of the .am file. |
| `order_macro_defined_late` | ERR-R17, ERR-R12 | Macros defined after erroneous lines and called in another order: line numbers are not ascending; a warning keeps its place among the errors. |
| `stale_files_preassembler_error` | OUT-14, MAC-23 | Stale .am, .ob, .ent and .ext are all removed when the pre-assembler fails. |
| `stale_files_first_pass_error` | OUT-14, OUT-11, ERR-R6 | A first-pass error: stale .ob, .ent and .ext are removed, the stale .am is replaced. |
| `stale_files_second_pass_error` | OUT-14, OUT-11, ERR-R6 | The same for a second-pass error. |
| `crlf_line_too_long` | SYN-25, ERR-10 | CR LF input: 80 characters plus CR LF is legal, 81 is an error, and 80 plus CR CR LF is 81 characters. |
| `cr_inside_string` | SYN-25, ERR-32 | A CR inside a string is an illegal character; the .am file has LF endings and keeps the inner CR. |
| `macro_name_not_first_word` | MAC-3, MAC-16, ERR-50 | A macro name used as an operand is not expanded (the .am file keeps the line) and is an undefined symbol. |
| `nul_byte` | SYN-26, ERR-46, ERR-R16, MAC-23 | A NUL byte in an instruction, before text that would be an error, in a short comment, in a comment of 82 bytes (one message), in a macro body line (one message although called twice), in a string and alone on a line: one pre-assembler message each, no .am. **Fails until defect D-1 is fixed.** |
| `label_on_err38_err45_line_is_defined` | LBL-23, ERR-38, ERR-45, ERR-15 | The label of an `A:.data 1` line, of a `C:stop` line and of a label-alone line is still defined: a later definition of each is a duplicate. |
| `number_wraps_around` | ERR-39, SYN-21 | `.data`, `.mat` and immediate values that wrap around 32 or 64 bits to a legal number (2^32+1, 2^64+1, 2^32-512, 2^32-128 and others) are out of range. |
| `mat_dimension_huge` | ERR-41, ALG-17 | `.mat [99999999999][2]`: memory overflow on that line, nothing else. |
| `mat_dimensions_product_huge` | ERR-41, ALG-17 | `.mat [200][200]`: 40000 words. |
| `mat_dimensions_product_wraps_32` | ERR-41, ALG-17 | `.mat [65536][65536]`: the product is 2^32 and must not wrap to 0. |
| `mat_dimension_wraps_32` | ERR-41, ALG-17 | `.mat [4294967297][1] 5`: the dimension must not wrap to 1. |
| `mat_dimension_wraps_64` | ERR-41, ALG-17 | `.mat [18446744073709551617][1] 5`: the same for 64 bits. |
| `operand_reserved_word` | LBL-24, ERR-49, ERR-R16 | `jmp mov`: exactly one error on the line. |
| `operand_ill_formed_name` | LBL-24, ERR-49, ERR-R16 | `jmp 1abc`: exactly one error on the line. |
| `operand_name_too_long` | LBL-24, ERR-49, ERR-R16 | An operand of 31 letters: exactly one error on the line. |
| `order_macro_warning_and_second_pass` | ERR-R17, ERR-R12, DIR-35, ERR-50 | A macro body with a warning line and an undefined symbol, called three times between other second-pass errors: all warnings first, then the errors in .am order. |
| `macro_unclosed` | MAC-39, ERR-48, MAC-23 | A `mcro` with no `mcroend` before the end of the file: one error, on the `mcro` line; no .am. **Fails until MAC-39 is implemented.** |
| `macro_unclosed_after_valid` | MAC-39, ERR-48, MAC-24 | An unclosed definition after a closed macro, a call and statements: one error on the open `mcro` line; its body lines (a call, a call with extra text) are not examined. **Fails until MAC-39 is implemented.** |
| `macro_unclosed_faulty_mcro` | MAC-39, MAC-38, ERR-2, ERR-R16 | `mcro mov` that is never closed: exactly its one message, no ERR-48; no .am. |
| `macro_unclosed_long_body_line` | MAC-39, ERR-48, ERR-10, ERR-R17 | An unclosed definition whose body holds a line of 87 characters: the message of the body line first, then ERR-48 with the smaller number of the `mcro` line. **Fails until MAC-39 is implemented.** |
| `macro_unclosed_no_newline` | MAC-39, ERR-48, SYN-24 | The file ends with the line `mcro m1` and no newline: the definition is open with an empty body; one error on that line. **Fails until MAC-39 is implemented.** |
| `warning_and_second_pass_error` | DIR-35, ERR-R16, ERR-R17, WRN-1, ERR-51, ERR-52 | `L: .entry X` with `X` undefined and with `X` external: the line gets a warning from the first pass and an error from the second pass; both warnings come before both errors. |
| `line_too_long_mcro` | MAC-40, ERR-10, ERR-7 | An over-long `mcro` line: one message; the definition is still opened and the macro defined (a later call with extra text is ERR-7). |
| `line_too_long_mcroend` | MAC-40, ERR-10, ERR-7, MAC-39 | An over-long `mcroend` line: one message; the definition is still closed (the next line is examined: ERR-7) and there is no ERR-48. |
| `line_too_long_macro_call` | MAC-40, ERR-10 | An over-long macro call: one message, no .am. |
| `mcroend_without_definition` | MAC-41, ERR-17, ERR-R12 | `mcroend` and `mcroend junk` while no definition is open: no pre-assembler error; the lines are copied to .am and the first pass reports each once. |
| `memory_overflow_with_other_error` | ALG-18, ALG-17, ERR-41, ERR-17, ERR-29, ERR-R16 | At 155 words, an unknown operation and a faulty `.data` line of four values add no words and get one message each; the next line fills the memory silently and the one after it reports the overflow. |
| `operand_not_label_first_pass` | LBL-24, ERR-49, ERR-R7 | A reserved word, a name of 31 letters, an ill-formed name and `r1[r2][r3]` as instruction operands are first-pass errors: the undefined symbol on a later line is not reported. |
| `extern_macro_name` | LBL-24, ERR-14, LBL-21, ERR-R7 | `.extern` with the name of a macro defined before the line and of one defined after it: first-pass errors on the `.extern` lines; the undefined symbol is not reported. |
| `entry_extern_operand_not_label` | LBL-24, ERR-34, ERR-R7 | `.entry` and `.extern` with a reserved word (operation, register, directive name) or a name of 31 letters: first-pass errors; the undefined symbol is not reported. |

### Command line (`tests/cli`)

| Case | SPEC rules | What it checks |
|---|---|---|
| `several_valid_files` | CLI-1, CLI-2, CLI-3, CLI-9, OUT-1 | Three valid files in one run; each gets its own outputs. |
| `same_names_in_two_files` | CLI-3, ALG-4 | Two files that reuse the same label and macro names with different meanings: no state leaks between files. |
| `error_then_valid` | CLI-3, CLI-9, ERR-R6, OUT-11 | A file with a first-pass error followed by a valid file: the valid one is still assembled, status 1. |
| `valid_then_error` | CLI-3, CLI-9, ERR-R6 | A valid file followed by a file with a first-pass error: the outputs of the valid one stay, status 1. |
| `preassembler_error_then_valid` | MAC-11, MAC-23, CLI-9 | A file with a pre-assembler error followed by a valid file: no .am for the first, the second is assembled. |
| `missing_file_last` | ERR-1, CLI-6, CLI-9, CLI-11, ERR-R13 | A valid file followed by a name with no .as file: message without a line number, status 1. |
| `missing_file_only` | ERR-1, CLI-6, CLI-9 | The only argument names a file that does not exist. |
| `as_suffix_argument` | ERR-1, CLI-8 | The argument prog.as while prog.as exists: the assembler looks for prog.as.as and reports it. |
| `no_arguments` | CLI-5, CLI-9, CLI-10 | No file arguments: exactly one usage line that is not a diagnostic, status 0, no file created. |
| `missing_file_first` | CLI-11, ERR-1 | The first argument cannot be opened: the next file is still assembled, status 1. |
| `missing_file_middle` | CLI-11, ERR-R17 | A missing file between other files: its message stands between theirs, the files after it are processed. |
| `missing_files_several` | CLI-11, ERR-1 | Three missing arguments (one name twice) around a valid file: one message each, in order. |
| `order_several_files` | ERR-R17, CLI-3 | Five arguments with first-pass errors, pre-assembler errors, a missing file, a warning plus a second-pass error, and a warning: messages in command-line order. |
| `warnings_only_status_zero` | ERR-R14, CLI-9, WRN-1 | Two files whose only messages are warnings: outputs written, status 0. |
| `stale_files_of_missing_file` | OUT-14, CLI-11 | Stale outputs of a name whose .as file does not exist are removed; stale .ent and .ext of the valid file too. |
| `source_is_directory` | CLI-12, ERR-47, CLI-9 | `d.as` is a directory (made by `setup.sh`): one message without a line number, no output file for `d`, the next file is assembled, status 1. **Fails until defect D-2 is fixed.** |
| `source_is_directory_stale` | CLI-12, ERR-47, OUT-14 | The same with stale `d.am`, `d.ob`, `d.ent`, `d.ext`: all four are removed. **Fails until defect D-2 is fixed.** |
| `same_file_twice` | CLI-3, CLI-9, ERR-R17 | `good bad bad good`: the faulty file is reported once per mention, the outputs of the valid one stay, status 1. |
| `same_valid_file_twice` | CLI-3, CLI-9 | The same valid name twice: no message, outputs written, status 0. |
| `missing_file_long_name` | ERR-1, CLI-11 | A base name of 250 characters with no file: one message, the next file is assembled, status 1. |
| `argument_with_directory` | CLI-8, ERR-R13, ERR-1 | Arguments `sub/e1` and `sub/nosuch` (`setup.sh` moves `e1.as` into `sub/`): the messages start with `sub/e1.as:2:` and `sub/nosuch.as:`. |
| `dash_arguments` | CLI-8, ERR-1 | `--` and `-h` are file names like any other: `--.as` and `-h.as` cannot be opened. |
