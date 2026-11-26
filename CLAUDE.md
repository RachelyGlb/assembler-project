# CLAUDE.md

This repository holds a two-pass assembler written in C90 for an imaginary
10-bit machine. It began as the final project (Assignment 14) of course 20465,
"Systems Programming Laboratory", at the Open University of Israel, and is
being rebuilt as a public portfolio piece: correct against the spec, modular,
clean, and shipped with a public demo.

Read this file, `docs/SPEC.md` and `docs/PROGRESS.md` before doing anything.

## Sources of truth

In priority order:

1. The course forum Q&A with moderator rulings (`docs/forum.txt`, extracted
   from `docs/Task_14.docx`). It overrides the booklet.
2. The official corrections sheet (`docs/corrections.pdf`).
3. The course booklet (`docs/booklet.pdf`). Printed pages 17 to 46; PDF page N
   is printed page N+16. Always cite printed page numbers.

`docs/SPEC.md` is the verified digest of these sources. Every rule there has a
stable ID and a source. Code, tests and audits cite SPEC rule IDs.

If the sources conflict or something is undefined: search the forum text. If
the forum has no answer, ask the project owner. Do not guess.

The course materials are not ours to publish. They are listed in `.gitignore`
and must never be committed. The code must be original: do not read or use
other people's solutions to this assignment.

## Build and test

```
make          # build the `assembler` binary with gcc -Wall -ansi -pedantic
make test     # build, run the real binary on tests/, diff against expected files
make clean    # remove objects, the binary and generated outputs
```

Usage: `./assembler x y hello` processes `x.as`, `y.as` and `hello.as`
independently (names are given without the extension).

The build must produce zero warnings. See `docs/PROGRESS.md` for the current
state of the build and of the test suite.

## Coding rules (binding)

- One module per responsibility, each a `.c`/`.h` pair: pre-assembler, macro
  table, first pass, second pass, line parsing (syntax), symbol table, memory
  image (code and data), base-4 encoding, output-file writing, error
  reporting, instruction table, shared definitions. The split may be adapted
  to the existing code, with a recorded reason.
- Hide implementations: define each data structure's struct in its `.c` file.
  The `.h` exposes only an opaque type and access functions. Code that uses
  the symbol table must not know whether it is an array or a linked list.
- One constant instruction table (name, opcode, operand count, legal source
  modes, legal destination modes). All validation and encoding derive from
  it. No repeated `if`/`strcmp` chains.
- No global variables (const tables are allowed). State is passed as
  parameters.
- Functions private to a module are `static`. One function does one thing and
  is about 40 lines at most.
- No magic numbers: a `#define` or an `enum` for every constant.
- All error messages go through one function in the error module.
- Fixed-size arrays are allowed only for the machine-code image. The symbol
  table and the macro table are dynamically allocated. Check every `malloc`;
  free everything on every path, including error paths and between input
  files.
- Documentation: a header comment in every file (the module's role) and above
  every function (what it does, parameters, return value), plus comments on
  non-obvious logic.
- C90: `/* */` comments only, declarations at the top of a block, include
  guards.
- `gcc -Wall -ansi -pedantic` with zero warnings. The Makefile has correct
  header dependencies and the targets `all`, `test` and `clean`.

## Working method

One orchestrator session plans, delegates, verifies and reports to the owner.
The orchestrator does not write or edit project code; subagents do. The
orchestrator keeps its own context small by reading reports, not whole files.
Subagents run on the Fable 5.2 model.

Persistent memory lives in files, not in the conversation:

- `CLAUDE.md`: the rules, the commands and this working method.
- `docs/SPEC.md`: the verified spec, each rule with its source.
- `docs/PROGRESS.md`: current phase, what is done, what is open, decisions
  made, known issues.

Subagent roles:

- **spec-analyst**: reads the booklet and the forum text, writes
  `docs/SPEC.md`. Touches no code.
- **test-engineer**: writes test inputs, expected outputs and the test
  runner. Touches no assembler code.
- **auditor**: read-only. Compares the code to `docs/SPEC.md` and to the
  coding rules, and reports findings with file and line.
- **fixer**: fixes exactly one bug per invocation.
- **refactorer**: refactors exactly one module or one concern per
  invocation, with no behavior change.
- **reviewer**: read-only and independent; has not seen the work being done.
  Checks a finished change against SPEC and the rules, runs the build and the
  tests, and reports pass or fail with evidence.
- **demo-builder**: builds everything under `demo/`. Touches no assembler
  code.

Rules for delegation:

- A subagent starts with no context. Every brief states its role, the single
  task, the files to read first (always `CLAUDE.md` and `docs/SPEC.md`), the
  files it may change, what "done" means, and the report format.
- One topic per subagent. A new topic gets a new subagent with a fresh
  context. A subagent is never reused for a second topic.
- Subagents that change files run one at a time. Only read-only subagents may
  run in parallel.
- Reports are at most 15 lines: what was changed (files), what was run and
  its actual output, what is still open.
- Reports are not trusted. After every change the orchestrator runs the build
  and `make test` itself and reads the output. Anything non-trivial gets a
  reviewer.
- After each completed topic: update `docs/PROGRESS.md` and commit.
- At the end of every phase: update `docs/PROGRESS.md` so that a brand-new
  session could continue from it alone, then tell the owner the phase is
  finished.

## Phases

0. **Understand.** Write `docs/SPEC.md`, `CLAUDE.md`, `docs/PROGRESS.md`.
   Stop for the owner.
1. **Tests before fixes.** `tests/valid` and `tests/errors` with expected
   outputs, plus `make test`. Tests are expected to fail at this point.
2. **Diagnose.** Bug list and cleanliness list, with an ordered plan. Stop
   and wait for approval. No project code is written before this approval.
3. **Correctness.** One bug at a time, one fixer each, a clean compile,
   `make test` and a separate commit after each. No whole-module rewrite
   without asking the owner.
4. **Clean-up.** Only when all tests pass. One refactorer per module or
   concern, `make test` green after each step, valgrind, then a full review.
   Stop for approval.
5. **Demo.** A separate `demo/` directory, GitHub Actions, GitHub Pages and
   the README. The assembler's code and interface do not change.

## General rules

- Never say something works without running it and showing the output.
- Before every push, check that the repository contains no personal files, no
  course materials and no identifying details beyond the owner's name.
- Everything in the repository is written in English, in full sentences:
  code, comments, commit messages, these documents, the demo page and briefs
  to subagents. Everything addressed to the owner is written in Hebrew, with
  identifiers, file names, commands and error messages kept in English.
- Terse style is used only for subagent reports and routine status chatter.
  Terse never means dropping facts.
