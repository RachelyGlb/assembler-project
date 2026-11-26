# Code-quality review of the rebuilt assembler

Role: reviewer (code quality), read-only and independent. Date: 2026-11-26.
Reviewed state: commit `7a476d4`, working tree clean. No file in the
repository was changed; all builds and runs were done on a copy in the
scratchpad.

## Verdict

**MINOR ISSUES.** 3 MUST, 17 SHOULD, 12 NICE.

The code is in very good shape. All sixteen modules were read completely.
None of the three MUST findings is a defect in the code: one is a binding
rule that the code breaks by its letter and a header comment that states the
opposite; the other two are repository documents. The SHOULD findings are
mostly duplication between modules, which matters because the stated selling
point is "modular and clean".

## How the review was done

- Every `.c` and `.h` file, the `Makefile`, `README.md`, `CHANGELOG.md`,
  `.gitignore`, `CLAUDE.md`, SPEC sections 1 to 3 and rule IMP-1 were read.
- Builds on a copy: `gcc 13.3.0 -Wall -ansi -pedantic` prints nothing.
  `-Wextra -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes`
  prints nothing. `clang 18.1.3 -Wall -Wextra -ansi -pedantic` prints nothing.
- `make test` on the copy: `passed 175, failed 0, total 175`.
- Every header was compiled alone (`#include "x.h"` only): all sixteen are
  self-contained.
- Function lengths were measured brace to brace with a script (197
  functions).
- `nm` on the objects: the only external symbol without a prototype in a
  header is `main`; there is no writable object at file scope.
- valgrind on a valid file, two error files and a missing file: no error and
  no leak.
- Allocation-failure injection: a binary in which the N-th `malloc` or
  `realloc` of the project code fails was run under valgrind for N = 1 to
  146 (every allocation of a run that uses macros, all directives, an entry
  and an external). Result: 146 runs, no leak, no invalid access, exit
  status 1 each time, and no `.ob`, `.ent` or `.ext` file left behind.

## Findings by rule

Ranks: MUST violates a binding rule; SHOULD is what an experienced reviewer
would ask for; NICE is taste. Sizes: trivial (minutes), small (under an
hour), medium (one sitting).

### Rule 1: modules, duplication, dependencies, includes

Dependencies: no violation found. The include graph is acyclic and layered:
`assembler` → stages (`preassembler`, `first_pass`, `second_pass`,
`output_writer`) → `syntax`, tables, `encoder`, `base4` → `instruction_table`,
`errors`, `util` → `common.h`. Every header is self-contained and none
includes a project header it does not need, with one nit: `errors.h:17`
includes `common.h` although only `errors.c` uses it (`NO_LINE`,
`SOURCE_EXTENSION`).

The instruction length is computed in one place (`syntax.c:1387-1432`) and
the bit positions in one place (`encoder.c`): no violation found there.

**Q-S01 (SHOULD, medium). The growing-array idiom is written five times.**
`util.c:95-115`, `macro_table.c:132-153`, `line_map.c:54-66`,
`reference_list.c:69-89` and `syntax.c:315-329` each contain the same "if
full, double the capacity, `realloc`, check, store" code with their own pair
of `*_INITIAL_CAPACITY` and `*_GROWTH_FACTOR` constants (ten constants in
all). None of the five checks the doubling for overflow (`capacity * 2` in
`int`, then `(size_t)capacity * sizeof`), which is harmless on a 32-bit `int`
but is exactly the kind of size computation a reviewer looks for. Fix: one
helper in `util`, for example
`Boolean util_reserve(void **items, int *capacity, int count, size_t element_size)`,
which holds the growth policy and the overflow check once; the five call
sites shrink to one call each.

**Q-S02 (SHOULD, small). Tokens are copied to the heap only to be looked
up.** `syntax.c:608-620` (`lookup_directive`), `syntax.c:1036-1042` and
`syntax.c:1062-1069` allocate a copy of a piece of the line, call
`directive_find` or `instruction_find`, and free it. Registers and macros
already have a lookup by text and length (`register_number_text`,
`macro_table_find_text`), so the same thing is done in two ways, and the
parser carries three allocations and their out-of-memory paths that it does
not need (`skip_ignored_label` returns NULL only for that reason). Fix: add
`instruction_find_text(text, length)` and `directive_find_text(text, length)`
and implement the string versions on top of them, as `macro_table.c:176-179`
already does. In the same step make `register_number`
(`instruction_table.c:129-141`) call `register_number_text`; today the two
loops are duplicates.

