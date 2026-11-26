# PROGRESS

A new session starts with: "Read CLAUDE.md, docs/SPEC.md and docs/PROGRESS.md,
then continue with the next phase."

## Current state

- **Phase 0 (Understand): finished on 2026-11-26.** Waiting for the owner's
  answers to the decisions listed below before Phase 1 starts.
- Next: Phase 1 (tests before fixes). No assembler code has been changed, and
  none may be changed before the owner approves the Phase 2 plan.
- Working branch: `claude/project-thread-l0vj7x`.

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

They are not in the repository. In a new session, copy them from the project's
shared files into `docs/` and extract the text:

```
cp "/mnt/project-files/מטלה 14.pdf" docs/booklet.pdf
cp "/mnt/project-files/תיקונים לטעויות המופיעות בממן.pdf" docs/corrections.pdf
cp "/mnt/project-files/Task 14.pdf" docs/Task_14.pdf
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
- Q-4 Separator in `.ob`/`.ent`/`.ext`. Recommended: one space, no indent.
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
