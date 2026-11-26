# PROGRESS

A new session starts with: "Read CLAUDE.md, docs/SPEC.md and docs/PROGRESS.md,
then continue with the next phase."

## Current state

- **Phase 0 (Understand), Phase 1 (Tests), Phase 2 (Diagnose): finished
  2026-11-26.** The owner approved the Phase 2 plan, the answers to the open
  questions and the publication items on 2026-11-26.
- **Phase 3 (Correctness): finished 2026-11-26.** `make` has zero warnings,
  `make test` prints `passed 175, failed 0, total 175`, and valgrind reports
  no errors and no leaks on valid, failing and mixed runs (checked by the
  orchestrator on `ps nosuch example1 undefined_symbol one_message_per_line`).
- **Phase 4 (Clean-up): in progress.** Two independent reviews of the rebuilt
  code are the starting point: correctness beyond the suite, and code quality
  against the coding rules. Each finding gets its own fixer or refactorer.
  Phase 4 ends with a module table and a STOP for the owner's approval.
- Working branch: `claude/project-thread-l0vj7x`.

## Phase 3: what was done

The approved plan had stage A (fixes in place: errors, symbol table, output
writer) followed by stage B (three rewrites). Because the approved target
structure (`docs/audit/STRUCTURE.md`) changes the public interface of every
module, the work was regrouped by pipeline stage so that nothing was fixed
twice; the content is the same. The orchestrator asked the owner with a
decision card and went ahead with the recommended option without waiting for
a tap; this departure is to be confirmed with the owner at the Phase 4 stop.

1. `1001b08` BUG-01: header renamed to `assembler.h`, Makefile reduced to
   `all`, `test`, `clean` with generated header dependencies.
2. `7a31dff` Dead code, `tests/sh`, CR LF endings and Hebrew comments removed;
   behaviour proven unchanged by a byte comparison of all test outputs.
3. `adea770` Stage 1: `common.h`, `errors`, `util`, `instruction_table`,
   `macro_table`, `line_map`, `preassembler` and a new driver `assembler.c`.
4. `a58f002` Stage 2: `syntax`, `symbol_table`, `memory_image`, `encoder`,
   `base4`, `reference_list`, `first_pass`, `second_pass`, `output_writer`;
   all legacy files deleted.

The test suite was extended to 175 cases (`696cdd7`) after the owner's
answers to Q-26 to Q-33 were recorded in SPEC (`cceacd4`).

Conventions of the new code that every later change must keep: a stage
returns a `StageResult` (`STAGE_OK`, `STAGE_ERRORS`, `STAGE_FATAL`); an
allocation failure is returned up to the driver as `STAGE_FATAL` and never
handled by calling `exit` inside a module; every diagnostic goes through
`errors_report` or `errors_warn` with the line translated by
`line_map_source_line`; each pass parses a line with the one parser, `statement_parse`, and a
syntax error yields a statement for which `statement_is_valid` is false.

Readings the code makes where SPEC section 6.1 is still open (to be confirmed
by the owner): Q-34 a label before `.entry` whose operand is undefined gives
the warning in the first pass and the error in the second pass; Q-35 an
over-long `mcro`, `mcroend` or call line gives one line-length error and
still opens or closes the definition; Q-36 a plain `mcroend` with no open
definition is copied and rejected by the first pass; Q-37 a line with another
error adds no words, so it never also reports memory overflow.

## Phase 1 result

- `docs/SPEC.md` (1,822 lines) now carries the owner's decisions of
  2026-11-26 as rules tagged "[owner decision, 2026-11-26]". The section
  "Decisions waiting for the owner" further down in this file is history:
  all of those recommendations were approved, with one change (Q-4 uses a
  TAB).
- Test suite: 124 cases (`tests/valid` 56, `tests/errors` 59, `tests/cli` 9),
  runner `tests/run_tests.sh` (POSIX sh; binary chosen with the `ASSEMBLER`
  environment variable; a case name as argument runs one case; scratch output
  and `diff.txt` under `tests/out/`). `tests/README.md` maps cases to SPEC
  rules. `make test` is wired.
- An independent reviewer rebuilt every expected output with a separate
  reference derived from SPEC only: 56 of 56 valid cases, 59 of 59 error
  cases and 9 of 9 cli cases match. The runner was checked against 25 fake
  binaries (do nothing, crash, hang, wrong output) and reports none as PASS.
- Current code against the suite, run by the orchestrator on a scratch build
  (sources copied, a lowercase copy of the header added,
  `gcc -Wall -ansi -pedantic *.c`): `passed 1, failed 123, total 124`. In the
  repository itself `make test` stops with `No rule to make target
  'assembler.h'`.