**Q-S03 (SHOULD, small). The line loop of a pass is written twice.**
`first_pass.c:393-417` and `second_pass.c:161-185` are the same statements:
line buffer, `.am` line counter, `errors_before`, the `util_read_line` loop
with the line-map translation, `free`, the fatal test. Fix: one helper that
reads the `.am` file and calls a function per line with the translated line
number, for example in a small `am_reader` unit or in `util`; each pass keeps
only its `handle_line`.

**Q-S04 (SHOULD, small). Two places remove output files and list the output
extensions.** `assembler.c:74-93` (`remove_stale_outputs`, four extensions,
`OUTPUT_FILE_COUNT`) and `output_writer.c:267-283` (`discard_outputs`, three
extensions, `WRITTEN_FILE_COUNT`). A new output file would have to be added
in both. Fix: `output_writer` exports one function that removes its three
files; the driver calls it and removes the `.am` file itself.

**Q-S05 (SHOULD, small). The five directives are enumerated in four
places.** The directive table (`instruction_table.c:73-78`), `DirectiveKind`
(`instruction_table.h:21-29`), five `STATEMENT_*` members
(`syntax.h:34-38`) with the switch that maps one enumeration to the other
(`syntax.c:670-695`), and the list of data directives in
`first_pass.c:50-58` and again in `first_pass.c:344-346`. Fix: a
`static const` dispatch table in `syntax.c` with one row per directive
(directive, statement kind, parsing function, "stores data"), and a
`statement_stores_data` accessor that replaces `is_data_statement` and the
three `case` labels. The switch disappears.

**Q-S06 (SHOULD, trivial). The position of a matrix operand's second word is
known in two modules.** `syntax.c:62-65` and `1387-1404` own the layout of
the additional words, but `first_pass.c:30` defines `MATRIX_INDEX_WORD 1` and
adds it at `first_pass.c:254`. Fix: let the syntax module answer the offset
of the index word (a second accessor, or a "which word" parameter), and
delete the constant from the first pass.

**Q-S07 (SHOULD, trivial). Small definitions are duplicated between
modules.** `STRINGIFY`/`STRINGIFY_TOKEN` in `preassembler.c:31-32` and
`syntax.c:72-73`; `WRITE_MODE` in `preassembler.c:35` and
`output_writer.c:18`; `NEWLINE` is defined in `util.c:18` while `'\n'` is
written as a literal in `errors.c:60` and `preassembler.c:192`;
`ARRAY_LENGTH` exists only in `instruction_table.c:27` while
`assembler.c:49` and `output_writer.c:24` count their arrays by hand. Fix:
move `STRINGIFY`, `ARRAY_LENGTH` and the file modes to `common.h`.

**Q-S08 (SHOULD, trivial). Fourteen of the sixteen `.c` files do not include
their own header first.** Only `base4.c:6` and `encoder.c:7` do. The others
include the standard headers first and then the project headers in
alphabetical order, so that in `first_pass.c:11`, `second_pass.c:11`,
`output_writer.c:13` and `preassembler.c:23` the own header is not even the
first project header. The headers are in fact self-contained (verified), so
this is a convention issue only; putting the own header first makes the
compiler prove that on every build.

**Q-N01 (NICE). The name syntax of a macro and of a label are two loops of
the same shape** (`preassembler.c:227-246`, `syntax.c:203-223`). The rules
differ (MAC-33 allows `_`), so two functions are defensible.

**Q-N02 (NICE). Three conventions for "a piece of a line":** a
`Field {start, length}` in `preassembler.c:55-59`, a `(start, end)` pointer
pair throughout `syntax.c`, and `(text, length)` in `util.h:68`,
`instruction_table.h:94` and `macro_table.h:70`.

**Q-N03 (NICE). `util` is a grab bag** (white space, string copies, file
names, line reading), and `instruction_table` also owns the registers, the
directives and the reserved words. Both headers say so honestly; no change
is needed unless Q-S01 and Q-S03 make `util` grow further.

### Rule 2: information hiding and const-correctness

