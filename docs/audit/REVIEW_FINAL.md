# Final review of the assembler before the owner's approval

Reviewer role: read-only and independent. Nothing in the repository was
changed; `git status --short` was empty at the end (build products are
ignored by `.gitignore`). All scratch work is under
`<scratchpad>/`.
Tools: gcc 13.3.0, clang 18.1.3, valgrind, Python 3. Head commit: `bec4566`.

## Verdict

**READY AFTER SMALL FIXES. 0 BLOCKER, 9 SHOULD, 5 NICE.**

No behaviour contradicts SPEC, no crash, hang, leak or memory error was
found by any run, and no binding coding rule is violated in the code. The
SHOULD findings are four stale or incomplete comments, three message
improvements, and two documentation items outside the code, one of which
(README and CHANGELOG, the earlier finding Q-M03) must be done before the
repository is published, although it does not touch the code.

Every source file was read completely: 18 `.c` files and 18 `.h` files,
6,589 lines, plus the Makefile (73 lines).

---

## Part A: behaviour

### A1. Build and warnings

| Command | Result |
|---|---|
| `make clean && make` (`gcc -Wall -ansi -pedantic -MMD -MP`) | 18 objects and the binary, **0 warnings** |
| gcc, each file, `-Wall -ansi -pedantic -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2` | **0 warnings** |
| the same plus `-Wconversion -Wcast-qual -Wwrite-strings` | **0 warnings** |
| clang 18, `-Wall -ansi -pedantic -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2` | **0 warnings** |
| clang `-Weverything -Wno-padded -Wno-declaration-after-statement -Wno-unsafe-buffer-usage` | 4 warnings, all intended: 2 `-Wswitch-enum` (a `default` label), 2 `-Wbad-function-cast` (`(int *)util_reserve(...)`). `-Wformat-nonliteral`, which the earlier review saw, is gone. |

### A2. Test suite

`make test` ends with `passed 195, failed 0, total 195` (76 valid, 97 error
and 22 command-line cases).

### A3. valgrind over every test input

Script: `scratchpad/vg.sh`. Every case directory was copied to a scratch
directory exactly as the runner does (the `.as` files, the `stale/` files,
`setup.sh` where present) and the binary (a `-g -O0` scratch build) was run
with the arguments of the case under
`valgrind -q --leak-check=full --show-leak-kinds=all --track-fds=yes --error-exitcode=9`.

- **Runs: 195. Runs with an error, a leak, a "still reachable" block or an
  open file descriptor of the program: 0.** (81 runs exit with 0, 114 with
  1; none with 9.) The only descriptor valgrind lists is its own log file.

Beyond what was asked, allocation failures were injected (scratch build
linked with `-Wl,--wrap=malloc,--wrap=realloc`, the N-th allocation returns
NULL, N = 1, 2, 3, ... until the run no longer fails), under the same
valgrind options, for 12 inputs with two file arguments each:

- **Runs: 1,156. Bad: 0.** Every failed run printed exactly one
  `<name>.as: error: out of memory; the assembler stops` as its last line,
  exited with 1, did not go on to the second file (ERR-R10) and left no
  leak and no invalid access. This is the evidence for "free everything on
  every path, including error paths".

### A4. Robustness: a fresh mutation fuzzer

Script: `scratchpad/fuzz/fuzz.py`. Corpus: the 206 `.as` files of the suite.
Mutations: bit flips, random bytes, insertions, deletions, truncation, line
duplication, line swaps and removals, splicing with another seed, repetition
of a chunk up to 40 times, insertion of language tokens (`mcro`, `mcroend`,
directive names, brackets, quotes, 90-character runs, 31-character names,
`%s%n%d`), NUL bytes and bytes 128 to 255, CR and CR LF. One to eight
mutations per input. Binary: scratch build with
`-fsanitize=address,undefined -g -O1`, leak detection on (checked first on a
deliberately leaking program), each run under a 10 second timeout.

Besides crashes, hangs and sanitizer output, every run was checked for:
exit status 0 or 1 only; empty `stderr`; every `stdout` line in the form of
ERR-R13 or ERR-R14; status 0 implies `.am` and `.ob` exist and no error
line; status 1 implies at least one error line and no `.ob`, `.ent`, `.ext`
(ERR-R6); no line number beyond the last line of the input; at most one
error per line for the pre-assembler and for inputs without macros
(ERR-R16); and, for an accepted input without macros, an `.am` file equal to
the input with normalised line endings (MAC-29, SYN-24, SYN-25).