- Known small gaps in the suite: no case for SYN-8 whitespace other than
  space and TAB, for a macro name appearing as one word among others on a
  statement line (MAC-3), or for an indented label; the valid group ignores
  `warning` lines; without the `timeout` command a hang is not caught.

## Phase 2 result

Full reports, written by two read-only auditors:

- `docs/audit/BUGS.md`: 46 bugs (BUG-01 to BUG-46), each with file and line,
  the SPEC rules violated, a minimal input with the real output, and the
  tests it fails; then a table of all 124 tests against bug IDs, the
  behaviour that must not regress, and valgrind results.
- `docs/audit/STRUCTURE.md`: 111 findings against the coding rules, the
  proposed target modules with public functions, a mapping of every existing
  file to its destination, and an order of refactoring steps.

Claims reproduced by the orchestrator: `A: .data 6, -9, 15` produces one data
word; a line with leading whitespace before a label aborts with
`free(): invalid pointer`; assembling two files that both define `MAIN`
reports `Duplicate label: MAIN` for the second.

Headline facts: after discounting the format differences only 3 of 56 valid
cases have correct machine words. 27 of the 46 bugs sit in one cluster
(`first_pass.c`, `line_analysis.c`, `instruction_validation.c`,
`memory_builder.c` and the operand code in `second_pass.c`), where the same
line is tokenised three incompatible ways and there is no real parser. Both
auditors independently recommend rewriting that cluster and the body of
`preassembler()` instead of fixing them bug by bug. `CLAUDE.md` forbids
rewriting a whole module without the owner's approval.

### Plan proposed at the end of Phase 2 (approved, then regrouped; see Phase 3)

Stage A, foundations, one fixer each, a commit after each:

1. BUG-01: rename the header include and fix the Makefile; reduce the
   Makefile to `all`, `test`, `clean` with correct header dependencies.
2. Remove dead code, undefined prototypes and stray files (`This`,
   `instruction_count`, `tests/sh`, `formal_tester.*`).
3. One error module: every diagnostic goes through one function, in the SPEC
   format with the file name, on stdout; the passes return a status; the exit
   status follows CLI-9 (BUG-05, BUG-06).
4. Symbol table as a handle created and freed per input file (BUG-08,
   BUG-13).
5. Output writer: TAB separators and the use-after-free (BUG-02, BUG-11,
   BUG-27).

Stage B, the three rewrites that need the owner's approval:

6. Instruction table completed as one `static const` table with legal modes,
   plus one `syntax` module that parses a line into a statement structure
   (label, kind, operands) and validates it against the table.
7. First pass rebuilt on that parser, absorbing `memory_builder.c`,
   `line_analysis.c` and `instruction_validation.c`; second pass reduced to
   resolving symbols and entries (BUG-15 to BUG-26, BUG-28 to BUG-43).
8. Pre-assembler rebuilt around a separate macro-table module, keeping blank
   and comment lines, producing the line map and leaving label checks to the
   first pass (BUG-03, BUG-04, BUG-07, BUG-09, BUG-10, BUG-12, BUG-44 to
   BUG-46).

Stage C: whatever still fails, one bug per fixer (BUG-14 allocation
failures, BUG-30 memory bound, and anything the earlier steps unmask), until
`make test` is green and valgrind is clean. Then Phase 4.

### Questions put to the owner at the end of Phase 2 (all approved as recommended)

- Approval of the plan, in particular the three rewrites in stage B.
- SPEC section 6.1, source-silent details, with recommended answers:
  Q-26 print nothing on success, a usage line when there are no arguments;
  Q-27 print a warning for a label before `.entry`/`.extern` and otherwise
  ignore the label; Q-28 the pre-assembler reports a label before `mcro`,
  `mcroend` or a call; Q-29 exactly one message per line, in line order
  within each stage; Q-30 memory overflow reported once, on the line that
  crosses the limit, and a label defined before its `.extern` reported on the
  `.extern` line; Q-31 output files left from an earlier run are removed when
  the current run does not produce them; Q-32 a missing final newline and CR
  LF endings are accepted, and a program with no statements yields an `.ob`
  holding only `a<TAB>a`; Q-33 a file that cannot be opened does not stop the
  remaining files.
- A gap the reviewer found: after a faulty `mcro` line, the lines up to
  `mcroend` are still treated as the macro body and produce no further
  messages (ten tests assume this; SPEC is silent). Recommended: yes.
- Publication hygiene: `README.md` contains an e-mail address; `CLAUDE.md`
  names the course number and links to the two friends' repositories; the git
  history has Hebrew commit messages; `formal_tester.as/.ob` look like a
  booklet example and the `.ob` was produced by the buggy program.

## Done in Phase 0