No violation found. No header defines a structure; all ten data types
(`ErrorContext`, `Instruction`, `MacroTable`, `Macro`, `LineMap`,
`Statement`, `SymbolTable`, `Symbol`, `MemoryImage`, `ReferenceList`) are
opaque, and no module reaches into another's representation. Read-only
parameters are `const` throughout the public interfaces.

**Q-N04 (NICE).** `symbol_table.c:101` (`find_symbol`) takes a
`const SymbolTable *` and returns a writable `Symbol *`. It is legal and
private, but it launders constness.

### Rule 3: instruction table

No violation found. There is one `static const` instruction table
(`instruction_table.c:50-66`), a register table and a directive table, all
private and reached through functions (IMP-1). No `if`/`strcmp` chain over
names exists anywhere. The only switch that follows the directives is the one
of Q-S05.

### Rule 4: globals

No violation found. No `extern`, no object at file scope except the three
`static const` tables; the three other `static const` arrays are inside
functions (`assembler.c:76`, `base4.c:22`, `output_writer.c:269`).

### Rule 5: functions

No violation found for linkage and length. Every non-static function except
`main` is declared in a header. No function is longer than 40 lines; the
longest are `parse_directive` (36, `syntax.c:660`), `start_definition` (35,
`preassembler.c:315`), `first_pass` (35, `first_pass.c:389`), `parse_label`
(34, `syntax.c:1146`), `parse_dimension` (34, `syntax.c:498`),
`handle_outside` (33, `preassembler.c:359`), `macro_table_add` (33,
`macro_table.c:92`), `parse_operand` (32), `parse_line` (32),
`util_read_line` (31) and `parse_list_value` (30). Nesting never exceeds
three levels.

Of the functions between 30 and 40 lines, three do more than one thing:

- `first_pass` (`first_pass.c:389-426`) sets up the state, runs the line
  loop, decides the result and relocates the data symbols. Q-S03 removes the
  loop.
- `start_definition` (`preassembler.c:315-350`) checks the name, reports,
  copies the name and opens the definition.
- `macro_table_add` (`macro_table.c:92-125`) checks for a duplicate,
  allocates and links; see Q-S09.

**Q-S09 (SHOULD, trivial). `macro_table_add` returns NULL for two unrelated
reasons** (duplicate name or no memory, `macro_table.h:40-42`), and its only
caller looks the name up first anyway (`preassembler.c:263`). The symbol
table does it the other way round ("the caller asks first",
`symbol_table.h:50-52`). Fix: drop the duplicate check from
`macro_table_add` so that both tables have the same contract and NULL means
only "out of memory".

**Q-S10 (SHOULD, trivial). `visit_entry` does two jobs chosen by a NULL
sentinel** (`output_writer.c:181-195`, used at `211-224`): it counts when
`walk.file` is NULL and writes when it is not. Fix: two visitors, or a
`symbol_table` function that counts the entry symbols.

**Q-S11 (SHOULD, trivial). Three public functions are never called:**
`instruction_name` (`instruction_table.h:45`), `line_map_count`
(`line_map.h:45`) and `syntax_is_valid_label` (`syntax.h:215`, whose comment
calls it "the one rule for a label name"). Remove them.

**Q-N05 (NICE).** `first_pass` and `second_pass` take six parameters each.
**Q-N06 (NICE).** Names that do not say much: `fail` (`syntax.c:127`),
`field` (`encoder.c:39`), and the parameter `register_` (`syntax.c:740`),
which carries a trailing underscore to dodge the keyword; `number` would do.

### Rule 6: magic numbers

No violation of substance found. Every width, bit position, size, ASCII
bound and exit status has a name. Remaining literals:

**Q-N07 (NICE, trivial).** The shift `0` is passed as a bare literal five
times in `encoder.c:52, 59, 68, 82, 88`, next to named shifts for every
other field; an `ARE_SHIFT` would complete the set. `start + 1`
(`syntax.c:633`) skips the dot that `instruction_table.c:30` names
`DIRECTIVE_DOT_LENGTH`. `EXIT_STATUS_SUCCESS` and `EXIT_STATUS_FAILURE`
(`assembler.c:36-37`) shadow the standard `EXIT_SUCCESS` and `EXIT_FAILURE`;
this is defensible because CLI-9 fixes the values 0 and 1.

