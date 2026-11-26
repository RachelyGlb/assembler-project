# Correctness review of the rebuilt assembler

Reviewer role: correctness, read-only, independent. Date: 2026-11-26.
Repository: `/home/user/assembler-project`. No file in the repository was
changed. All scratch work is under
`<scratchpad>/rv/`.

## Verdict

**FAIL.** Two DEFECT findings. No crash, hang, sanitizer report, leak or
valgrind error was found. Both defects sit on input that is outside ordinary
text files (a NUL byte in the source, a directory in place of the source
file); on every ordinary text input that was tried, the program agreed with
SPEC.

## 1. Developers' claims, confirmed

| Claim | Result |
|---|---|
| `make clean && make` has zero warnings | Confirmed: 16 compilations and the link with `gcc -Wall -ansi -pedantic`, no warning line. A scratch build with `-Wall -Wextra -ansi -pedantic -fsanitize=address,undefined` was also silent. |
| `make test` | Confirmed: `passed 175, failed 0, total 175`. |
| valgrind is clean | Confirmed on what was run here: 250 fuzz inputs, one run over all 185 suite inputs given twice in a single command line with a missing file between them, and four runs with blocked output files; 0 errors, 0 leaks. |

## 2. Findings

### D-1 DEFECT: a NUL byte ends the line for the whole program

Rules: SYN-2 / SYN-2b / ERR-10 (a line longer than 80 characters must be
reported), MAC-29 (a line is copied to `.am` "exactly as it is, character for
character"), ERR-27 and the other syntax errors (text after a statement must
be reported).

The line reader stores the bytes of a line correctly, but every consumer
treats the buffer as a C string, so everything from the first NUL byte to the
end of the line is invisible: it is not counted, not copied and not parsed.

Minimal input A (one line of 82 bytes: `;`, NUL, 80 times `x`, then LF):

```
printf ';\0%s\n' "$(printf 'x%.0s' $(seq 1 80))" > t.as ; ./assembler t
```

Actual: nothing printed, exit status 0, `t.am` holds `;\n`, `t.ob` is
`a<TAB>a`. Expected: `t.as:1: error: ...` (ERR-10), no `.am`, status 1.

Minimal input B (bytes `stop`, NUL, `,,`, LF):

```
printf 'stop\0,,\n' > t.as ; ./assembler t
```

Actual: nothing printed, exit status 0, `t.am` holds `stop\n` (differs from
the source although there is no macro), `t.ob` holds the `stop` word.
Expected: the line is not a legal statement, so one error on line 1 and no
`.ob`; and if the file were accepted, `.am` would have to hold the 8 bytes
of the source line.

Further observed consequences of the same cause: `st\0op` is reported as the
unknown operation `st`; `.string "a\0b"` is reported as a missing closing
quotation mark (it is an error under ERR-32 either way, so only the wording
differs); `mov r1\0, r2` is reported as too few operands.

Responsible module: `preassembler.c`, the length check at line 465
(`strlen(line) > MAX_SOURCE_LINE_LENGTH`) and the functions that write lines
to the `.am` file with string functions; `util.c`, `util_read_line`, which
returns no length, so no caller can know that the line continues after a
NUL. The passes read the `.am` file back from disk (`assembler.c`, line 161),
so they never see the lost bytes at all.

Note on the class: SPEC never mentions the NUL byte. It is classed as a
DEFECT because SYN-2 and MAC-29 are stated for every line and every
character without exception, and because the effect is the silent acceptance
of a file with content the assembler never examined. If the owner prefers to
treat a NUL byte as outside the language, the natural rule is "a NUL byte in
a line is an error", which the program does not implement either.

107 of the 110 disagreements of the byte-level differential fuzz run (section
3.2) are this finding.

### D-2 DEFECT: a read error is taken for the end of the file; a directory named `<name>.as` "assembles"

Rules: CLI-6 and ERR-1 (a source file that cannot be opened must be
reported), CLI-9 (status 0 only if every file was assembled without errors),
CLI-11.

Minimal input:

```
mkdir d.as ; ./assembler d ; echo $?
```