- `docs/SPEC.md` written by a spec-analyst subagent from the booklet (all 30
  pages), the official corrections sheet and the whole forum Q&A. Every rule
  has a stable ID and a source. It contains the instruction table (section 3),
  both booklet examples with the corrections applied and their full expected
  `.ob`, `.ent` and `.ext` content (section 4), the error categories
  (section 5) and the open questions (section 6).
- Orchestrator checks on SPEC.md: no forum participant's name appears in it;
  the line-number ruling was read in the forum text itself; the header, the
  addresses and several words of the `ps.ob` example were re-derived by hand
  and match (22 = `bbc`, 15 = `dd`, 100 = `bcba`, `mov M1[r2][r7],W` =
  `aacba`, 133 relocatable = `cabbc`, -9 = `dddbd`).
- `CLAUDE.md` written: coding rules, commands, working method, phases.
- `.gitignore`: the course materials are ignored (`docs/booklet.pdf`,
  `docs/booklet.txt`, `docs/corrections.pdf`, `docs/corrections.txt`,
  `docs/Task_14.pdf`, `docs/Task_14.docx`, `docs/forum.txt`). The old rule
  `/*/*`, which ignored every file in every subdirectory, was removed.

## Where the course materials come from

They are not in the repository. The owner keeps three PDF files in the
project's shared files: the assignment booklet, the official corrections
sheet (both with Hebrew file names) and the forum Q&A (`Task 14.pdf`). In a
new session, copy them into `docs/` as `booklet.pdf`, `corrections.pdf` and
`Task_14.pdf`, then extract the text:

```
pdftotext -layout docs/booklet.pdf docs/booklet.txt
pdftotext -layout docs/corrections.pdf docs/corrections.txt
pdftotext -layout docs/Task_14.pdf docs/forum.txt
```

Notes: the booklet PDF has 30 pages, printed 17 to 46, so PDF page N is printed
page N+16. `docs/booklet.txt` drops the Hebrew letter nun and garbles tables;
read the PDF for anything exact. The forum is a PDF in English (not a `.docx`)
and its text contains the set of threads twice; the copies agree.

## Key findings

- The line number in an error message may refer to the `.as` file or to the
  `.am` file; the moderator accepts both (SPEC section 2.14). The project must
  pick one (decision C-1).
- Three words of the printed worked examples change under the corrections:
  address 109 in both examples (`prn #-5`: `daaaa` becomes `dbaaa`) and
  address 120 in the second (`L3`: `aaaac` becomes `aaaab`). Correction 4
  changes no word. The `prn` word is implied by correction 1 and stated by a
  student; the moderator never confirmed it (Q-24).
- Six of the seven forum rulings the owner reported from memory were found.
  "A label in front of `.entry` or `.extern` is ignored" is not a forum ruling
  but is stated by the booklet (p.27, p.28). The matrix ruling has a
  refinement: whitespace inside the brackets is allowed.
- Rulings that go beyond the owner's checklist are listed in SPEC and include:
  a comment needs `;` as the very first character; whitespace is mandatory
  after a label's colon; a number that overflows its word is an error; string
  characters are limited to 32..126; `.mat` with too many values is an error;
  a symbol both defined locally and declared `.extern` is an error; `mcro` and
  `mcroend` are reserved; macro names follow label rules plus `_`; a program
  that exceeds memory is an error.

## Decisions waiting for the owner

Choices the forum leaves to the implementer (SPEC section 2.15). The
orchestrator's recommendation is given for each; none is decided yet.

- C-1 Line number base for pass errors. Recommended: the `.as` line.
- C-2 Stage that checks the 80-character limit. Recommended: pre-assembler.
- C-3 Empty and comment lines in `.am`. Recommended: keep them, which also
  keeps `.as` and `.am` line numbers aligned outside macros.
- C-4 Extra text after a macro call. Recommended: error in the pre-assembler.
- C-5 Extra text on `mcroend`. Recommended: report it and close the macro.

Open questions that no source settles (SPEC section 6.1), with recommended
readings:

- Q-3 File name in error messages. Recommended: `file.as:line: error: text`.
- Q-4 Separator in `.ob`/`.ent`/`.ext`. Recommended: one TAB, no indent
  (changed after the second look: both reference programs use a TAB).
- Q-6 Numeric bounds. Recommended: signed only (-128..127 immediate,
  -512..511 data).
- Q-7 `.mat` dimensions. Recommended: unsigned positive integers only.
- Q-8 Repeated `.extern`/`.entry`. Recommended: accept silently.
- Q-9 Reserved words. Recommended: directive names without the dot are also
  reserved; `PSW` and `PC` are not.
- Q-10 Macro call inside a macro body. Recommended: not expanded (no nesting).
- Q-11 Macro name starting with `_` or equal to a register. Recommended: both
  are errors.