Constants repeated inside message texts are listed under Q-S13.

### Rule 7: error reporting

No module other than `errors` prints a diagnostic, and none calls `exit` or
`abort`. The other output calls are legitimate file output
(`output_writer.c:97`, `preassembler.c:191-192`) and the usage line (Q-N08).

**Q-S12 (SHOULD, trivial). The errors module has two print paths, and its
file comment says it has one.** `errors.c:4-6` states that every diagnostic
goes through `print_diagnostic`, but `errors_report_no_memory`
(`errors.c:100-104`) calls `print_prefix` and `puts` directly, so the layout
"prefix, text, newline" is written twice. Fix: a small variadic static
wrapper around `print_diagnostic` that `errors_report_no_memory` calls; then
the comment is true.

**Q-S13 (SHOULD, small). The wording of the messages is not uniform.** The
test runner never compares message text (`tests/run_tests.sh:198`), so this
can be changed without touching the tests.

- Quoting. Names and pieces of the line are quoted with `'...'` in most
  messages, but not in `syntax.c:365` ("the value %.*s"), `syntax.c:721`
  ("the immediate operand %.*s"), `syntax.c:648` ("the directive name
  %.*s") and `syntax.c:1046` ("the operation name %.*s").
- File messages have five shapes: "cannot open the %s file"
  (`assembler.c:164`, extension), "cannot open the source file"
  (`assembler.c:200`), "cannot create the file %s" and "cannot write the
  file %s" (`preassembler.c:546`, `524`, full name), "cannot create the %s
  output file" and "cannot write the %s output file" (`output_writer.c:56`,
  `81`, extension).
- The label messages name the label; the macro messages do not ("a macro
  with this name is already defined", "the macro name is a reserved word of
  the language", `preassembler.c:261-265`).
- "extra text after ..." (`preassembler.c:269`, `384`, `426`) against
  "there is extra text after ..." (`syntax.c:947`) and "there is text after
  ..." (`syntax.c:480`, `798`).
- Constants spelled out in the text although a definition exists: "r0 to
  r7" (`syntax.c:759`, `REGISTER_COUNT`), "'_'" (`preassembler.c:331`,
  `MACRO_NAME_EXTRA_CHARACTER`), "mcro" and "mcroend"
  (`preassembler.c:269`, `326`, `366`, `421`, `426`), while
  `MAX_NAME_LENGTH` is inserted with `STRINGIFY` in the same file.

**Q-S14 (SHOULD, small). Format strings that the compiler cannot check.**
`first_pass.c:97-109` passes a variable (`reason`) as the format of
`errors_report`, and `fail_at` (`syntax.c:143-150`) takes a format that
"must hold exactly one `%.*s`" by comment only. clang reports the root at
`errors.c:59` under `-Wformat-nonliteral`. Fix: give the two callers the
shape that `require_name` already has (`"%s '%s' %s"` with the variable
parts as arguments), so that every format is a literal at the call of
`errors_report`.

**Q-N08 (NICE).** `assembler.c:250` prints the usage line with `puts`. It is
not an error (the exit status is 0, CLI-10), so the binding rule does not
cover it, but it is the only text on standard output that does not come from
the errors module.

### Rule 8: memory and resources

Every `malloc` and `realloc` is checked. Ownership is documented in every
header ("it is copied", "owned", "the caller frees"). Every error path was
read; the injection run described above confirms that every one of the 146
allocation-failure paths frees and closes everything. `strcpy` appears twice
(`util.c:80-81`) into a buffer that was sized for it one line earlier; no
`strcat`, `sprintf`, `sscanf`, `atoi`, `strtok` or `strtol` is used. Every
`<ctype.h>` call casts its argument to `unsigned char` (nine calls). Numbers
are read with a clamp (`syntax.c:38`, `53`) so that no input can overflow a
`long` or a 16-bit `int`.