- **Iterations: 60,400 (40,000 + 20,000 + 400, three seeds).
  Crashes: 0. Hangs: 0. Sanitizer reports: 0. Invariant violations: 0.**
  7,089 inputs assembled, 53,311 were rejected. Nothing to reduce.

I/O failure paths were probed by hand as well (file-size limit with
`ulimit -f`, a non-empty directory in the place of an output file): a write
failure of `.am` gives `cannot write the '.am' file`, status 1, no `.am`; a
write failure of `.ob` and a blocked `.ob` or `.ext` give one message,
status 1 and none of the three output files.

### A5. Diagnostic texts

All 76 call sites were read (28 calls of `errors_report`, 46 calls of the
parser's `fail`, which forwards to `errors_report_list`, the one
`errors_warn` and the out-of-memory line). They follow
the style block of `errors.h`: lower case, no final period, the offending
token quoted, limits taken from the constants, one vocabulary. No course
jargon (no IC, DC, A,R,E, "first pass") appears in any text. No output call
outside `errors.c` prints a diagnostic; the only other `stdout` text is the
usage line (`assembler.c:257`). The ones I would change:

| ID | Where | Text today | Proposed |
|---|---|---|---|
| R-1 (S-5) | `preassembler.c:393` | `a label is not allowed in front of 'mcro', 'mcroend' or a macro call` (one text for three cases, although `info.kind` is known; `preassembler.c:449` is already specific) | by kind: `a label is not allowed in front of 'mcro'`, `... in front of 'mcroend'`, `... in front of the call of the macro '%s'` |
| R-2 (S-6) | `syntax.c:232` | `the %s '%s' is longer than the %d characters of a label name` | `the %s '%s' is longer than %d characters, the limit of a label name` |
| R-3 (S-7) | `syntax.c:1033-1043`, `syntax.c:594-605` | `movr1,r2` gives `unknown operation 'movr1,r2'` and `.externlabel` gives `unknown directive '.externlabel'`: the specific text for ERR-35 and ERR-36 is found only when the glued operand does not start with a lowercase letter | look for the longest prefix of the lowercase run that is an operation (or directive) name, and then print the existing `white space is missing after the operation name 'mov'` |

### A6. SPEC conformance probes

Twelve probe files were written from SPEC alone (`scratchpad/probe/`) and
every output line was checked by hand. All agree with SPEC.

| # | Rules | Probe | Result |
|---|---|---|---|
| 1 | ENC-3, ADR-3, ADR-5, ADR-7, ADR-9 to ADR-13, ENC-8, ENC-15, OUT-4, OUT-13 | `p1.as`: all four modes as source and as destination, two registers, two matrices in one instruction, an external, negative limits | the 28 code words and 10 data words equal the words I derived by hand from the field layout (`mov #-128,r0` = `aaada`, `caaaa`, `aaaaa`; `cmp r1,r7` = `abdda`, `abbda`; five words for `mov MAT[r1][r2],MAT[r3][r4]`); header `bda<TAB>cc` |
| 2 | OUT-8, OUT-9, OUT-2, DIR-34 | `p1`, `p8` | `.ext` has one line per use (`EXT bcda`, `EXT bdba`); `.ent` once for a symbol named twice; no `.ext` when the external is never used |
| 3 | LBL-2 to LBL-6, LBL-16, LBL-18, LBL-20, LBL-22, ERR-11 to ERR-16, ERR-38, ERR-45, SYN-9 | `p2.as` | 30 characters accepted, 31 rejected; `a` and `A` distinct; `r8`, `PSW`, `PC` legal; `LABEL:stop`, `L : stop`, `1x:`, `data:`, `r3:`, `mcro:`, a label alone, a second `a:`, `x_y:` and ` ; text` each get exactly one error on the right line |
| 4 | SYN-13, DIR-6, ERR-19, ERR-20, ERR-26 to ERR-30 | `p3.as`, 20 lines | every comma fault and operand count fault reported once; the two legal lines with white space around commas accepted |
| 5 | MAC-28, MAC-30, MAC-31, MAC-33, MAC-34, MAC-36 to MAC-38, SYN-2b, MAC-23 | `p4.as`, 31 lines | the 12 expected messages on lines 4, 8, 11, 13, 15, 17, 18, 19, 21, 25, 27, 28 and nothing else; the over-long line inside a discarded body is silent; `L: mcro lab` still defines `lab`; no `.am` left |
| 6 | ERR-R12, MAC-35, ERR-R17, LBL-21, DIR-35 | `p5.as`, `p5b.as` | an error and a warning in a macro body called twice are printed twice with the body's `.as` lines (4 and 5), in `.am` order after the earlier line 7; second-pass errors likewise |
| 7 | ERR-R16 | `p6.as` | five lines with several faults each: one message per line |
| 8 | OUT-14, OUT-11, ERR-1, CLI-8, CLI-10, CLI-11 | `p7`, `nosuch`, `p7ok` with stale `.am .ob .ent .ext` | all stale files gone for the failing, the missing and the valid file; `p1.as` as an argument gives `p1.as.as: error: ...`; no arguments gives one usage line and status 0 |
| 9 | OUT-15, OUT-13 | empty file, comments only | `.ob` is `a<TAB>a`, `.am` is empty, status 0, nothing printed |
| 10 | SYN-2, SYN-24, SYN-25 | `p9.as`, `p9b.as` | 80 characters plus CR LF accepted, 81 rejected; CR LF dropped from `.am`; a last line without a newline gets one |
| 11 | ALG-15, ALG-17, ALG-6 | `p10a`, `p10b`, `p10d`, `p10e` | 156 words accepted (last address `dddd`), 157 rejected; reported once, on the line that crosses the limit, although a later line overflows again; data symbols relocated by ICF (`D` at 101, `E` at 255) |
| 12 | SYN-21, DIR-24 to DIR-27, DIR-32, DIR-33, SYN-17, SYN-18, SYN-22, SYN-23, INS-4, DIR-23, DIR-36, ERR-50 to ERR-53 | `p10c.as` (41 lines), `p11`, `p12`, `p12b`, `p13` | every limit, every `.mat` and `.string` form, every bracket and white-space rule and all four symbol conflicts behave as SPEC says, each with one message on the right line |