- Q-14 Label alone on a line. Recommended: error.
- Q-16 Argument with `.as` attached. Recommended: taken literally (`x.as.as`
  fails to open); exit status 0 only when every file assembled.
- Q-17 `.string ""`. Recommended: legal, one terminator word.
- Q-21 Duplicate macro names (two conflicting moderator answers).
  Recommended: report an error, which satisfies both answers.
- Q-22 Label before `mcro`, `mcroend` or a macro call. Recommended: error.
- Q-25 The moderator said not to define arrays as globals, even a const
  instruction table; `CLAUDE.md` allows const tables. Recommended: keep the
  const table `static` inside the instruction-table module, reachable only
  through access functions, which satisfies both.

## Second look at the open points (2026-11-26)

The owner doubted that the 19 points were really open. An independent auditor
re-read the whole forum text and the booklet, and a second auditor built and
ran the two reference repositories (behaviour only, no code taken). SPEC.md
section 6.1 has not yet been edited to reflect this; a spec-analyst should do
that together with the owner's decisions at the start of Phase 1.

Note on `docs/forum.txt`: its two copies are not equal. Lines 1 to 2545 stop at
the thread "Matrix Labels"; 28 threads, including most rulings from 03/06 to
25/06, exist only in lines 4639 to 5304. Always search the whole file.

Result of the source audit (forum.txt line numbers in parentheses):

- C-1 to C-5: confirmed as explicit moderator "at your discretion" answers
  (2295, 4839, 5191, 774, 5174, 1810, 1855).
- Settled by a ruling: Q-16 an argument is a name without the extension, so
  `x.as` makes the program try `x.as.as` and report an error (2258); Q-25 no
  global arrays, even a const instruction table (1765, 2452); Q-21 a duplicate
  macro name may be reported as an invalid macro name, and reporting satisfies
  all four moderator posts (5204, 4942, 4643-4655, 378); Q-11 a macro name
  must be checked against label names (405).
- Implied by the sources: Q-6 signed ranges only, -128..127 and -512..511
  (p.19, p.21, 1024); Q-8 repeated `.extern`/`.entry` accepted (p.35 step 10,
  p.36 step 5, 1933-1939); Q-9 directive names without the dot are reserved
  (p.29); Q-10 a macro call inside a macro body is not expanded (p.31, 4701);
  Q-11 a macro name starts with a letter and may not be a register name
  (5285, p.29); Q-14 a label alone on a line is an error (p.25, p.28, p.35);
  Q-22 a label before `mcro`/`mcroend`/a call is illegal input; Q-4 widths and
  a zero header printed as `a` (p.45, 5153).
- Still open after the audit: Q-3 file name in error messages; Q-4 the
  separator character only; Q-7 `.mat` dimension 0, negative or signed, and
  whitespace before the first value; Q-9 `PSW` and `PC` only; Q-16 the exit
  status only; Q-17 `.string ""`.

What the two reference programs do on the points that are still open or left
to the implementer (called Y and N here;
observed by running them):

- C-1: both report the `.am` line in the passes and the `.as` line in the
  pre-assembler. C-2: both check line length in the pre-assembler. C-3: both
  remove empty and comment lines from `.am`. C-4: Y ignores the extra text, N
  leaves the line for the first pass. C-5: Y reports an error, N does not
  detect it.
- Q-3: neither prints the file name in the message. Q-4: both separate fields
  with one TAB; Y indents the header with a TAB, N pads the header to four
  letters and adds an empty line. Q-7: both reject dimension 0 and accept
  `[2][2]1,2,3,4`; Y accepts `+2`. Q-9: Y accepts every name, N rejects the
  bare directive names and accepts `PSW` and `PC`. Q-16: Y exits 1 only for a
  file that does not open, N always exits 0. Q-17: both accept `""` and emit
  one zero word.
- Both programs have defects of their own (N crashes on `stop` at end of
  line; Y strips all whitespace in `.am`), so they are a data point about
  what was accepted, not a reference for correct behaviour.

## Known issues in the existing code (seen in passing, not yet audited)

- `make` fails on Linux: `make: *** No rule to make target 'assembler.h',
  needed by 'assembler.o'.` The header in the repository is `Assembler.h`.
- The repository root holds stray files (`This`, `instruction_count`, both
  empty) and old helper scripts under `tests/sh`.
- `.gitignore` ignores `*.am`, `*.ent`, `*.ext` and `*.expected`; Phase 1
  must adjust it so that expected test outputs can be committed.
- The `.gitignore` comments are in Hebrew; the repository is meant to be in
  English.

## Decisions made

- 2026-11-26: subagents run on Fable 5.2 (owner).
- 2026-11-26: two friends' repositories may be consulted for structure only,
  never for code or logic (owner; see `CLAUDE.md`).