**Q-M01 (MUST by the letter, trivial). Fixed-size arrays exist outside the
memory image, and a header comment says they do not.** The binding rule
reads "Fixed-size arrays are allowed only for the machine-code image".
`syntax.c:103` holds `Operand operands[MAX_OPERAND_COUNT]`, and
`output_writer.c:109, 124, 125, 147, 148` hold `char x[BASE4_BUFFER_SIZE]`.
Both are bounded by construction (an instruction has at most two operands; a
base-4 number of an `unsigned int` has a known maximum length), so the intent
of the rule, no fixed capacity for anything the input controls, is kept, and
I do not recommend changing the code. But `memory_image.h:9-10` states "This
is the only module that uses arrays of a fixed size", which is false. Fix:
correct that sentence, and record the reading of the rule (arrays whose size
is a constant of the machine or of the C implementation are allowed) in
`CLAUDE.md` or `docs/PROGRESS.md`.

**Q-S15 (SHOULD, small). A read error is taken for the end of the file.**
`util_read_line` (`util.c:122-128`) cannot tell `EOF` from a failed read, and
neither `preassembler.c:485-503` nor the passes call `ferror` on the input,
although the output files are checked carefully (`preassembler.c:516`,
`output_writer.c:73`). A source that fails half way is assembled as if it
ended there. Fix: check `ferror` after each loop and report through
`errors_report`. This changes behaviour only when the read fails.

Not counted as findings: the return value of `remove` is ignored
(`assembler.c:89`), and `memory_image_add_code`/`add_data` results are
ignored after `has_room` succeeded (`first_pass.c:189`, `283`).

### Rule 9: documentation

No violation found. Every file has a header comment that states the role of
the module, including the `Makefile`. Every one of the 197 functions has a
comment directly above it; static functions carry the full form (what,
parameters, return value) and public definitions carry one line that points
to the full contract in the header. The style is the same in all sixteen
modules. Comments cite SPEC rule IDs rather than restating the code.

Comments that are wrong or stale:

- `memory_image.h:9-10`: see Q-M01.
- `errors.c:4-6`: see Q-S12.
- `syntax.h:215` ("the one rule for a label name"): the function is unused,
  see Q-S11.
- `docs/PROGRESS.md:49` says "a line is parsed once, by `statement_parse`";
  each line is parsed once per pass, which `second_pass.c:4-5` says
  correctly.
- `syntax.h:8`: "SPEC sections 5.3" should read "section".

### Rule 10: C90

No violation found. There is no `//` comment, no declaration after a
statement, every header has an include guard, and no library function
outside C90 is used. Signed values are never shifted when negative
(`encoder.c:42` converts to `unsigned` first and says why); no code depends
on the signedness of `char` or on an `int` wider than 16 bits.

Extra warnings, for information:

- gcc with `-Wextra -Wshadow -Wconversion -Wstrict-prototypes
  -Wmissing-prototypes`: nothing.
- clang `-Wall -Wextra`: nothing.
- `-Wswitch-enum` (gcc and clang): three switches rely on `default`
  (`first_pass.c:337`, `syntax.c:231`, `syntax.c:670`). Intended.
- clang `-Weverything`: besides the above only `-Wformat-nonliteral` at
  `errors.c:59` (see Q-S14).

**Q-N09 (NICE, pedantic). Identifiers in the reserved name spaces.** The
eleven `memory_image_*` functions begin with `mem` and a lowercase letter,
and `token_end` (`syntax.c:189`, in a file that includes `<ctype.h>`) begins
with `to` and a lowercase letter; C90 section 7.13 reserves both patterns
for future library functions. No compiler objects. The macros that begin
with `E` and an uppercase letter are fine, because `<errno.h>` is never
included.

### Rule 11: Makefile

No violation found. The targets `all`, `test` and `clean` exist and are
phony; header dependencies are generated with `-MMD -MP` and included, so
they are correct by construction; `clean` removes objects, dependency files,
the binary and `tests/out`. Portability notes only: the pattern rule and
`-include` need GNU make, and `-MMD -MP` need gcc or clang; objects do not
depend on the `Makefile` itself, so a change of `CFLAGS` needs a
`make clean`.

### Rule 12: consistency and naming

Formatting is uniform: four spaces, no tab, no trailing white space, no line
over 80 columns, Allman braces, braces on every block, `? TRUE : FALSE` for
every conversion to `Boolean`, the same create/destroy/NULL-tolerant pattern
for every heap-allocated type, and the same order in every header and source file.

**Q-N10 (NICE). Naming outliers.** `is_reserved_word`, `register_number`,
`register_number_text` and `directive_find` (`instruction_table.h`) have no
module prefix, unlike every other module. The module `syntax` exports
functions named `statement_*` and one `syntax_*`; the name of the file and
the name of its type disagree (see rule 14).