Actual: nothing printed, exit status 0, and the files `d.am` (empty) and
`d.ob` (`a<TAB>a`) are created. Expected: `d.as: error: <text>` and status 1,
with no output files.

Cause: on Linux `fopen` succeeds on a directory and the first `getc` fails
with `EISDIR`. `util_read_line` (`util.c`, lines 117 to 147) returns
`READ_END` for any `EOF` from `getc`, and no caller calls `ferror` on the
source or on the `.am` file that the passes read back (`grep ferror` finds it
only for the written files, `output_writer.c` line 73 and `preassembler.c`
line 516). The same code path means that a genuine I/O error in the middle
of a source file would silently assemble the part that was read and exit
with status 0.

Responsible module: `util.c`, `util_read_line` (no distinction between end of
file and read error) and `assembler.c` (the function that opens
`job->source`, line 197, and the callers of the pre-assembler and the
passes, which do not check the stream afterwards).

### ROBUSTNESS

None. See section 3.3 for the counts.

### SPEC GAP items (the program made a choice; not defects)

- **G-1 A NUL byte in the source.** SPEC is silent on whether it is legal;
  see D-1 for what the program does.
- **G-2 A macro name as the operand of `.extern`.** `mcro m` / `mcroend` /
  `.extern m` gives a first-pass error on the `.extern` line ("the symbol 'm'
  has the name of a macro"). LBL-21 and ERR-14 speak only of a label
  definition; LBL-8 says "symbol". One reference model accepts the file.
- **G-3 An operand that cannot be a label name.** `jmp data`, `jmp mov`,
  `jmp 1abc`, `jmp a_b`, `jmp <31 characters>`, and the same names in front
  of matrix brackets, are first-pass errors. Section 5.3 has no ERR ID for
  "the operand is a reserved word, ill-formed or over-long" (ERR-11 to
  ERR-13 are about label definitions; ERR-34 covers only `.entry` and
  `.extern`). The alternative reading is an undefined symbol in the second
  pass (ERR-50); the stage decides which other messages appear in the run.
- **G-4 A warning and a second-pass error on one line (Q-34).** `x: .entry x`
  prints the warning and later the error. Open question, not a defect.
- **G-5 `mcroend` with no open definition (Q-36).** A plain `mcroend` line is
  left for the first pass (unknown operation); `L: mcroend` is a
  pre-assembler error (ERR-9). Open question, not a defect.
- **G-6 A missing `mcroend` (MAC-7, undefined input).** `mcro m` / ` stop`
  with no closing line assembles silently: the body is swallowed, `.am` is
  empty, status 0.
- **G-7 An output file that cannot be created.** With a non-empty directory
  in the place of `<name>.am`, `.ob`, `.ent` or `.ext`, the program prints
  `<name>.as: error: cannot create ...`, exits with status 1, leaves no
  partial `.ob`/`.ent`/`.ext` of this run and goes on to the next file. SPEC
  has no rule for it (OUT-14 only says the old files are removed). An empty
  directory of that name is removed and replaced by the file.
- **G-8 Numbers beyond the machine word.** `.mat [99999999999][2]`,
  `.mat [65536][65536]`, `.mat [18446744073709551617][1]` are reported as
  memory overflow (ERR-41), also when values follow that would otherwise be
  "too many values" (ERR-40). One message either way.
- **G-9 `--`, `-h`, the empty argument.** Each is taken as a file name
  (`--.as`, `-h.as`, `.as`) and reported as ERR-1. Consistent with CLI-8.
- **G-10 Scale.** 60,000 `.extern` lines take about 9 seconds and 30,000
  macro definitions about 14 seconds (linear search in a linked list, so
  quadratic time). ALG-12 asks for structures that are "efficient and
  economical" without a measurable bound. Not a hang; one million comment
  lines, 300,000 macro calls and a 30 MB line each finish in under 0.2
  seconds.

### TEST GAP items (correct behaviour that no test pins down)

- **T-1 NUL bytes and read errors** have no test (D-1, D-2).
- **T-2 LBL-23 on an ERR-38 line.** `A:.data 1` followed by `A: stop` gives
  two errors (the label of the first line is defined). The suite has ERR-38
  lines but never a later reuse of their labels; one reference model got this
  wrong, which shows how easy it is to miss.
- **T-3 Integer wrap-around.** `.data 4294967297`, `.data
  18446744073709551617`, `prn #4294967296` and `prn #18446744073709551616`
  (values that wrap to a legal number in 32 or 64 bits) are correctly
  rejected (ERR-39). The suite only has `99999999999999999999`, which does
  not wrap to a value in range.
- **T-4 Huge `.mat` dimensions** (product that overflows 32 or 64 bits) are
  correctly reported; no test.
- **T-5 `.string "a" "b"`** is one legal string of 5 characters (SYN-22,
  "the string ends at the last quotation mark"); no test with two quoted
  parts separated by a space.
- **T-6 Allocation failure (ERR-R10)** can be provoked with `ulimit -v`: the
  program printed `<name>.as: error: out of memory; the assembler stops`,
  exited with status 1 and did not process the next argument. No test.
- **T-7 The same argument twice, an argument with a directory part**
  (`sub/ok`, messages start with `sub/e1.as:1:`), and a 250-character file
  name work; no test.
- **T-8 Macro body line that is a `.mat` overflow called three times, a
  warning line called three times, first-pass and second-pass errors in a
  body called twice between other errors**: all correct (ERR-R12, ERR-R17,
  ALG-17); the suite has these singly, the combinations were probed by hand.

## 3. What was run

### 3.1 Differential run, generated programs (step 2)

Generator: `rv/gen.py`; driver: `rv/diff.py`. Reference: `ref/ref.py` (same
command line as the real binary, byte-based), with `refasm.py` read for
adjudication. Compared per program: exit status, every diagnostic as (file,
line, severity) in order, `.am` and `.ob` byte for byte, `.ent` and `.ext` as
sets of lines, and the set of files present.

| Set | Programs | Agree | Disagree |
|---|---|---|---|
| Valid generator (all 16 operations, all legal mode pairs, `.data`, `.string`, `.mat`, entries, externals, warnings, up to 3 macros called 0 to 3 times, odd whitespace including VT, FF and CR in a third of the programs, CR LF endings in one of eleven, no final newline in one of seven) | 3,000 (2,762 assembled, 238 rejected by both sides, mostly lines over 80 characters) | 3,000 | 0 |
| One or two mutations of such a program (22 mutators: deleted, doubled, trailing comma; illegal mode; bad and out-of-range numbers; duplicate, reserved, ill-formed, over-long label; undefined symbol; bad string; bad matrix syntax; 25 macro faults; over-long line; deleted or inserted character, token, whitespace; `.entry`/`.extern` conflicts; 70 bad statements; duplicated, deleted, swapped lines; memory overflow) | 6,000 (1,203 still valid, 4,797 with errors) | 5,841 | 159 |

The 159 disagreements by cause:

- 149: the mutation produced a `mcroend` line with no open definition (a
  deleted or swapped `mcro` line). The reference refuses such input
  ("Q-36 input"). Open question Q-36; the real binary neither crashed nor
  misbehaved. Not a defect.
- 10: `.mat [99999999999][2]`. The reference crashed with `MemoryError`; the
  real binary reports ERR-41. Fault of the reference (G-8).

### 3.2 Differential run, byte-level mutations of the suite inputs

Driver: `rv/dfuzz.py`; 4,000 inputs made from the 185 `.as` files of the
suite by bit flips, byte replacement, truncation, duplicated and swapped
lines, inserted tokens (including NUL, 0xFF, 0x80, CR). 138 were skipped as
Q-36 input, 3,752 agreed, 110 disagreed:

- 107: NUL byte in the input (D-1).
- 2: LBL-23 on an ERR-38 line (`A:.mat [` and `LOOP:]   inc r2`); the real
  binary defines the label and reports the later duplicate, the reference
  does not. The real binary follows LBL-23; fault of the reference (T-2).
- 1: `.extern fill` where `fill` is a macro (G-2).

### 3.3 Robustness (step 4)

- Mutation fuzzer `rv/fuzz.py` on the sanitizer build
  (`-fsanitize=address,undefined -fno-sanitize-recover=undefined`): 6,000
  iterations, 5 second timeout each. Crashes 0, hangs 0, sanitizer reports
  0, exit statuses other than 0 and 1: 0, output on `stderr`: 0. Each run was
  also checked for consistency: every `stdout` line has the form of ERR-R13
  or ERR-R14, status 0 goes with `.am` and `.ob` and no error line, status 1
  goes with at least one error line and no `.ob`, `.ent` or `.ext`. No
  violation.
- The same fuzzer under `valgrind --leak-check=full
  --errors-for-leak-kinds=all` on the real binary: 250 iterations, 0 errors,
  0 leaks.
- All 185 suite inputs twice in one command line with a missing file in the
  middle (371 arguments): sanitizer build silent, valgrind silent.
- Error paths: output file blocked by a directory (four variants), directory
  as source, unterminated macro, stray `mcroend`: sanitizer build and
  valgrind silent.
- Allocation failure provoked with `ulimit -v` in the line reader, the macro
  table and the symbol table: clean message, status 1, no crash.
- Scale: see G-10.

### 3.4 Hand probes that turned out correct (step 3), about 330 lines of input

- Line length: 80 and 81 characters; 80 + CR LF; 80 + CR CR LF (error); 81 +
  CR LF (error); 80 + CR at the end of a file without newline (error, the CR
  counts, SYN-25); over-long last line without newline; over-long line after
  a faulty `mcro` line is silent (MAC-38).
- Numbers: -129/-128/127/128 immediate, -513/-512/511/512 data and matrix
  values, leading zeros, `+0`, `-0`, lone sign, `1-`, `0x1`, `1.5`, `1e3`,
  sign separated from digits, 20-digit and wrap-around values.
- Memory: exactly 156 words and 157 words (data only, data then code, code
  only), overflow on a macro body line reported once with the body line
  number, the overflow line when zero-word lines follow.
- Operands: 17 malformed matrix operands, whitespace inside and outside
  brackets, 23 comma and operand-count variants, 23 immediate variants.
- Directives: 26 `.data` variants, 36 `.mat` variants (whitespace of every
  kind inside the brackets, `][` adjacency, signs, zero, leading zeros,
  missing parts), 9 `.string` variants, `.entry`/`.extern` operands that are
  reserved, over-long or ill-formed.
- Labels: empty label, `L1: L2: stop`, label alone, label followed by `;`,
  label equal to a macro name before and after the definition.
- Warnings: `x:y: .extern Z`, `:: .extern Z`, `: .entry Q`, warning line in a
  macro body called three times.
- Macros: 24 cases around MAC-30 to MAC-38 (duplicate, reserved, extra text,
  label in front of `mcro`, `mcroend` and a call, a name whose definition
  was faulty, unexpanded call in a body, VT/FF/CR around the keywords).
- Message order (ERR-R17) and count (ERR-R16) with a macro called twice in
  the first pass and in the second pass, together with a warning.
- Several files: seven arguments mixing a valid file, a first-pass error, a
  missing file, a pre-assembler error, a second-pass error and a warning,
  with stale `.am`, `.ob`, `.ent`, `.ext` for all six names: messages in
  command-line order, status 1, no stale file left (OUT-14), exactly the
  expected files present.
- Non-ASCII bytes 0x80, 0x85, 0xA0, 0xFF outside strings: reported as
  ordinary illegal text, never treated as whitespace or as end of file.

### 3.5 Not tested

- An unreadable source file or an unwritable directory: the session runs as
  root, so permission bits have no effect. Blocked outputs were simulated
  with directories instead (G-7).
- A write error in the middle of an output file (disk full).
- The inputs of Q-35 and Q-37 beyond what the mutation run happened to
  produce.

## 4. Files of this review

All under `<scratchpad>/rv/`:
`gen.py` (generator and mutators), `diff.py` (differential driver),
`dfuzz.py` (byte-level differential fuzz), `fuzz.py` (robustness fuzz),
`t.sh` (single probe against binary and reference), `asan/asm_asan`
(sanitizer build), `dis_invalid/` and `dis_fuzz/` (every disagreeing input
with both results), `*.log` (the counts quoted above).