---

## Part B: code

### B7. The binding coding rules, one by one

| Rule | Finding |
|---|---|
| One module per responsibility, `.c`/`.h` pairs | No violation found. All twelve required responsibilities exist. Six modules go beyond the list (`statement`, `am_reader`, `line_map`, `reference_list`, `util`, `encoder`); each header states why it exists, but the adapted split is not recorded in `docs/PROGRESS.md` (S-9). |
| Hidden implementations, opaque types | No violation found. No header defines a structure. Ten opaque types (`ErrorContext`, `Instruction`, `MacroTable`, `Macro`, `LineMap`, `Statement`, `SymbolTable`, `Symbol`, `MemoryImage`, `ReferenceList`); the symbol table is reached only through functions and a visitor. |
| One constant instruction table, no name chains | No violation found. `instruction_table.c:47-63` is the only list of operations; legality is a bit-mask lookup; the parser's `directive_rules` (`syntax.c:612`) is keyed by `DirectiveKind`, not by name. The only `strcmp` calls on language words are the two keywords in `is_reserved_word`. |
| No global variables | No violation found. `nm -g --defined-only *.o` lists functions only. File-scope objects are five `static const` tables; no array has external linkage (IMP-1). |
| Private functions are `static`; about 40 lines at most | No violation found. Every external symbol is declared in its module's header (compared with `nm`). 208 functions were measured (`scratchpad/funclen.py`, from the first line of the signature to the closing brace): **none is over 40 lines.** The longest: `parse_label` 40 (`syntax.c:1132`), `util_read_line` 40 (`util.c:185`), `parse_dimension` 39 (`syntax.c:467`), `handle_outside` 35 (`preassembler.c:386`), `parse_line` 33. |
| No magic numbers | No violation of substance. Outside `#define` lines and enumerations the only literals other than 0 and 1 are the opcodes and operand counts inside the instruction table itself. See N-1 for the bare shift `0`. |
| All messages through the error module | No violation found. |
| Fixed-size arrays only for the image; dynamic tables; every allocation checked; everything freed | No violation found. 13 `malloc` sites and 1 `realloc` site, all checked; fixed arrays only in `memory_image.c:15-16`, the operand array (`statement.c:55`) and base-4 buffers, which `CLAUDE.md` now allows explicitly. Evidence: A3 (195 runs and 1,156 injected failures, 0 leaks). |
| Documentation | No violation found by script: every file starts with a header comment, every function definition and every prototype has a comment directly above it. Four comments are stale or incomplete (S-1 to S-4). |
| C90 | No violation found. No `//`, every header guarded, `-ansi -pedantic` clean with gcc and clang. |
| Zero warnings; Makefile | No violation found. Targets `all`, `test`, `clean` exist and are phony; dependencies are generated with `-MMD -MP` (`syntax.d` lists all six headers it reaches). |

Formatting: no tab, no trailing white space, no line over 80 columns in any
source file.

### B8. The earlier quality review (`docs/audit/REVIEW_QUALITY.md`)