### Rule 13: the repository as a public portfolio

**Q-M02 (MUST, trivial). Non-English text in a tracked file.**
`docs/PROGRESS.md:193-194` contain two Hebrew file names, and lines 193-195
contain the local path `/mnt/project-files/`. `CLAUDE.md` requires
everything in the repository to be in English. Fix: describe the files in
English without the local names.

**Q-M03 (MUST before publication; the README rewrite is already planned).
`README.md` and `CHANGELOG.md` describe code that no longer exists.**

`README.md`, stale or wrong:

- Line 1, "B"H", and the emoji in every heading.
- Lines 5-9 and 73-95: "Work in Progress", "95% complete", "encoding bug in
  the second pass". All 175 tests pass.
- Lines 15-49: twelve files that do not exist (`assembler.h`, `types.h`,
  `macro_and_label_func.c`, `line_analysis.c/.h`, `line_parser.c`,
  `word_extractor.c`, `instruction_validation.c/.h`, `error_handling.c`,
  `file_utils.c`, `memory_builder.c/.h`, `formal_tester.*`,
  `instruction_count`, `.vscode/`), and no mention of `common.h`, `errors`,
  `util`, `macro_table`, `line_map`, `syntax`, `memory_image`, `encoder`,
  `base4`, `reference_list`, `docs/` or the test suite.
- Lines 60-79: a "Phase 3: Memory Image Building" stage that the program
  does not have.
- Lines 99-110: the example `./assembler formal_tester.as` is wrong; names
  are given without the extension (CLI-2), and `make test` is missing.
- Lines 122-127: "InstructionTable" and "MachineWord" are not types of the
  code.
- Lines 150-163: a to-do list for the old code.
- Lines 167-173: a contact block with a handle, to be checked against "no
  identifying details beyond the owner's name".

`CHANGELOG.md`, stale or wrong: dated 2025-08-18; the file list (lines
20-28) names `instruction_*`, `line_*`, `error_handling.c` and
`file_utils.c`; "symbol table with efficient lookup" (the table is a linked
list); emoji headings; placeholder dates "2025-08-XX". It says nothing about
the rebuild. Rewrite it or delete it.

Other items for the owner to decide before a push:

- `docs/audit/BUGS.md` and `docs/audit/STRUCTURE.md` (2,220 lines) describe
  the deleted code, with files and line numbers that no longer exist.
- `.gitignore:23` still has `!/formal_tester.ob`, a leftover of the old
  tree.
- The git history carries four spellings of the author identity with an
  e-mail address. This is not a file, but it is published with the
  repository.
- `tests/errors/string_illegal_char/` holds non-ASCII bytes on purpose (a
  test input); this is not a finding.
- No course material is tracked; the seven files under `docs/` are ignored.

### Rule 14: design smells

**Q-S16 (SHOULD, medium). `syntax.c` is two modules in one file.** See the
judgement below.

**Q-S17 (SHOULD, trivial). An addressing mode stands in for "no operand".**
`first_pass.c:204-205` initialises both mode fields with
`ADDRESSING_IMMEDIATE` because its value is zero, and `encoder.h:28-32`
documents the pun as part of the contract. Fix: a named constant for an
empty mode field in `encoder.h` (for example `MODE_FIELD_UNUSED 0`) and
`int` parameters, or two flags; the pun disappears.

**Q-N11 (NICE).** `memory_image_merge_code` silently ignores an address
outside the image, and `memory_image_code_word`/`data_word` return zero for a
bad index (`memory_image.c:69-78`, `99-116`). This hides a programming error
instead of exposing it.

**Q-N12 (NICE).** A register operand keeps a heap copy of its name that
nothing reads (`syntax.c:824-836`); and each line is parsed twice, once per
pass, where a list of statements kept from the first pass would do. Both are
simple and correct as they are.

## Judgement on `syntax.c`

`syntax.c` has 1,461 lines and 58 of the program's 197 functions; about half
of the lines are comments. It is cohesive in the sense that everything in it
serves "one line in, one `Statement` out", every function is short, and the
order of the file is easy to follow. It is not a rule violation.

It does hold two different things, and the public names already show the
seam: the functions are called `statement_*`, the file is called `syntax`.

1. **The statement model** (`syntax.c:84-109` and `1225-1455`, about 300
   lines): the `Statement` and `Operand` structures, create and destroy,
   sixteen accessors, and the word-layout arithmetic of ADR-14
   (`operand_words`, `statement_operand_word_offset`, `instruction_words`,
   `statement_word_count`). None of this is parsing; the layout arithmetic
   in particular is what both passes depend on.
2. **The parser** (`syntax.c:120-1219`, about 1,100 lines): lexical helpers,
   names and numbers, directive operands (303-696), instruction operands
   (698-1077), label and line (1079-1219).

Recommended split, along that seam: `statement.c`/`statement.h` own the
structure, the accessors, the layout and a small set of builder functions
(set label, set instruction, add operand, add value, set data count);
`syntax.c`/`syntax.h` keep `statement_parse` (better named `syntax_parse`)
and fill the statement through the builders. The structure stays opaque to
everyone, including the parser, which is the strictest reading of the hiding
rule. A further cut of the parser into directive parsing and operand parsing
is possible but would need a private header for `Parser`, `fail` and the
token helpers; I would not do it unless the first cut still leaves the parser
uncomfortably long (it would be about 1,100 lines, half of them comments).

## The modules as they are

Lines are `.c` + `.h`. `common.h` (96 lines) holds shared definitions only.

| Module | Single responsibility | Opaque types | Public functions | Lines |
|---|---|---|---|---|
| `assembler` | Program entry and per-file driver: runs the stages, owns and frees everything of one file. | none (private `FileJob`) | `main` | 268 + 0 |
| `errors` | The one place that prints diagnostics and counts errors. | `ErrorContext` | `errors_create`, `errors_report`, `errors_warn`, `errors_report_no_memory`, `errors_count`, `errors_destroy` | 116 + 68 |
| `util` | Helpers that know nothing about the language: white space, string copies, file names, reading a line. | none | `util_is_space`, `util_skip_spaces`, `util_skip_field`, `util_is_blank`, `util_copy_string`, `util_copy_text`, `util_make_file_name`, `util_read_line` | 150 + 98 |
| `instruction_table` | The words of the language: the instruction table, registers, directives, reserved words. | `Instruction` | `instruction_find`, `instruction_name`, `instruction_opcode`, `instruction_operand_count`, `instruction_allows_source`, `instruction_allows_destination`, `register_number`, `register_number_text`, `directive_find`, `is_reserved_word` | 206 + 112 |
| `macro_table` | The macros of one file with their body lines and source line numbers. | `MacroTable`, `Macro` | `macro_table_create`, `macro_table_destroy`, `macro_table_add`, `macro_add_line`, `macro_table_find`, `macro_table_find_text`, `macro_name`, `macro_line_count`, `macro_line`, `macro_line_source` | 220 + 99 |
| `line_map` | The map from `.am` line numbers to `.as` line numbers. | `LineMap` | `line_map_create`, `line_map_destroy`, `line_map_append`, `line_map_count`, `line_map_source_line` | 85 + 56 |
| `preassembler` | Macro expansion: reads the `.as` file, writes the `.am` file, reports pre-assembler errors. | none | `preassemble` | 558 + 42 |
| `syntax` | Parses one `.am` line into a `Statement` and answers questions about it, including its length in words. | `Statement` | `statement_parse`, `statement_destroy`, `statement_is_valid`, `statement_kind`, `statement_label`, `statement_has_ignored_label`, `statement_instruction`, `statement_operand_count`, `statement_operand_is_source`, `statement_operand_mode`, `statement_operand_value`, `statement_operand_symbol`, `statement_operand_matrix_register`, `statement_operand_word_offset`, `statement_word_count`, `statement_data_count`, `statement_data_value`, `statement_directive_symbol`, `syntax_is_valid_label` | 1,461 + 217 |
| `symbol_table` | The symbols of one file: value, kind, entry attribute. | `SymbolTable`, `Symbol` | `symbol_table_create`, `symbol_table_destroy`, `symbol_table_add`, `symbol_table_find`, `symbol_table_mark_entry`, `symbol_table_relocate_data`, `symbol_table_for_each`, `symbol_name`, `symbol_address`, `symbol_kind`, `symbol_is_entry` | 182 + 123 |
| `memory_image` | The code image and the data image with IC and DC and the memory limit. | `MemoryImage` | `memory_image_create`, `memory_image_destroy`, `memory_image_free_words`, `memory_image_add_code`, `memory_image_add_data`, `memory_image_merge_code`, `memory_image_instruction_counter`, `memory_image_data_counter`, `memory_image_code_count`, `memory_image_code_word`, `memory_image_data_word` | 116 + 102 |
| `encoder` | The bit layout of the machine words; pure functions. | none | `encode_first_word`, `encode_immediate`, `encode_registers`, `encode_matrix_indices`, `encode_address`, `encode_data` | 89 + 84 |
| `base4` | The "unique base 4" notation of numbers. | none | `base4_format`, `base4_format_shortest` | 52 + 49 |
| `reference_list` | An ordered list of (name, address) pairs: the uses of external symbols. | `ReferenceList` | `reference_list_create`, `reference_list_destroy`, `reference_list_append`, `reference_list_count`, `reference_list_name`, `reference_list_address` | 128 + 60 |
| `first_pass` | First pass: defines symbols, stores data, stores every instruction bit that needs no symbol. | none | `first_pass` | 426 + 48 |
| `second_pass` | Second pass: marks entries, completes the symbol words, records external uses. | none | `second_pass` | 187 + 45 |
| `output_writer` | Writes the `.ob`, `.ent` and `.ext` files and removes them after a failure. | none | `write_output_files` | 307 + 36 |