| Finding | State |
|---|---|
| Q-M01 fixed arrays and the false sentence | Resolved: `memory_image.h:9-10` corrected, the reading recorded in `CLAUDE.md`. |
| Q-M02 non-English text in `PROGRESS.md` | Resolved: the file is ASCII, the local path is gone. |
| **Q-M03 `README.md` and `CHANGELOG.md` describe deleted code** | **Not resolved (S-8).** Both files are unchanged: `README.md:1` `B"H`, `:5` "Work in Progress - Encoding Bug Under Repair", twelve files that do not exist, `./assembler formal_tester.as` at `:113`, emoji headings, a contact block. It is documentation scheduled for Phase 5, not code, but the repository must not be published with it. |
| Q-S01 growing array written five times | Resolved: `util_reserve` (`util.c:122`) with an overflow check, five callers. |
| Q-S02 heap copies for lookups | Resolved: `instruction_find_text`, `directive_find_text`; `register_number` calls `register_number_text`. |
| Q-S03 line loop written twice | Resolved: `am_reader`. |
| Q-S04 two lists of output files | Resolved: `remove_output_files` (`output_writer.c:260`). |
| Q-S05 directives enumerated in four places | Resolved in substance: one `directive_rules` table and `statement_stores_data`. A three-label switch remains in `statement.c:231-239`; acceptable. |
| Q-S06 matrix index word known twice | Resolved: `statement_operand_index_word_offset`. |
| Q-S07 duplicated small definitions | Resolved: `ARRAY_LENGTH`, file modes and `NEWLINE_CHARACTER` in `common.h`; `STRINGIFY` is gone. |
| Q-S08 own header first | Resolved in all 17 modules. |
| Q-S09 `macro_table_add` two meanings of NULL | Resolved. |
| Q-S10 `visit_entry` two jobs | Resolved: `count_entry`, `write_entry`. |
| Q-S11 three unused public functions | Resolved; a new check finds no public function without a caller outside its module. |
| Q-S12 two print paths | Resolved: `print_line` over `print_diagnostic`. |
| Q-S13 wording not uniform | Resolved; three leftovers of taste are R-1 to R-3. |
| Q-S14 formats the compiler cannot check | Resolved: every format is a literal (or a choice of two literals) at the call; `-Wformat=2` is clean. |
| Q-S15 read error taken for end of file | Resolved: `READ_ERROR`, reported by the pre-assembler and by `am_reader`. |
| Q-S16 `syntax.c` two modules | Resolved: `statement` and `syntax`. |
| Q-S17 addressing mode as "no operand" | Resolved: `MODE_FIELD_UNUSED`. |
| Unnumbered: `syntax.h:8` "SPEC sections 5.3" | **Not resolved (S-3).** |
| Unnumbered: `docs/PROGRESS.md:49` says `statement_parse` | **Not resolved (S-4)**; the function is `syntax_parse`. |

### B9. What six refactorings left behind

Verified with grep. No TODO, FIXME, `#if 0`, dead function, unused public
function or leftover of a removed name (`STRINGIFY`, `fail_at`,
`visit_entry`, `is_data_statement`, `instruction_name`, `line_map_count`,
`syntax_is_valid_label`) exists in the code. `statement.h:4` and
`second_pass.c:4-5` name the syntax module correctly. What is left:

- S-1 to S-4 (stale or incomplete comments and documents, below).
- N-2 to N-4 (naming, single-use helpers, repeated lookups, below).
- `instruction_table.c:11` includes `<stddef.h>`, which its own header
  already includes; `encoder.h:13` includes `common.h` although only
  `encoder.c` needs it. Harmless.

### B10. Interfaces and dependencies