Total: 5,886 lines in 16 source files and 16 headers.

## Proposed refactoring steps

Each step is one sitting, changes no behaviour (except where noted), and
ends with a clean build and `make test`. MUST items come first.

1. **Q-M01.** Correct the sentence in `memory_image.h:9-10` and record the
   reading of the fixed-size-array rule in `CLAUDE.md` or
   `docs/PROGRESS.md`. Trivial.
2. **Q-M02.** Replace the Hebrew file names and the local path in
   `docs/PROGRESS.md:193-195` with an English description; fix the "parsed
   once" sentence at line 49 in the same edit. Trivial.
3. **Q-M03.** Rewrite or delete `CHANGELOG.md`; decide on `docs/audit/` and
   remove `.gitignore:23`. (The README is rewritten in Phase 5.) Small.
4. **Q-S11, Q-S07, Q-S08.** Remove the three unused public functions; move
   `STRINGIFY`, `ARRAY_LENGTH` and the file modes to `common.h`; put the own
   header first in fourteen `.c` files. One mechanical step. Trivial.
5. **Q-S12.** Give the errors module one print path and make its header
   comment true. Trivial.
6. **Q-S02.** Add lookups by text and length to `instruction_table`, build
   the string lookups on them, and remove the three heap copies from
   `syntax.c`. Small.
7. **Q-S01.** Introduce one array-growing helper in `util` with an overflow
   check and use it in the five places. Medium.
8. **Q-S04.** Move the removal of the output files into `output_writer` as
   one exported function. Small.
9. **Q-S03.** Extract the shared line loop of the two passes. Small.
10. **Q-S05.** Replace the directive switch by a dispatch table in
    `syntax.c` and add `statement_stores_data`. Small.
11. **Q-S06, Q-S17.** Move the index-word offset into the syntax module and
    remove the `ADDRESSING_IMMEDIATE` pun from the first-word encoding.
    Trivial.
12. **Q-S09, Q-S10.** Make `macro_table_add` report only "out of memory";
    split `visit_entry` into a counting and a writing function. Trivial.
13. **Q-S13, Q-S14.** One pass over all message texts for uniform quoting
    and wording, with every format a literal at the call of `errors_report`.
    The text changes; the tests do not compare it. Small.
14. **Q-S15.** Check `ferror` on the input files and report a read error.
    Behaviour changes only when a read fails; needs the owner's agreement.
    Small.
15. **Q-S16.** Split `syntax.c` into `statement` (model, accessors, layout,
    builders) and `syntax` (parser). Do this last, after steps 6 and 10 have
    made the parser smaller. Medium.
16. **NICE items** (Q-N01 to Q-N12), only if the owner wants them: naming
    prefixes in `instruction_table`, `ARE_SHIFT`, the `register_` parameter,
    the reserved name patterns.