Every header can be read on its own: it states the role of the module, the
ownership of every pointer and the meaning of every return value, and cites
SPEC rule IDs. The include graph is acyclic and layered. Direct
dependencies (project headers included by the `.c` or the `.h`):

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
common            -> (nothing)
```

Judgement. The layering is what a reader expects: driver, stages, the
reader and parser, the tables, the leaves. Neither pass knows the other,
the parser does not know the structure of a statement, and only `encoder`
knows a bit position. Nothing is over-engineered. Three things a first
reader will notice, none of which needs a change: `statement.h` is wide (32
functions, 14 of them building functions that only the parser may use; the
passes get a `const Statement *`, which enforces it); `instruction_table`
also owns the registers, the directives and the reserved words, which its
header says; and a piece of a line is described in three ways (`Field` in
`preassembler.c`, a start and end pair in `syntax.c`, text and length in
the table interfaces).

---

## Findings

### BLOCKER

None.

### SHOULD

| ID | File and line | Finding | Fix |
|---|---|---|---|
| S-1 | `first_pass.h:12-14` | The comment says the syntax errors "are found by the syntax module, which this pass calls for every line". Since the `am_reader` step the pass no longer calls the parser; `am_reader` does. | "...are found by the syntax module while `am_reader` parses every line for this pass." |
| S-2 | `preassembler.h:7` | "reports every pre-assembler error of SPEC section 5.2 (ERR-2 to ERR-10)". Two later fixes added ERR-46 (NUL byte, `preassembler.c:506`) and ERR-47 (unreadable source, `preassembler.c:546`); the contract does not mention them. | Add "and ERR-46, and ERR-47 for a source file that cannot be read (CLI-12)". |
| S-3 | `syntax.h:8` | "SPEC sections 5.3": already listed as stale by the earlier review, still there. | "SPEC section 5.3". |
| S-4 | `docs/PROGRESS.md:49`, `docs/PROGRESS.md:12`, `tests/README.md:65` | `statement_parse` does not exist (it is `syntax_parse`); the suite is said to have 175 cases (it has 195); the layout lists a directory `tests/sh/` that does not exist. | Correct the three lines when `PROGRESS.md` is updated for the end of Phase 4. |
| S-5 | `preassembler.c:393-395` | Message R-1: one text for three different faults. | Choose the text by `info.kind`, as `preassembler.c:449` already does. |
| S-6 | `syntax.c:232-234` | Message R-2: awkward wording. | The proposed text of R-2. |
| S-7 | `syntax.c:1027-1044`, `syntax.c:588-606` | Message R-3: the most common form of ERR-35 (`movr1,r2`, `incr1`) gets the generic "unknown operation" text. Behaviour is correct (one error, right line); only the text is less helpful than the one that exists. | Try the prefixes of the lowercase run, longest first, with `instruction_find_text` / `directive_find_text`. |
| S-8 | `README.md`, `CHANGELOG.md` | Earlier finding Q-M03, unresolved: both describe the deleted code, say the project has an encoding bug, are not plain English text (emoji, `B"H`) and carry a contact block. | Rewrite in Phase 5 as planned; do not publish before. The owner should also know that the git history holds four spellings of the author identity with an e-mail address. |
| S-9 | `docs/PROGRESS.md` (decisions) | `CLAUDE.md` allows an adapted module split "with a recorded reason". The reasons for `statement`, `am_reader`, `line_map`, `reference_list`, `util` and `encoder` are in their headers and in commit messages only. | One paragraph in `PROGRESS.md`; the module table below can be reused. |

### NICE (five)

| ID | File and line | Remark |
|---|---|---|
| N-1 | `encoder.c:52, 59, 68, 82, 88` | The shift `0` is a bare literal next to named shifts for every other field (earlier Q-N07). An `ARE_SHIFT` would complete the set. |
| N-2 | `instruction_table.h:82-108`, `output_writer.h:30-46`, `preassembler.c:362`, `am_reader.c:28`, `first_pass.c:44` | Naming: `is_reserved_word`, `register_number`, `directive_find_text`, `write_output_files`, `remove_output_files` have no module prefix, unlike all other modules; the same out-of-memory flag is called `enough_memory`, `memory_left` and `no_memory` in different modules. |
| N-3 | `util.c:176`, `am_reader.c:44`, `first_pass.c:409` | `store_length` and `report_unreadable` are two-statement helpers with one caller that exist for the line budget; `init_pass` exists in the first pass while `second_pass.c:163-169` initialises its state inline. |
| N-4 | `syntax.c:829-832`, `second_pass.c:41` and `:59`, `base4.c:24` and `:34` | Small repetitions: `register_number(name)` is called twice; the entry symbol is looked up twice and the Boolean result of `symbol_table_mark_entry` is not used; a digit is masked twice. |
| N-5 | `errors.h:17` against `syntax.c:328, 605, 1043` and others | Rule 1 of the style block promises "plain ASCII", but a quoted token is copied from the source byte for byte, so control characters and bytes above 127 reach the terminal. Either print such bytes as `?` in one place in `errors.c`, or say "apart from text quoted from the line". |

---

## Part C: the modules as they are

Generated from the headers (`scratchpad/modtab.py`) and compared with
`nm -g --defined-only` on every object file: the lists match exactly.
Lines are those of the `.c` file plus the `.h` file. "Other types" are
public enumerations and function-pointer types.

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
external functions including `main`; 208 function definitions in all;
6,589 lines of C (4,914 in `.c` files, 1,675 in headers), and a Makefile of
73 lines.
