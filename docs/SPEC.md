# SPEC: two-pass assembler for the 10-bit imaginary machine

This file is the verified digest of the course documents for Assignment 14 of
course 20465, together with the decisions of the project owner. Every rule has
a stable ID and ends with its source. Code, tests and audits cite the rule
IDs. The file is binding: where the sources allow several behaviours, the
owner's decision recorded here fixes one of them.

## 1. Sources and precedence

| Tag | Source | Notes |
|---|---|---|
| `[booklet p.NN]` | `docs/booklet.pdf`, the assignment booklet (Hebrew) | Printed pages 17 to 46. PDF page N is printed page N+16. All citations use printed page numbers. |
| `[correction N]` | `docs/corrections.pdf` / `docs/corrections.txt`, the official corrections sheet, items 1 to 4 | Overrides the booklet. |
| `[forum: <thread title>, <date>]` | `docs/forum.txt`, the course forum Q&A (English; extracted from `docs/Task_14.pdf`, 90 pages) | Highest priority. Only posts marked "Posted by moderator" are rulings; the date is the date of the moderator post (dd/mm/yyyy). Student posts are never rulings; where one is mentioned it is labelled "student post, not a ruling". Names of students are not reproduced here. |
| `[inferred]` | A conclusion drawn by the spec author | Always accompanied by its reasoning. Not an official statement. |
| `[owner decision, 2026-11-26]` | A decision of the project owner | Used only where the sources leave a choice to the implementer or are silent. It is always followed by the source that allows the chosen option, or by the statement that the sources are silent. It is binding for code and tests. |

Precedence, highest first: forum moderator rulings, corrections sheet,
booklet. A moderator ruling overrides the corrections sheet and the booklet;
every rule below that is changed by the forum says so. An owner decision never
overrides a source: it selects one of the behaviours the sources allow, or
fills a gap. If a later reading of the sources shows a conflict with an owner
decision, the conflict is reported to the owner and not resolved silently.

Notes on the forum text:

- The whole forum text was read. `docs/forum.txt` holds two extractions of
  the forum, and **the two halves differ**. Lines 1 to 2545 are a copy that is
  cut short: it stops in the thread "Matrix Labels". Lines 2546 to 5304 are
  the complete copy. Where the two overlap they agree, but several dozen
  threads, among them most of the rulings dated 03/06/2025 to 25/06/2025,
  exist only in lines 4639 to 5304. A search must therefore always cover the
  whole file. Unclear passages were checked against the PDF.
- The corrections sheet is also posted in the forum by the moderator.
  [forum: Clarifications and Corrections, 14/07/2025]
- Several moderator answers say only "an answer exists in the forum" or "at
  your discretion". The first kind adds nothing; the second kind is recorded
  in section 2.15 as a choice left to the implementer, together with the
  option the owner chose.
- When two moderator posts disagree, both are cited. The only such case is the
  duplicate macro name (MAC-15, MAC-28); it is settled by an owner decision
  that satisfies every one of the posts.
- Seven rulings were first reported by the owner from memory. Section 2.13
  records how each was verified against the forum text.

The four official corrections, translated:

1. "The correct opcode table appears on page 35. (There are wrong
   substitutions in the first table on page 20.)"
2. "Wherever the sentence 'every word is encoded in hexadecimal base' appears,
   replace it with: 'the encoding of every word in the machine code is done in
   unique base 4'."
3. "In the encoding table on page 43, row 120, there is a mistake in the
   encoding of an external symbol; it should be `00000000-01`."
4. "In the table on page 21, row 'direct register addressing', it should say:
   'it is encoded in an additional word that holds, in the four bits 6-9, the
   number of the register'."

Translations of booklet sentences in this file are by the spec author. Where
the exact wording matters, the Hebrew original is on the cited page.

The existing C source files were deliberately not read while writing this
file. No web source and no other solution was used.

---

## 2. Rules

### 2.1 Machine (M)

- **M-1** The machine has a CPU, registers and RAM. Part of the memory serves
  as a stack. [booklet p.19]
- **M-2** There are eight general registers named `r0`, `r1`, `r2`, `r3`,
  `r4`, `r5`, `r6`, `r7`. Register names are always written with a lowercase
  `r`. Each register is 10 bits wide; bit 0 is the least significant and bit 9
  the most significant. [booklet p.19]
- **M-3** There is also a program status word register, `PSW`, holding flags
  (for example the zero flag `Z`). It is not addressable from the assembly
  language. [booklet p.19, p.22]
- **M-4** Memory has 256 cells, addresses 0 to 255 (decimal). Each cell is 10
  bits and is called a "word". Bits of a word are numbered like bits of a
  register. [booklet p.19]
- **M-5** The machine works only with positive and negative integers. There is
  no support for real numbers. Arithmetic is two's complement. [booklet p.19]
- **M-6** Characters are supported and are represented in ASCII. [booklet p.19]
- **M-7** `PC` (program counter) is an internal CPU register, not a general
  register; it holds the address of the first word of the instruction being
  executed. It is used only in the description of instruction semantics.
  [booklet p.22]
- **M-8** The assembler only translates. Linking and loading are not part of
  the assignment and must not be written. [booklet p.19, p.33, p.38]

### 2.2 Instruction encoding (ENC)

- **ENC-1** A machine instruction consists of an operation and 0 to 2
  operands. With two operands the first is the source operand and the second
  is the destination operand. [booklet p.19, p.28]
- **ENC-2** An instruction is encoded in consecutive memory words, from one
  word up to a maximum of five words, depending on the addressing modes of its
  operands. [booklet p.19]
- **ENC-3** The first word of every instruction always has this layout:
  bits 9-6 opcode, bits 5-4 source operand addressing mode, bits 3-2
  destination operand addressing mode, bits 1-0 the A,R,E field.
  [booklet p.19, p.20]
- **ENC-4** The A,R,E field of the first word of an instruction is `00`
  (absolute). [inferred] The booklet never states this in a sentence; every
  first word in the worked examples ends in `00` (p.31-32, p.38, p.43), and
  p.29 defines "absolute" as a word whose content does not depend on where the
  code is loaded, which holds for the first word.
- **ENC-5** A,R,E values: `00` absolute (A), `01` external (E), `10`
  relocatable (R). [booklet p.20, p.42]
- **ENC-6** The A,R,E bits are added only to the encodings of instructions
  (not of data), and they are also added to every additional word of those
  encodings. [booklet p.20, p.29]
- **ENC-7** Meaning of the field: A means the word's content does not depend
  on the load address (for example a word holding an immediate operand). R
  means the content depends on the load address (for example a word holding
  the address of a label defined in the source file). E means the content
  depends on the value of an external symbol (a label not defined in the
  source file). [booklet p.29, p.42]
- **ENC-8** A word that holds the address of an external symbol is encoded as
  address bits all zero and A,R,E = `01`, that is `00000000-01`.
  [correction 3] [booklet p.43 rows 103 and 108, p.45]
- **ENC-9** For a one-operand instruction, the operand is encoded as a
  destination operand. The source mode field (bits 5-4) of the first word is
  unused and is zero. [booklet p.23, p.40]
- **ENC-10** For a zero-operand instruction the encoding is a single word; the
  source and destination mode fields (bits 5-2) are zero. [booklet p.24, p.40]
- **ENC-11** Each operand needs at most two additional words. If only one
  operand needs additional words, they belong to it. If both operands need
  additional words, the first additional words belong to the source operand
  and the last ones to the destination operand. [booklet p.20]
- **ENC-12** Bits of an additional word that are not used are zero.
  [booklet p.22 (end of the addressing table)]
- **ENC-13** Every machine-code word is written in "unique base 4": the four
  digits are `a`, `b`, `c`, `d`, where `a`=0, `b`=1, `c`=2, `d`=3. A 10-bit
  word is written as exactly five base-4 digits, with leading `a` digits as
  needed. [booklet p.19] [correction 2]
- **ENC-14** Instruction names are always written in lowercase.
  [booklet p.20, p.28]
- **ENC-15** Negative values are stored in two's complement, truncated to the
  width of the field: 8 bits (bits 9-2) for an immediate operand, 10 bits for
  a data word. Example: `#-5` gives `11111011-00`; `.data -9` gives
  `1111110111`. [booklet p.19 (two's complement), p.21, p.32 rows 110 and 130]

### 2.3 Addressing modes (ADR)

- **ADR-1** There are four addressing modes, numbered 0 to 3. [booklet p.20]
- **ADR-2** Mode 0, immediate. Syntax: the operand starts with `#`, and
  attached to it immediately after comes an integer in decimal. Example:
  `mov #-1,r2`. [booklet p.21]
- **ADR-3** Mode 0 encoding: one additional word that holds the operand
  itself, a number represented in 8 bits (bits 9-2), plus the A,R,E field
  (`00`). [booklet p.21, p.31-32 row 110]
- **ADR-4** Mode 1, direct. Syntax: the operand is a label that has already
  been declared or will be declared later in the file. A label is declared by
  writing it at the start of a `.data`, `.string` or `.mat` directive, or at
  the start of an instruction, or as the operand of a `.extern` directive.
  Example: `dec x`. [booklet p.21]
- **ADR-5** Mode 1 encoding: one additional word that holds the address of a
  memory word, represented in 8 bits (bits 9-2), plus the A,R,E field: `10`
  for a label defined in the file, `01` with a zero address for an external
  label. [booklet p.21, p.29, p.43] [correction 3]
- **ADR-6** Mode 2, matrix access. Syntax: the name of the label that denotes
  the matrix, followed by the row and the column, each given by a register
  only and each written in square brackets. Example: `add #4, a[r2][r5]`.
  Indices start at 0. [booklet p.21]
- **ADR-7** Mode 2 encoding: two additional words. The first is the address of
  the start of the matrix (the label), encoded like a mode 1 word (ADR-5). The
  second holds the index registers: bits 9-6 the number of the row register,
  bits 5-2 the number of the column register, A,R,E = `00`.
  [booklet p.21, p.31 rows 101-102]
- **ADR-8** Mode 3, direct register. Syntax: the operand is the name of a
  register. Example: `mov r1,r2`. [booklet p.21]
- **ADR-9** Mode 3 encoding, destination register: one additional word holding
  the register number in bits 5-2. [booklet p.21]
- **ADR-10** Mode 3 encoding, source register: one additional word holding the
  register number in bits 9-6. [correction 4] (The booklet's p.21 wording is
  "in the six bits 6-9"; the bit positions are the same, only the word "six"
  is wrong.)
- **ADR-11** If an instruction has two operands and both are registers, they
  share one additional word: bits 9-6 the source register, bits 5-2 the
  destination register. [booklet p.21-22]
- **ADR-12** A register word has A,R,E = `00`. [booklet p.22, p.32 rows 105,
  112, 118]
- **ADR-13** The sharing in ADR-11 applies only when both operands are in
  mode 3. Index registers of a matrix operand are never merged with a register
  operand: `mov M1[r3][r3],r3` takes four words (first word, matrix address,
  index word, destination register word). [booklet p.32 rows 115-118]
- **ADR-14** Instruction length in words = 1 + (words of source operand) +
  (words of destination operand), where modes 0, 1 and 3 take one word, mode 2
  takes two words, and two mode 3 operands together take one word. The maximum
  is 5 (two matrix operands). [inferred] Direct consequence of ENC-2, ENC-11
  and ADR-3 to ADR-11.
- **ADR-15** The addressing mode of an operand is determined by its syntax: a
  number after `#` is immediate, a label is direct, and so on.
  [booklet p.40]

### 2.4 Instruction table (INS)

- **INS-1** There are 16 operations. Their opcodes are: mov=0, cmp=1, add=2,
  sub=3, lea=4, clr=5, not=6, inc=7, dec=8, jmp=9, bne=10, jsr=11, red=12,
  prn=13, rts=14, stop=15. [correction 1] [booklet p.35]
- **INS-2** The table on p.20 and the opcode columns on p.22-23 are wrong for
  six operations and must not be used: they give not=4, lea=6, red=11, prn=12,
  jsr=13 (clr=5 happens to be right). [correction 1]
- **INS-3** Instructions fall into three groups by operand count: two operands
  (mov, cmp, add, sub, lea), one operand (clr, not, inc, dec, jmp, bne, jsr,
  red, prn), no operands (rts, stop). [booklet p.22-24]
- **INS-4** The legal addressing modes of each operand of each instruction are
  those of the table in section 3. [booklet p.35]
- **INS-5** Semantics (not needed for encoding, recorded for completeness):
  mov copies source to destination; cmp compares the operands and sets the Z
  flag; add and sub store destination ± source in the destination; lea puts
  the address of the source label in the destination; clr zeroes; not inverts
  all bits; inc and dec add and subtract one; jmp jumps; bne jumps if Z is 0;
  jsr calls a subroutine (pushes the return address); red reads a character
  from stdin; prn prints a character to stdout; rts returns from a subroutine;
  stop halts. [booklet p.22-24]

### 2.5 Statements and lines (SYN, part 1)

- **SYN-1** A source file consists of lines; each statement is on its own
  line. Statements are separated by the newline character. [booklet p.25]
- **SYN-2** A line of the source file is at most 80 characters long, not
  counting the newline. A longer line is an error that must be reported.
  [booklet p.25]
- **SYN-3** There are four statement types: empty, comment, directive,
  instruction. [booklet p.25]
- **SYN-4** An empty statement is a line that contains only whitespace, that
  is only spaces and tabs, or nothing at all. The assembler ignores it.
  [booklet p.25, p.40]
- **SYN-5** A comment statement is a line whose first character is `;`. The
  assembler ignores the line completely. [booklet p.25, p.40]
- **SYN-6** A directive statement tells the assembler what to do; it may
  allocate and initialise memory, but it produces no instruction encoding.
  [booklet p.25]
- **SYN-7** An instruction statement produces the encoding of one machine
  instruction. [booklet p.25]
- **SYN-8** Whitespace is recognised with the C library function `isspace`,
  not only space and tab. This overrides the booklet's "only spaces and tabs"
  in SYN-4. [forum: Whitespace Characters, 03/06/2025]
- **SYN-9** A comment exists only as a whole line whose very first character
  is `;`. A `;` after leading whitespace, or after a statement on the same
  line, does not start a comment (the text is then ordinary, erroneous,
  statement text). [forum: Line with More Than 80 Characters: Does the
  Assembler Ignore the Line and Continue or Stop?, 14/07/2025] This confirms
  the literal reading of SYN-5.
- **SYN-2a** A line longer than 80 characters is an error that must be
  reported, and the assembler continues to look for errors in the rest of the
  file. [forum: Line with More Than 80 Characters: Does the Assembler Ignore
  the Line and Continue or Stop?, 14/07/2025; Should we continue checking when
  the line length is greater than 80 characters?, 22/07/2025] The sources
  leave the stage that performs the check to the implementer; SYN-2b fixes
  it.
- **SYN-2b** The 80-character limit is checked by the pre-assembler, on every
  line of the `.as` file. A longer line is a pre-assembler error (ERR-10): it
  is reported, the pre-assembler keeps scanning the rest of the file for
  further pre-assembler errors (MAC-22), no `.am` file is left and the passes
  are not run for that file (MAC-23). [owner decision, 2026-11-26] (choice
  C-2), allowed by [forum: Character Overflow Error, 09/06/2025; Checking Line
  Length Validity, 22/06/2025], both answered "at your discretion".
  [inferred] The limit applies to every line whatever its kind (comment
  lines, lines of a macro body), because SYN-2 speaks of "a line of the
  source file" with no exception. Consequence: errors that only the passes
  could find in such a file are not reported in the same run. Two later
  decisions narrow "every line": the carriage return of a CR LF line ending
  is not counted (SYN-25), and the lines discarded after a faulty `mcro` line
  are not checked at all (MAC-38).
- **SYN-24** A last line that is not terminated by a newline character is
  accepted and is treated as a complete line, exactly as if a newline
  followed it. The `.am` file always ends every line with a newline, so the
  copy of such a line in the `.am` file gets the newline that the source
  lacked. [owner decision, 2026-11-26] (Q-32 a). The sources are silent on a
  missing final newline. For such a file without macros the `.am` file
  differs from the `.as` file by that one newline; this is a deliberate
  deviation from the letter of MAC-14.
- **SYN-25** A carriage return (ASCII 13) that stands immediately before the
  newline character (a CR LF line ending) is removed when the line is read:
  it is not part of the line, it does not count toward the 80 characters of
  SYN-2, and it is not copied to the `.am` file, which is always written
  with LF line endings. A carriage return anywhere else in a line, including
  one at the end of a last line that has no newline, is an ordinary
  whitespace character under SYN-8. [owner decision, 2026-11-26] (Q-32 b).
  The sources are silent on line endings. For a CR LF source without macros
  the `.am` file differs from the `.as` file by the removed carriage
  returns; this too is a deliberate deviation from the letter of MAC-14.
- **SYN-26** A source line that contains a NUL byte (value 0) is an error
  (ERR-46), reported by the pre-assembler on that line. The length of a line
  for SYN-2 is the number of bytes up to its line terminator, NUL bytes
  included, so a NUL byte cannot hide the rest of a line from the length
  check or from the `.am` file. A line that is both too long and contains a
  NUL byte gets one message (ERR-R16), the line-length one. [project decision, 2026-11-26, pending the owner's confirmation]
  The sources are silent: they describe the input as text. Found by the
  independent correctness review of 2026-11-26 (finding D-1).

### 2.6 Directives (DIR)

- **DIR-1** A directive statement has: an optional label definition at the
  start, then the directive name, then its parameters (their number depends on
  the directive). [booklet p.25]
- **DIR-2** A directive name starts with `.` followed by lowercase letters
  only. [booklet p.25]
- **DIR-3** Machine words produced by a directive have no A,R,E field; the
  value fills all 10 bits of the word. [booklet p.26]
- **DIR-4** There are (despite the booklet's phrase "four kinds") five
  directives: `.data`, `.string`, `.mat`, `.entry`, `.extern`. [booklet
  p.26-27]

`.data`

- **DIR-5** The parameters of `.data` are legal integers, one or more,
  separated by commas. Example: `.data 7, -57, +17, 9`. [booklet p.26]
- **DIR-6** Any number of spaces and tabs, or none, may appear between a
  number and a comma and between a comma and a number. A comma must appear
  between numbers. There must not be more than one comma between two numbers,
  nor a comma after the last number, nor a comma before the first number.
  [booklet p.26]
- **DIR-7** `.data` allocates one word in the data image per value, stores the
  values in order, and advances the data counter by the number of values.
  [booklet p.26, p.40]
- **DIR-8** If the `.data` line has a label, the label gets the value of the
  data counter before the values are stored, and enters the symbol table with
  the attribute "data". [booklet p.26, p.35 step 6, p.41]

`.string`

- **DIR-9** `.string` has exactly one parameter, a legal string (SYN-22).
  [booklet p.26]
- **DIR-10** The characters of the string are encoded by their ASCII values
  and stored in the data image in order, one character per word. A word with
  the value 0 is added after them to mark the end of the string. The data
  counter advances by the length of the string plus one. Example:
  `STR: .string "abcdef"` allocates 7 words. [booklet p.26-27, p.41]
- **DIR-11** A label on a `.string` line is handled exactly like a label on a
  `.data` line (DIR-8). [booklet p.26, p.41]

`.mat`

- **DIR-12** `.mat` allocates a matrix. Its form is the directive name, then
  the number of rows in square brackets, then the number of columns in square
  brackets, then an optional comma-separated list of integer initial values.
  Examples: `MAT8: .mat [2][3]` and `MAT5: .mat [2][2] 4,-5,7,9`.
  [booklet p.27]
- **DIR-13** A matrix with no initial values is not initialised with listed
  values but still takes rows × columns words in the data image; these cells
  are initialised to 0. [booklet p.27, p.41]
- **DIR-14** Initial values are listed left to right in row order (row-major):
  the first value is cell [0][0], then [0][1], and so on. The matrix holds
  integers only. [booklet p.27]
- **DIR-15** Each initial value is stored in one data word and advances the
  data counter by one. [booklet p.41]
- **DIR-16** A label on a `.mat` line is handled exactly like a label on a
  `.data` line (DIR-8): value = data counter, attribute "data".
  [booklet p.29, p.35 steps 5-6, p.41]
  [forum: Symbol characterization in the symbol table, 22/07/2025] The blank
  attribute cell of `M1` in the symbol table printed on p.37 is a mistake; it
  should be "data".

`.entry`

- **DIR-17** `.entry` has one parameter: the name of a label that is defined
  in the current source file (it gets its value in this file). The directive
  marks the label so that other source files can use it. The directive may
  appear before the definition of the label. [booklet p.27]
- **DIR-18** A label defined at the start of a `.entry` line is meaningless
  and the assembler ignores it; the assembler may print a warning.
  [booklet p.27] (The forum adds nothing: asked what such a label means, the
  moderator pointed back to the booklet text. [forum: Questions I didn't find
  answers to in the booklet., 22/07/2025]) This project prints the warning
  and ignores the label completely (DIR-35).
- **DIR-19** `.entry` is skipped in the first pass and handled in the second
  pass: the attribute "entry" is added to the symbol. If the symbol is not in
  the symbol table, an error is reported. [booklet p.35 step 9, p.36 steps
  4-5]

`.extern`

- **DIR-20** `.extern` has one parameter: the name of a label that is not
  defined in the current source file. It tells the assembler that the label is
  defined in another file and is used in this one. [booklet p.27]
- **DIR-21** The symbol named by `.extern` enters the symbol table with the
  value 0 and the attribute "external". [booklet p.35 step 10, p.41]
- **DIR-22** A label defined at the start of a `.extern` line is meaningless
  and the assembler ignores it; the assembler may print a warning.
  [booklet p.28] This project prints the warning and ignores the label
  completely (DIR-35).
- **DIR-23** The same label must not be declared both `.entry` and `.extern`
  in the same file. This is an error and must be reported. [booklet p.28]

Forum additions to directives

- **DIR-24** If a `.mat` directive lists fewer initial values than
  rows × columns, the listed values fill the first cells in row order and all
  remaining cells get the value 0. This is not an error. The booklet covered
  only "all values" and "no values". [forum: Partial Values in Matrix,
  02/07/2025]
- **DIR-25** If a `.mat` directive lists more initial values than
  rows × columns, it is an error. [forum: Initializing a Matrix with More
  Values Than its Size, 14/07/2025]
- **DIR-26** The dimensions of a `.mat` directive are numbers. Anything else
  in the brackets, for example registers as in `.mat [r2][r1]`, is an error.
  [forum: Can .mat be Defined by Non-Constant Values?, 14/07/2025]
- **DIR-27** Whitespace in the dimension part of `.mat`: whitespace is allowed
  inside each pair of brackets (`[ 2 ][ 3 ]`); the second opening bracket must
  be adjacent to the first closing bracket (`][`, nothing between them).
  [forum: Spaces within square brackets in a matrix, 22/07/2025] Whitespace
  between `.mat` and the first `[` is mandatory by SYN-17.
- **DIR-28** `.data`, `.string` and `.mat` are legal without a label; the data
  is still allocated. [forum: Should We Deal with Labels that Waste Space?,
  02/07/2025] (The booklet agrees: the label is optional, p.25.)
- **DIR-29** `.entry` may name any label defined in the file, including a
  label defined on a `.data`, `.string` or `.mat` line. [forum: Translation
  Unit and Second Pass Errors, 14/07/2025] (The booklet example on p.43 does
  this with `LENGTH`.)
- **DIR-30** A label cannot be external and internal at the same time. A label
  declared `.entry` must be defined in the same file. A label declared
  `.extern` must not be defined in the same file; defining it there is an
  error, whichever line comes first. [forum: Translation Unit and Second Pass
  Errors, 02/07/2025] This extends the booklet, whose skeleton algorithm
  catches only the order "`.extern X` first, `X:` later" (p.35 steps 6, 11).
  The stage and the line of the report are fixed by DIR-36.
- **DIR-31** A string given to `.string` does not need, and is not checked
  for, any terminator of its own; the assembler always adds the terminating
  zero word in memory. [forum: Questions I didn't find answers to in the
  booklet., 22/07/2025]

Owner decisions on directives

- **DIR-32** `.string ""` is legal. It allocates exactly one word, which holds
  0 (the terminator of DIR-10), and advances the data counter by one.
  [owner decision, 2026-11-26] (Q-17). The sources are silent: SYN-22 defines
  a string as "a sequence" of characters without a minimum length, and DIR-10
  and DIR-31 always add the terminating word.
- **DIR-33** A dimension of `.mat` is a string of decimal digits with no sign,
  and its value must be at least 1. A dimension of 0, a negative number, or a
  number written with a sign (`+2`, `-2`) is an error (ERR-44). Leading zeros
  are digits and are legal. Whitespace between the last `]` and the first
  initial value is optional: `.mat [2][2]1,2,3,4` is legal.
  [owner decision, 2026-11-26] (Q-7). The sources are silent on all four
  points; DIR-26 says only that a dimension is a number.
- **DIR-34** Declaring the same symbol more than once with `.extern`, or more
  than once with `.entry`, is accepted silently: no error and no warning. The
  symbol is in the symbol table once, and an entry symbol appears exactly once
  in the `.ent` file (OUT-8). [owner decision, 2026-11-26] (Q-8), implied by
  the skeleton algorithms, which test for an existing symbol when a label is
  defined (p.35 steps 6 and 11) but not at `.extern` (p.35 step 10), and
  which merely add an attribute at `.entry` (p.36 step 5). [booklet p.35,
  p.36] No forum post addresses the repetition. A symbol that is both
  defined in the file and declared `.extern` remains an error (DIR-30), and so
  does a symbol declared both `.entry` and `.extern` (DIR-23).
- **DIR-35** A label in front of `.entry` or `.extern` produces a warning
  (WRN-1) in the format of ERR-R14, on that line, reported by the first
  pass. The label is otherwise ignored completely: it is not entered into
  the symbol table, and its name is not validated in any way (not its
  syntax, not its length, not against the reserved words, not against the
  macro names, not against the symbols already defined; ERR-11 to ERR-15 do
  not apply to it). The directive itself is processed as if the label were
  not there. The warning does not suppress the output files and does not
  change the exit status. [owner decision, 2026-11-26] (Q-27), the option
  offered by [booklet p.27, p.28] ("the assembler ignores this label"; a
  warning may be printed); the forum adds nothing (DIR-18). [inferred]
  Because the name is not validated, the line is recognised by its fields
  alone: its first whitespace-delimited field ends with `:` and its second
  field is exactly `.entry` or `.extern`. Any other line follows the
  ordinary rules, so `L:.extern W` is ERR-38 and `L : .extern W` is ERR-16.
  If the directive on such a line has a first-pass error of its own (ERR-34,
  ERR-53), only the error is printed (ERR-R16). The line is a line of the
  `.am` file, so the warning of a macro body line is printed once per call
  (ERR-R12).
- **DIR-36** A symbol that is both defined as a label in the file and
  declared `.extern` (ERR-53, DIR-30) is reported by the first pass, once,
  on whichever of the two lines comes second: on the label definition line
  if the `.extern` line came first, and on the `.extern` line if the label
  definition came first. On the label definition line the message is ERR-53,
  not ERR-15. [owner decision, 2026-11-26] (Q-30 b). The error itself is
  required by [forum: Translation Unit and Second Pass Errors, 02/07/2025];
  the order "`.extern` first" is the one the skeleton catches [booklet p.35
  steps 6, 11]; the sources do not name the stage or the line for the other
  order.

### 2.7 Labels and symbols (LBL)

- **LBL-1** A label is a symbol defined at the start of an instruction
  statement or at the start of a `.data`, `.string` or `.mat` directive.
  [booklet p.28, p.29]
- **LBL-2** A legal label starts with an alphabetic letter (uppercase or
  lowercase), followed by any sequence of alphabetic letters (uppercase or
  lowercase) and/or digits. [booklet p.28]
- **LBL-3** The maximum length of a label is 30 characters. [booklet p.28]
- **LBL-4** A label definition ends with `:`. The colon is not part of the
  label; it only marks the end of the definition. The colon must be attached
  to the label, with no whitespace between them. [booklet p.29]
- **LBL-5** The same label must not be defined more than once (on different
  lines). The assembler verifies that every symbol is defined exactly once.
  [booklet p.29, p.34, p.35 steps 6 and 11]
- **LBL-6** Uppercase and lowercase letters are different: labels are
  case-sensitive. Examples of legal labels: `hEllo`, `x`, `He78902`.
  [booklet p.29]
- **LBL-7** Reserved words of the assembly language cannot be label names: the
  name of an operation, the name of a directive, the name of a register.
  [booklet p.29] The complete list used by this project is LBL-20.
- **LBL-8** The same symbol must not serve both as a label and as a macro
  name. The assembler must check this. [booklet p.29] How and where the check
  is made is LBL-21.
- **LBL-9** A label defined on a `.data`, `.string` or `.mat` line gets the
  current value of the data counter (DC). A label defined on an instruction
  line gets the current value of the instruction counter (IC), that is the
  address of the first word of the instruction, with the attribute "code".
  [booklet p.28, p.29, p.35 step 11, p.40]
- **LBL-10** A label may be used as an operand before it is defined, provided
  it is defined somewhere in the file (by a label definition or by `.extern`).
  [booklet p.22, p.33, p.41]
- **LBL-11** An instruction operand may use a symbol that is not defined as a
  label in the current file, as long as the symbol is declared external by a
  `.extern` directive somewhere in the file. [booklet p.29]
- **LBL-12** The symbol table stores for each symbol its name, its value, and
  attributes: the location ("data" or "code") or the kind of update
  ("external"), plus "entry" added in the second pass. [booklet p.36 step 5,
  p.39]
- **LBL-13** An operand symbol that is not found in the symbol table in the
  second pass is an error. [booklet p.36 step 6]

Forum additions to labels

- **LBL-14** `mcro` and `mcroend` are reserved words and are not legal label
  names. [forum: Reserved Words for Label Names, 03/06/2025]
- **LBL-15** The check of LBL-8 (a label must not have the name of a macro) is
  mandatory, not an assumption. [forum: Should we check if a label name is not
  a macro name or is it considered a given?, 22/07/2025]
- **LBL-16** A name that looks like a register but is outside r0..r7, such as
  `r20`, is not a register. As an operand it is a label reference (a label
  with that name may be defined). If no such label is defined, an error is
  reported; the message may say either "undefined label" or "register out of
  range". [forum: How to Treat an Operand that is a Register and Not in
  Range?, 14/07/2025] This narrows the booklet's "register name that does not
  exist" error (p.34).
- **LBL-17** The assembler does not check what kind of statement a label was
  defined on. Any defined label may be used in direct mode or as the label of
  a matrix operand, whether it was defined on an instruction, `.data`,
  `.string` or `.mat` line; for example `M1: .data 1,2,3,4` followed by
  `mov M1[r3][r3],r3` is legal. Only the addressing-mode table (section 3)
  limits an operand. [forum: Matrix Labels, 25/06/2025; The role of the label
  in different commands and specifically lea, 22/07/2025]
- **LBL-18** Whitespace is mandatory between the colon of a label definition
  and the statement that follows: `LABEL:stop` is an error.
  [forum: Is Whitespace Required Between a Colon (After a Label Name) and an
  Instruction/Directive?, 03/06/2025] The booklet is silent.
- **LBL-19** No maximum number of labels may be assumed. [forum: Label and
  Macro Limits, 02/07/2025]

Owner decisions on labels

- **LBL-20** The reserved words are exactly these 31 names, compared
  case-sensitively: the 16 operation names of section 3; the five directive
  names written without the dot, `data`, `string`, `mat`, `entry`, `extern`;
  the eight register names `r0` to `r7`; `mcro` and `mcroend`. None of them
  may be a label name (ERR-13) or a macro name (MAC-33, ERR-2). `PSW` and `PC`
  are **not** reserved and are legal label and macro names. Names such as `r8`
  or `r20` are not reserved (LBL-16). [owner decision, 2026-11-26] (Q-9).
  Sources: operation, directive and register names [booklet p.29]; `mcro` and
  `mcroend` [forum: Reserved Words for Label Names, 03/06/2025]. [inferred]
  The booklet's "name of a directive" can only mean the name without the dot,
  because a name with a dot is never a syntactically legal label (LBL-2).
  The sources say nothing about `PSW` and `PC`, which never appear in the
  assembly language (M-3, M-7).
- **LBL-21** A label whose name equals the name of a macro defined in the same
  file is an error (ERR-14), wherever the label definition and the macro
  definition stand relative to each other. The error is detected in the first
  pass, which consults the macro table built by the pre-assembler; the macro
  table of a file therefore lives until the first pass of that file has
  ended. The error is reported on the line of the label definition.
  [owner decision, 2026-11-26] (Q-11). The check itself is mandatory by
  [booklet p.29] and [forum: Should we check if a label name is not a macro
  name or is it considered a given?, 22/07/2025]; the sources do not name the
  stage.
- **LBL-22** A label definition with nothing after the colon (only whitespace
  up to the end of the line) is an error (ERR-45). [owner decision,
  2026-11-26] (Q-14), implied by the booklet: a label is defined "at the
  start of" an instruction statement or of a `.data`, `.string` or `.mat`
  directive (p.28, p.29), an empty statement holds only whitespace (p.25),
  and the skeleton first pass has no branch for a symbol followed by nothing
  (p.35 steps 3 to 11). No forum post addresses it.
- **LBL-23** A syntactically legal, non-reserved label on a line that has
  another error is still entered into the symbol table (with the attribute
  the statement would have given it when the statement kind is recognisable,
  otherwise "code"), so that the line does not cause follow-on "undefined
  symbol" messages elsewhere. Its value does not matter, because no output
  is written for a file with errors (ERR-R6). [owner decision, 2026-11-26]
  (Q-14). The sources are silent.

### 2.8 Syntax details (SYN, part 2)

Instruction statements

- **SYN-10** An instruction statement has: an optional label, the operation
  name, and 0 to 2 operands according to the operation. Forms:
  `label: opcode source-operand, target-operand`,
  `label: opcode target-operand`, `label: opcode`. [booklet p.28]
- **SYN-11** The operation name is always lowercase and is one of the 16
  operations. An unknown operation name is an error. [booklet p.28, p.34,
  p.35 step 12]
- **SYN-12** The operation name and the first operand must be separated by
  spaces and/or tabs, one or more. [booklet p.28]
- **SYN-13** Two operands are separated by a comma. The operands need not be
  attached to the comma; any number of spaces and/or tabs on both sides of the
  comma is legal. [booklet p.28, p.46]
- **SYN-14** A wrong number of operands is an error (for example "too many
  operands" when two operands are given to a one-operand instruction). An
  operand whose addressing mode is not legal for the operation is an error. A
  register name that does not exist is an error. [booklet p.34]

Whitespace

- **SYN-15** Redundant whitespace must be accepted and skipped: any number of
  spaces and tabs before and after a comma, and before and after the operation
  name. Blank lines are allowed. [booklet p.46]
- **SYN-16** Whitespace is not allowed between a label and its colon (LBL-4),
  nor between `#` and the number of an immediate operand ("attached", ADR-2).
  [booklet p.21, p.29]
- **SYN-17** Whitespace is mandatory between a directive name and its
  operands. `.data1,2`, `.string"x"`, `.mat[5][2]`, `.externL` and `.entryL`
  are errors. [forum: Spaces in directives, 22/07/2025] The booklet is silent
  for directives (it states the requirement only for instructions, SYN-12).
- **SYN-18** In a matrix operand the first opening bracket must be adjacent to
  the label, and the second opening bracket must be adjacent to the first
  closing bracket: `M1 [r2][r7]` and `M1[r2] [r7]` are errors. Whitespace
  **inside** the brackets is allowed: `M1[ r2 ][ r6 ]` is legal.
  [forum: Spaces within square brackets in a matrix, 22/07/2025] The booklet
  is silent on all of this (p.21 only shows the form).
- **SYN-19** Whitespace before the first field of a line (indentation) is
  allowed. [booklet p.46 ("before and after the operation name"), and the
  indented example listings on p.30, p.43]

Numbers and strings

- **SYN-20** A legal number starts with an optional sign, `-` or `+`, followed
  by a sequence of decimal digits. Examples: `76`, `-5`, `+123`. No base other
  than decimal is supported, and non-integers are not supported.
  [booklet p.29]
- **SYN-21** Value ranges. The booklet does not state ranges and does not say
  that an out-of-range number is an error. The field widths are: an immediate
  operand is "a number represented in 8 bits" [booklet p.21]; a data word uses
  all 10 bits [booklet p.26]; an address is "represented in 8 bits"
  [booklet p.21]. The assembler must check that a number does not overflow
  its field; an overflow is an error. This applies to `.data` values and to
  immediate operands. [forum: Maximum size of numbers using 2's complement
  method, 14/07/2025] The moderator gave no numeric range. The ranges are
  signed only: an immediate operand must be in -128..127, and a `.data` or
  `.mat` value must be in -512..511. Any value outside its range, including
  the unsigned upper halves 128..255 and 512..1023, is an error (ERR-39).
  [owner decision, 2026-11-26] (Q-6), implied by the sources: the machine
  works with positive and negative integers in two's complement [booklet
  p.19], an immediate operand is a number represented in 8 bits [booklet
  p.21], a data word has 10 bits [booklet p.26], and overflow must be checked
  (the ruling above). (A student post in the same thread argues for
  -128..127; it is not a ruling.)
- **SYN-22** A legal string is a sequence of visible (printable) ASCII
  characters enclosed in double quotes. The quotes are not part of the string.
  Example: `"hello world"`. [booklet p.29] Forum refinements: the legal
  characters are exactly the ASCII codes 32 to 126; every character between
  the opening and the closing quotation mark is stored as it is, with no
  escape sequences, so a backslash is an ordinary character and a quotation
  mark inside the string is an ordinary character too (the string ends at the
  last quotation mark). [forum: What is Considered a Valid String for
  .string?, 22/06/2025 and 25/06/2025] The quotation mark is the ASCII
  character; the typographic quotes printed in the booklet are an artefact.
  [forum: What is Considered a Valid String for .string?, 23/06/2025] The
  empty string `""` is legal (DIR-32).
- **SYN-23** A matrix operand must have a label before the brackets. An
  operand that starts with a bracket (`[r2][r7]`) is an error that must be
  reported; it may not be assumed away. [forum: Matrix definition, 17/07/2025]

### 2.9 Macros and pre-assembler (MAC)

- **MAC-1** A program is built from macros and statements. A macro is a named
  block of statements; using the macro at some place in the program expands
  the block at that place. [booklet p.24]
- **MAC-2** A macro definition starts with a line `mcro <name>` and ends with
  a line `mcroend`; the lines between them are the body. Example:
  `mcro a_mc` / `inc r2` / `mov A,r1` / `mcroend`. [booklet p.24]
- **MAC-3** Using a macro is simply mentioning its name. In all examples the
  name stands alone on its line, and the skeleton algorithm tests whether the
  *first field* of the line is a macro name. [booklet p.24, p.30, p.31 step 2]
  The forum settles what the booklet leaves implicit: a macro name is a call
  only when it stands on a separate line; a macro name that appears as one
  word among others on a line is not expanded. [forum: Must a Macro Contain a
  Sequence of Instructions and/or Directives?, 25/06/2025]
- **MAC-4** The pre-assembler runs first. The assembler works on the program
  after macro expansion: "the program after macro expansion is the program the
  assembler is to translate". [booklet p.25, p.29]
- **MAC-5** In the expanded file each macro call is replaced by the body
  lines, and the definition lines (`mcro` line, body, `mcroend` line) are
  removed. All other lines are copied unchanged. [booklet p.30, p.31] The
  exact content of the `.am` file is fixed by MAC-29.
- **MAC-6** Assumption, no check needed: there are no nested macro
  definitions. [booklet p.25]
- **MAC-7** Assumption, no check needed: every `mcro` line has a closing
  `mcroend` line. [booklet p.25, p.34]
- **MAC-8** Assumption, no check needed: a macro is always defined before it
  is called. [booklet p.25]
- **MAC-9** Must check: the macro name is valid; the name of an instruction or
  of a directive cannot be a macro name ("it is not an instruction name and
  the like"). [booklet p.25]
- **MAC-10** Must check: there are no extra characters on the definition line
  (after the macro name) and none on the `mcroend` line. [booklet p.25]
- **MAC-11** If an error is found in the macro expansion stage, the assembler
  cannot go on to the next stages for that file: it must stop, report the
  errors, and move on to the next source file if there is one. [booklet p.25]
- **MAC-12** Only if macro expansion ends successfully may the assembler move
  to the next stage. "Otherwise, the errors must be shown and no files are
  created." [booklet p.30] (So a failed pre-assembler leaves no `.am` file;
  confirmed by MAC-23.)
- **MAC-13** Errors inside the body of a macro are found in the later stages
  (the passes), not by the pre-assembler. Such an error may be reported once
  for every place where the macro is expanded; this duplication cannot be
  avoided and is accepted. [booklet p.25, p.34]
- **MAC-14** The pre-assembler writes the expanded program to a new file with
  the suffix `.am`. If the source has no macros, the `.am` file is identical
  to the `.as` file. [booklet p.25, p.29-30, p.39] (In this project two
  kinds of input are the exception: a last line without a newline, SYN-24,
  and CR LF line endings, SYN-25.)
- **MAC-15** Duplicate macro names in the forum. The booklet is silent. Four
  moderator posts bear on the question and they do not agree: (1) "Yes" to
  "if multiple macros with the same name are defined, should an error be
  output?" [forum: Do We Need to Check Multiple Definitions of the Same
  Macro?, 08/06/2025]; (2) "Of course" to a question asking whether the
  validity of the macro name, the extra characters on the opening and closing
  lines and "multiple macros with the same name" are checked in the
  pre-assembler [forum: Whitespace in Macros, 18/06/2025]; (3) "You can
  report an invalid macro name (with the line number in addition)" to a
  question that lists "a name that repeats itself" among the kinds of invalid
  macro name [forum: Error Reporting, 25/06/2025]; (4) "There is no need to
  check for duplicate macro names" [forum: Macro name, 22/07/2025]. Posts 1
  to 3 require or allow the report; post 4 makes it optional and forbids
  nothing. This rule records the sources only; the behaviour of this project
  is MAC-28.
- **MAC-17** A macro name follows the rules of a label name (starts with a
  letter, then letters and digits), and in addition the character `_` is
  legal in a macro name. [forum: Valid Macro Names, 03/06/2025] Its length is
  limited like a label name, 30 characters. [forum: Macro name, 22/07/2025]
  The booklet gives no syntax for macro names. The complete rule for this
  project is MAC-33.
- **MAC-18** The pre-assembler does perform the checks of MAC-9 and MAC-10
  (valid macro name; no extra characters on the `mcro` and `mcroend` lines).
  The booklet's sentence on p.34, "the pre-assembler stage contains no error
  detection stage", applies only to the macro body. [forum: Whitespace in
  Macros, 17/06/2025 and 18/06/2025; Error in Pre-assembler, 14/07/2025]
- **MAC-19** Whitespace on the `mcro` and `mcroend` lines is allowed; "extra
  characters" in MAC-10 means non-whitespace characters. [forum: Whitespace in
  Macros, 17/06/2025]
- **MAC-20** On the `mcroend` line only text **after** the word `mcroend`
  needs to be checked; it may be assumed that nothing precedes it.
  [forum: Additional Characters Before mcroend, 02/07/2025]
- **MAC-21** A label cannot appear on the same line as `mcro`, `mcroend` or a
  macro call. [forum: Label at the beginning/end of macro definition,
  22/07/2025] (The one-word answer "No" does not say whether this is an
  assumption or an error to report; MAC-34 makes it an error.)
- **MAC-22** The pre-assembler must find all errors of the macro expansion
  stage in the file; it does not stop at the first one. [forum: Can Macro
  Expansion be Stopped at the First Error or Must the Entire File be
  Processed?, 25/06/2025; Error in Pre-assembler, 14/07/2025]
- **MAC-23** After a pre-assembler error there is no `.am` file for that
  source file and the passes are not run. [forum: Error in Pre-assembler,
  14/07/2025] Writing the `.am` file and deleting it when an error is found is
  an acceptable way to achieve this. [forum: In Case of an Error in a File,
  e.g., in the Macro Expansion Stage, 25/06/2025] This confirms MAC-12.
- **MAC-24** The pre-assembler does not check the validity of the macro body;
  errors there are found by the first or second pass. [forum: Must a Macro
  Contain a Sequence of Instructions and/or Directives?, 25/06/2025]
- **MAC-25** The `.am` file may omit the empty lines and the comment lines of
  the source file; keeping them is equally acceptable. [forum: Handling Empty
  Lines and Comment Lines, 09/06/2025; Should the pre-assembler also copy
  empty lines or can it ignore and delete them?, 17/07/2025] This relaxes the
  booklet's "identical to the source file" (MAC-14). This project keeps them
  (MAC-29).
- **MAC-26** A pre-assembler error message may be general, for example
  "invalid macro name", as long as it carries the line number.
  [forum: Error Reporting, 25/06/2025]
- **MAC-27** No maximum number of macros may be assumed. [forum: Label and
  Macro Limits, 02/07/2025]
- **MAC-16** Skeleton pre-assembler algorithm (not mandatory) [booklet p.31]:
  1. Read the next line of the source file. At end of file go to 9.
  2. If the first field is a macro name found in the macro table, replace the
     name by copying all the matching lines from the table into the file, and
     go to 1. Otherwise continue.
  3. If the first field is `mcro` (start of a definition) continue, otherwise
     go to 6.
  4. Turn on the "in macro" flag.
  5. Enter the macro name into the macro table.
  6. Read the next line of the source file. At end of file go to 9. If the
     "in macro" flag is on and the line is not `mcroend`, enter the line into
     the macro table and delete it from the file. Otherwise (not in a macro)
     go to 1.
  7. If the line is `mcroend`, delete it from the file and continue. If not,
     go to 6.
  8. Turn off the "in macro" flag and go to 1.
  9. End: save the expanded file.

Owner decisions on macros and the pre-assembler

- **MAC-28** A second definition of a macro name that is already in the macro
  table is an error reported by the pre-assembler (ERR-8), on the `mcro` line
  of the second definition. [owner decision, 2026-11-26] (Q-21). Reporting
  satisfies all four moderator posts quoted in MAC-15: it is required by
  [forum: Do We Need to Check Multiple Definitions of the Same Macro?,
  08/06/2025] and [forum: Whitespace in Macros, 18/06/2025], offered by
  [forum: Error Reporting, 25/06/2025], and not forbidden by [forum: Macro
  name, 22/07/2025].
- **MAC-29** Content of the `.am` file. Every line of the `.as` file that is
  not part of a macro definition and is not a macro call is copied exactly as
  it is, character for character; this includes empty lines and comment
  lines. Only the line terminator is normalised: every line of the `.am`
  file ends with one newline character, also when the last source line had
  none (SYN-24), and the carriage return of a CR LF line ending is dropped
  (SYN-25). The lines of a macro definition (the `mcro` line, the body lines and
  the `mcroend` line) are removed. Each macro call line is replaced by the
  body lines of the macro, each copied exactly as it was written in the
  definition. Consequently a source file without macros, with LF line
  endings and a final newline, gives an `.am` file identical to the `.as`
  file, as the booklet says (MAC-14). [owner decision,
  2026-11-26] (choice C-3), allowed by [forum: Handling Empty Lines and
  Comment Lines, 09/06/2025; Should the pre-assembler also copy empty lines
  or can it ignore and delete them?, 17/07/2025] ("it is possible to create
  an .am file without" them; "you can delete") and matching [booklet p.30,
  p.39]. Empty lines and comment lines inside a macro body are body lines:
  they are removed with the definition and reappear at every call.
  [inferred] from MAC-24: the pre-assembler does not interpret the body.
- **MAC-30** A macro call line is a line whose first field is the name of a
  macro in the macro table (MAC-16 step 2). Any non-whitespace text after the
  macro name on that line is an error reported by the pre-assembler (ERR-7).
  Whitespace before and after the name is allowed. [owner decision,
  2026-11-26] (choice C-4), the option the moderator called "more logical"
  [forum: Macro Mention with Additional Text, 14/07/2025].
- **MAC-31** Non-whitespace text after the word `mcroend` is an error
  reported by the pre-assembler (ERR-4), and the macro definition is closed
  at that line: the lines that follow are ordinary lines again. [owner
  decision, 2026-11-26] (choice C-5). The report is required by MAC-10; the
  closing is left "at your discretion" by [forum: Redundant Text After
  Command, 14/07/2025].
- **MAC-32** Macro expansion is not nested. A line inside a macro body is
  stored and later copied as it is, even when its first field is the name of
  a macro defined earlier; it is not expanded. In the `.am` file such a line
  is an ordinary line, and the first pass reports it as an unknown operation
  (ERR-17). [owner decision, 2026-11-26] (Q-10), implied by the sources: the
  skeleton algorithm stores body lines without looking at them [booklet p.31
  step 6], and the body is not examined during macro expansion [forum: Must a
  Macro Contain a Sequence of Instructions and/or Directives?, 25/06/2025].
- **MAC-33** A legal macro name starts with an alphabetic letter; the
  characters after the first are letters, digits and `_`; `_` is not allowed
  as the first character; the length is at most 30 characters; and the name
  is not a reserved word (LBL-20), which excludes operation names, directive
  names, register names, `mcro` and `mcroend`. A violation is a
  pre-assembler error (ERR-2, ERR-6). [owner decision, 2026-11-26] (Q-9,
  Q-11), implied by "a macro name must be valid like a label name; the
  character `_` is also valid" [forum: Valid Macro Names, 03/06/2025]
  together with LBL-2 and LBL-7 [booklet p.28, p.29] and MAC-9 [booklet
  p.25]. The clash between a macro name and a label name is handled by
  LBL-21, not by the pre-assembler.
- **MAC-34** A label definition in front of `mcro`, in front of `mcroend`, or
  in front of a macro call is an error (ERR-9). [owner decision, 2026-11-26]
  (Q-22), the stricter reading of the moderator's "No" in [forum: Label at
  the beginning/end of macro definition, 22/07/2025]. The pre-assembler
  reports it; MAC-36 says how the line is recognised and MAC-37 what happens
  to the macro state.
- **MAC-35** The pre-assembler hands the passes, together with the expanded
  text, a line map: for every line of the `.am` file, the number of the line
  of the `.as` file it was copied from. For a line that comes from a macro
  body, that number is the number of the body line inside the macro
  definition, not the number of the call line. The map exists only in memory;
  it is not written to any file and does not change the `.am` file.
  [owner decision, 2026-11-26] (choice C-1); see ERR-R12 and section 2.14.
- **MAC-36** The error of MAC-34 (ERR-9) is reported by the pre-assembler.
  It is a pre-assembler error like any other: no `.am` file is left and the
  passes do not run for that file (MAC-23). The pre-assembler recognises
  such a line, outside a macro body, by its first two whitespace-delimited
  fields: the first field ends with `:`, and the second field is exactly
  `mcro`, exactly `mcroend`, or the name of a macro defined so far (a name
  that is in the macro table when the line is read). The text before the
  colon is not validated. A line that does not match, for example
  `L : mcro x` or `L:mcro x`, is not recognised: it is an ordinary line, and
  by MAC-16 its first field is neither `mcro` nor a macro name. While a
  definition is open, only `L: mcroend` is recognised; other body lines are
  not examined (MAC-24, MAC-32). [owner decision, 2026-11-26] (Q-28). The
  sources say only that such a line cannot occur [forum: Label at the
  beginning/end of macro definition, 22/07/2025]; they do not name the
  stage.
- **MAC-37** Macro state after the error of MAC-36. One message is printed
  for the line (ERR-R16), and then:
  - `L: mcro x` still opens the definition of `x`: the following lines up to
    and including the `mcroend` line are its body and are swallowed, that
    is not copied and not examined. If the rest of the line is a correct
    `mcro` line, `x` is entered into the macro table with that body; if it
    is faulty as well (MAC-38), the rule of MAC-38 applies and no second
    message is printed.
  - `L: mcroend` still closes the open definition; the lines that follow
    are ordinary lines again. Text after `mcroend` on that line gets no
    second message.
  - `L: m`, where `m` is a macro name, is not expanded: no body line is
    copied for it. Text after `m` on that line gets no second message.

  [owner decision, 2026-11-26] (Q-28). The sources are silent. None of this
  can be seen in an `.am` file, because none is written; it decides which
  later lines produce messages.
- **MAC-38** A faulty `mcro` line, that is one with a missing name (ERR-5),
  a name that is not valid (ERR-6) or reserved (ERR-2), extra text after the
  name (ERR-3) or a name that is already in the macro table (ERR-8), is
  reported once, on that line. The following lines up to and including the
  matching `mcroend` line are still treated as the body of a definition and
  are discarded: they produce no further message of any kind and are not
  copied anywhere. The faulty macro is not entered into the macro table;
  for a duplicate name the first definition remains in force. The matching
  `mcroend` line is the first following line whose first field is `mcroend`
  (MAC-20) or that has the form `L: mcroend` (MAC-36). [owner decision,
  2026-11-26] (gap found in review). The sources are silent: MAC-22 only
  requires that the scan goes on, and MAC-7 allows the assumption that the
  `mcroend` line exists. Consequences of "no further message": a discarded
  line longer than 80 characters is not reported (this narrows SYN-2b), and
  neither is extra text or a label on the closing `mcroend` line (ERR-4,
  ERR-9); a later line whose first field is the faulty name is an ordinary
  line, not a call. These two consequences sit uneasily with two source
  rules, MAC-10 (extra characters on the `mcroend` line must be checked,
  [booklet p.25]) and SYN-2a (an over-long line must be reported); the
  decision is kept as given and the tension has been reported to the owner
  (section 1). The file fails in either case, because the `mcro` line itself
  is reported.

### 2.10 Two-pass algorithm (ALG)

- **ALG-1** The first pass identifies the symbols and gives each a numeric
  value (its memory address). The second pass builds the machine code using
  the symbol values, the opcodes and the register numbers. [booklet p.31,
  p.32-33]
- **ALG-2** Code is built to be loaded starting at address 100 (decimal). The
  initial value of IC is 100. [booklet p.31, p.33, p.38, p.42] p.35 says
  "starting at address 0" and "IC ← 0"; the forum rules that the first load
  address is 100. [forum: First Address for Loading into Memory, 08/06/2025]
- **ALG-3** IC is advanced, after each instruction line, by the number of
  words the instruction takes, so it always points at the next free cell.
  [booklet p.33]
- **ALG-4** The assembler keeps two images: the instruction (code) image and
  the data image, each indexed by its own counter, IC and DC. Both counters
  get their initial value when the assembler starts on a source file.
  [booklet p.39]
- **ALG-5** In the output machine code the instructions and the data are two
  separate segments: all instruction words first, then all data words. The
  input file need not keep them separate. [booklet p.34, p.41, p.42]
- **ALG-6** At the end of the first pass, the value of every symbol with the
  attribute "data" is increased by ICF, the final value of IC (equivalently:
  by the total length of the code plus the load address 100), because the data
  image is placed right after the code image. [booklet p.36 step 19, p.41,
  p.42]
- **ALG-7** In the first pass it is always possible to build: the complete
  first word of every instruction, the additional word of an immediate operand
  or a register operand, and the encoding of all data (`.data`, `.string`,
  `.mat`). Words that depend on a symbol are completed in the second pass.
  [booklet p.33, p.36]
- **ALG-8** Both passes read the (expanded) source file from its beginning.
  [booklet p.35]
- **ALG-9** First pass, skeleton algorithm (not mandatory) [booklet p.35-36]:
  1. Initialise IC and DC (the booklet writes `DC ← 0, IC ← 0`; ALG-2 fixes
     IC at 100).
  2. Read the next line. At end of file go to 17.
  3. If the first field is a symbol continue, otherwise go to 5.
  4. Turn on the "symbol definition" flag.
  5. If the line is a data-storage directive (`.data`, `.string` or `.mat`)
     continue, otherwise go to 8.
  6. If there is a symbol definition, enter it into the symbol table with the
     attribute "data" and the value DC. If the symbol is already in the table,
     report an error.
  7. Identify the kind of data, encode it into the data image, and advance DC
     by its length. Go to 2.
  8. If the line is `.extern` or `.entry` continue, otherwise go to 11.
  9. If it is `.entry`, go to 2 (it is handled in the second pass).
  10. If it is `.extern`, enter the symbol that appears as its operand into
      the symbol table with the value 0 and the attribute "external". Go to 2.
  11. The line is an instruction. If there is a symbol definition, enter it
      into the symbol table with the attribute "code" and the value IC. If the
      symbol is already in the table, report an error.
  12. Look up the operation name in the operation table. If it is not found,
      report an error in the instruction name.
  13. Analyse the operands and compute L, the total number of words the
      instruction takes.
  14. Build the binary code of the first word and of every additional word
      that encodes an immediate operand.
  15. Save the values of IC and L together with the machine code of the
      instruction.
  16. `IC ← IC + L`, go to 2.
  17. The file has been read completely. If errors were found in the first
      pass, stop here.
  18. Save the final values of IC and DC as ICF and DCF. They are used to
      build the output files after the second pass.
  19. Add ICF to the value of every symbol with the attribute "data".
  20. Start the second pass.
- **ALG-10** Second pass, skeleton algorithm (not mandatory) [booklet p.36]:
  1. Read the next line. At end of file go to 7.
  2. If the first field is a symbol (label), skip it.
  3. If the line is `.data`, `.string`, `.mat` or `.extern`, go to 1.
  4. If the line is `.entry` continue, otherwise go to 6.
  5. Add the attribute "entry" to the symbol that appears as the operand of
     the directive. If the symbol is not in the symbol table, report an error.
     Go to 1.
  6. Complete the binary encoding of the operand words, according to the
     addressing modes. For every operand that contains a symbol, find the
     symbol's value in the symbol table; if it is not there, report an error.
     If the symbol has the attribute "external", add the address of the
     relevant word to the list of words that refer to an external symbol. The
     values of IC and L saved in the first pass may be used. Go to 1.
  7. The file has been read completely. If errors were found in the second
     pass, stop here.
  8. Build the output files.
- **ALG-11** The assembler processes a line by its type: it ignores empty and
  comment lines; for an instruction it finds the operation and the addressing
  modes, reserves the additional words and advances IC; for a directive it
  acts according to the directive. [booklet p.40-41]
- **ALG-12** The length of the input program is not known in advance. A
  maximum size may be assumed **only** for the arrays that hold the
  machine-code image. Every other structure (symbol table, macro table) must
  be implemented efficiently and economically, for example with a linked list
  and dynamic allocation. [booklet p.46] No maximum number of lines in an
  input file may be assumed. [forum: Input File Line Limit, 14/07/2025]
- **ALG-13** The skeleton algorithms are not mandatory; what is mandatory is
  the required output. In particular, all encoding may be done in the second
  pass. [forum: Two Passes, 30/06/2025; Is it Possible to Encode Only in the
  Second Pass?, 14/07/2025]
- **ALG-14** The data image starts immediately after the last instruction
  word, with no gap. [forum: Data section starting address, 22/07/2025]
- **ALG-15** Memory overflow must be checked: the program (code plus data) may
  not go beyond the 256 memory cells. An overflow is an error; it is reported
  and the assembler goes on looking for further errors. IC and DC
  are what the check uses. [forum: Memory overflow, 14/07/2025; Memory size,
  17/07/2025; Questions Regarding the First Pass, 22/06/2025] [inferred] With
  the load address 100 (ALG-2) and addresses 0..255 (M-4), the limit is
  code words + data words ≤ 156, that is the last used address ≤ 255. The
  line on which the error is reported is fixed by ALG-17.
- **ALG-16** The instructions are only encoded, never executed. No rule
  requires any particular instruction (for example `stop`) to be present in a
  program. [forum: Jumping to a Label, 17/06/2025; Must the stop command
  appear at least once?, 22/07/2025]
- **ALG-17** Memory overflow (ERR-41) is reported by the first pass, once
  per file, on the line whose words first make the code length plus the
  data length exceed the available memory, that is the first instruction or
  data line after which the running total of code words and data words is
  greater than 156. The first pass keeps scanning the remaining lines for
  other errors, but the overflow is not reported again for that file,
  however many further lines add words. The line number is that of ERR-R12,
  like every other first-pass message. [owner decision, 2026-11-26]
  (Q-30 a). The report and the continued scan are required by [forum:
  Memory overflow, 14/07/2025]; the sources do not say on which line, or how
  often, the error is reported.

### 2.11 Errors (ERR-R)

- **ERR-R1** The assembler must detect and report syntax errors of the source
  program, such as: an operation that does not exist, a wrong number of
  operands, an operand type that does not fit the operation, a register name
  that does not exist, "and other errors". It also verifies that each symbol
  is defined exactly once. [booklet p.34]
- **ERR-R2** Error messages are printed to standard output (`stdout`).
  [booklet p.34]
- **ERR-R3** Every error message must state the number of the line in the
  source file in which the error was found. Line numbering starts at 1.
  [booklet p.34] The number may be the line number in the original `.as` file
  or the line number in the expanded `.am` file; both are accepted.
  [forum: Printing Errors with Line Number in Case of Syntax Error in File,
  02/07/2025] This project always reports the `.as` line (ERR-R12, section
  2.14).
- **ERR-R4** The assembler does not stop after the first error. It continues
  over the input to find further errors, if any. It must detect and report all
  erroneous lines of the input. [booklet p.34, p.46]
- **ERR-R5** Messages should be as detailed as possible, so that one can
  understand what each error is and where it is. [booklet p.46]
- **ERR-R6** If the input file has errors, the output files `.ob`, `.ent` and
  `.ext` are not produced for it. [booklet p.34, p.46]
- **ERR-R7** If errors were found in the first pass, the second pass is not
  run. [booklet p.36 step 17]
- **ERR-R8** The booklet does not require the file name in an error message.
  It requires only the line number (ERR-R3) and enough detail to locate the
  error (ERR-R5). [inferred] Since several files can be processed in one run
  (CLI-1), printing the file name is the natural way to satisfy "where each
  error is", but it is not a stated rule of the sources. This project prints
  it (ERR-R13).
- **ERR-R9** One error message per input line is sufficient; every line of
  the file must still be examined. [forum: Error reporting, 22/07/2025] The
  booklet is silent and compatible (p.34, p.46). This project prints exactly
  one (ERR-R16).
- **ERR-R10** On a memory allocation failure the assembler reports an error,
  releases all allocated memory, and the entire program exits (it does not
  move on to the next file). [forum: Linked List for Macros, 17/06/2025;
  Memory Allocation Errors, 14/07/2025; Memory allocation error, 22/07/2025]
  The booklet is silent.
- **ERR-R11** Every file opened while processing a source file is closed when
  the processing of that file ends. [forum: A Few Questions That Came Up,
  22/07/2025]
- **ERR-R12** The line number in every message, from every stage, is the
  number of the line in the original `.as` file, counted from 1. The first
  and second pass obtain it from the line map of MAC-35. For a line that
  comes from a macro body, the number reported is that of the body line
  inside the macro definition, which is the place the author has to fix. A
  macro that is called N times can therefore produce the same message, with
  the same line number, N times; this repetition is accepted. [owner
  decision, 2026-11-26] (choice C-1), allowed by [forum: Printing Errors with
  Line Number in Case of Syntax Error in File, 02/07/2025]; the repetition is
  accepted by [booklet p.34] (MAC-13).
- **ERR-R13** Message format. Every error is one line on `stdout` of the form
  `<name>.as:<line>: error: <text>`, where `<name>` is the command-line
  argument exactly as given (CLI-8) and `<line>` is the decimal line number
  of ERR-R12 with no padding. There is no space before the line number, and
  exactly one space after the colon that follows the line number and after
  the colon that follows `error`. An error that has no line (for example a
  file that cannot be opened) has the form `<name>.as: error: <text>`. The
  `<text>` strings are not fixed by this file; they must be informative
  (ERR-R5), contain no newline, and may change without a change to SPEC.
  Tests compare the file name, the line number and the severity of each
  message, never its wording. [owner decision, 2026-11-26] (Q-3). The
  sources fix only `stdout` and the line number [booklet p.34] and give no
  format.
- **ERR-R14** A warning has the same format as an error with the word
  `warning:` in place of `error:`. A warning is not an error: it does not
  suppress the output files and does not change the exit status (CLI-9).
  [owner decision, 2026-11-26] (Q-3). The assembler prints exactly one kind
  of warning, WRN-1 (DIR-35, section 5.6).
- **ERR-R15** `stdout` carries only diagnostics. Nothing is printed for a
  file that assembles without errors and without warnings: no success
  notice, no progress text, no file name. Every line the assembler writes
  to `stdout` is an error line (ERR-R13), a warning line (ERR-R14) or the
  usage line of CLI-10. [owner decision, 2026-11-26] (Q-26). The sources
  are silent on what is printed when there is no error.
- **ERR-R16** Exactly one message is printed for a line that has one or more
  errors: the first error found on that line. Once an error has been found
  the rest of the line is not examined further. Which of several errors is
  "the first found" is not fixed by this file (the wording of messages is
  free, ERR-R13); what is fixed is that the line yields one message. "Line"
  means a line as a stage reads it: a line of the `.as` file for the
  pre-assembler, a line of the `.am` file for the passes, so a macro body
  line with an error still yields one message per call (ERR-R12). A warning
  and an error on the same line do not arise by construction; if they
  would, only the error is printed. [owner decision, 2026-11-26] (Q-29),
  the minimum allowed by "one error per line is sufficient" [forum: Error
  reporting, 22/07/2025].
- **ERR-R17** Order of the messages on `stdout`. Files are reported in
  command-line order; the message for a file that cannot be opened (ERR-1)
  stands at the place of that file. Within a file the messages are either
  all pre-assembler messages, or else all first-pass messages followed by
  all second-pass messages (the second-pass group is empty when the first
  pass found an error, ERR-R7). Within a stage the messages appear in the
  order in which the stage reads the lines: the order of the `.as` file for
  the pre-assembler, and the order of the `.am` file for the passes. The
  line numbers printed by the passes are `.as` line numbers (ERR-R12), so
  they are not always ascending: a macro body line repeats its number at
  every call, later in the output. Warnings are ordered like errors, as
  first-pass messages. [owner decision, 2026-11-26] (Q-29). The sources are
  silent on the order.

### 2.12 Command line (CLI) and output files (OUT)

- **CLI-1** The assembler receives, as command-line arguments, a list of one
  or more source file names. [booklet p.39]
- **CLI-2** Source files must have the suffix `.as`. The names are passed
  **without** the suffix. Example: `assembler x y hello` runs the assembler on
  `x.as`, `y.as` and `hello.as`. [booklet p.39]
- **CLI-3** Each source file is processed separately and gets its own output
  files. [booklet p.39]
- **CLI-4** The interface is the command line only. No interactive input, no
  menus, no windows. The invocation format must be exactly as specified.
  [booklet p.46]
- **CLI-5** Running with no file arguments can happen and is not an error; the
  assembler may print a notice. [forum: Number of Input Files, 14/07/2025]
  This project prints a usage line (CLI-10).
- **CLI-6** The argument is the file name without the extension. The assembler
  tries to open the name with `.as` appended and reports an error if the file
  does not open; this must be checked, not assumed. [forum: Input File Name,
  22/06/2025]
- **CLI-7** There is no limit on the number of input files. [forum: Limit on
  Number of Input Files, 14/07/2025]
- **CLI-8** A command-line argument is always taken as a name without the
  extension; the assembler never strips or recognises a suffix. The argument
  `x.as` therefore makes the assembler try to open `x.as.as` and, when that
  file does not exist, report that it cannot be opened (ERR-1), in the form
  `x.as.as: error: <text>` (ERR-R13 with `<name>` = `x.as`). [owner decision,
  2026-11-26] (Q-16), following "the input is the file name without the
  extension; you can try to open a file with the standard extension and
  report an error if the file does not open" [forum: Input File Name,
  22/06/2025].
- **CLI-9** Exit status. The program exits with status 0 if and only if every
  file named on the command line was assembled without errors; this includes
  a run with no file arguments (CLI-5, CLI-10). In every other case, that is
  when at least one file could not be opened (CLI-11) or had an error in any
  stage, or when
  the program stopped on an allocation failure (ERR-R10), the status is 1.
  Warnings do not affect the status. [owner decision, 2026-11-26] (Q-16).
  The sources are silent on the exit status.
- **CLI-10** When the program is run with no arguments it prints exactly one
  usage line to `stdout` and exits with status 0. The wording is free, for
  example `usage: assembler file1 [file2 ...]`; the line must not have the
  form of an error or warning line (ERR-R13, ERR-R14). Nothing else is
  printed and no file is created. [owner decision, 2026-11-26] (Q-26),
  allowed by "this is not an error, you can report" [forum: Number of Input
  Files, 14/07/2025].
- **CLI-11** A file that cannot be opened is reported (ERR-1) and the
  assembler continues with the next argument; the files after it are
  processed normally. The exit status of the run is 1 (CLI-9). [owner
  decision, 2026-11-26] (Q-33). The sources say only "report an error if the
  file does not open" [forum: Input File Name, 22/06/2025]; continuing
  matches CLI-3 and MAC-11.
- **CLI-12** A source file that opens but cannot be read (the name refers to
  a directory, or a read fails part of the way through) is treated like a
  file that cannot be opened: one message without a line number in the form
  of ERR-R13 (ERR-47), no `.am`, `.ob`, `.ent` or `.ext` file is left for
  it, the assembler continues with the next argument and the exit status is
  1. The same holds if reading the `.am` file back in a pass fails. A read
  error is never taken for the end of the file. [project decision, 2026-11-26, pending the owner's confirmation]
  Follows the intent of CLI-6 and CLI-11. Found by the independent
  correctness review of 2026-11-26 (finding D-2).
- **OUT-1** Output file names are the input name as given on the command line
  plus a suffix: `.am` (after macro expansion), `.ob` (object), `.ent`
  (entries), `.ext` (externals). [booklet p.39, p.46]
- **OUT-2** The `.ext` file is not created if the source has no `.extern`
  directive. The `.ent` file is not created if the source has no `.entry`
  directive. An `.ent` or `.ext` file that would remain empty must not be
  created. [booklet p.39, p.45-46]
- **OUT-3** The `.ob` file holds the memory image in two parts: the
  instruction image first, immediately followed by the data image.
  [booklet p.42]
- **OUT-4** The `.ob` file is made of text lines. The first line has two
  numbers: the total length of the instruction segment in words, and the total
  length of the data segment in words. Every following line has two numbers:
  the address of a memory word, and the content of that word. All numbers in
  the file are in unique base 4 (ENC-13). [booklet p.42, p.45]
- **OUT-5** Field widths in `.ob`. A word is five base-4 digits with leading
  `a` [booklet p.19]. In the printed example an address is four base-4 digits
  (`bcba` = 100, `caca` = 136), and the two numbers of the first line are
  printed with no leading `a` (`bbc dd` = 22 and 15). [inferred] from the
  example on p.45; the booklet has no sentence about the width of addresses or
  of the header numbers. Asked about alignment, separators and the first line,
  the moderator answered only that the example in the booklet shows the
  format. [forum: Output File Format, 08/06/2025] So the example is the
  authority for the widths; the separator characters cannot be read from a
  PDF, and OUT-13 fixes them. (The same widths, with the reasoning "the
  header is just a number, an address needs 8 bits, a word 10 bits", appear
  in student posts, not rulings.)
- **OUT-6** The column heading "Base 4 address / Base 4 code" printed above
  the example is not part of the file. [booklet p.45]
- **OUT-7** Lines of `.ob` after the first are in ascending address order,
  starting at 100, code words then data words. [booklet p.42, p.45]
- **OUT-8** The `.ent` file is made of text lines. Each line has the name of a
  symbol declared as entry and its value as found in the symbol table (after
  ALG-6), in unique base 4. Example (the gap is one TAB, OUT-13): `LOOP bccd`,
  `LENGTH caab`.
  [booklet p.42, p.45] Each entry symbol appears once, however many `.entry`
  directives name it (DIR-34). The value is written as an address, four
  base-4 digits (OUT-13).
- **OUT-9** The `.ext` file is made of text lines. Each line has the name of a
  symbol declared external and the address, in unique base 4, of a machine
  word that encodes an operand referring to that symbol. Every reference gets
  its own line, so a symbol used several times appears on several lines.
  Example (the gap is one TAB, OUT-13): `W bcbd`, `W bcda`, `L3 bdca`.
  [booklet p.39, p.42, p.45]
- **OUT-10** The order of the lines in `.ent` and `.ext` does not matter.
  [booklet p.46]
- **OUT-11** The `.am` file is produced by the pre-assembler (MAC-14). It is
  not in the list of files that are withheld when the passes find errors
  (ERR-R6 lists only ob, ext, ent), so it stays when the first or second pass
  fails. [booklet p.46] [forum: If There is an Error in the First Pass, Should
  the .am File be Deleted or Left Open?, 25/06/2025] It is not created when
  the pre-assembler itself fails (MAC-12, MAC-23). [booklet p.30]
- **OUT-12** `.ent` and `.ext` files left over from an earlier run need not be
  deleted when the current run has nothing to write to them; they are simply
  not recreated. [forum: Deleting previous .ent and .ext files, 17/07/2025]
  This project deletes them all the same (OUT-14).
- **OUT-13** Exact layout of `.ob`, `.ent` and `.ext`. Every line has exactly
  two fields separated by exactly one TAB character (ASCII 9). There is no
  indentation, no trailing whitespace and no blank line, and every line,
  including the last, ends with one newline character (ASCII 10). The first
  line of `.ob` is `<code length><TAB><data length>`, each in unique base 4
  with no leading `a`; a length of zero is printed as the single digit `a`.
  Every other line of `.ob` is `<address><TAB><word>`: the address as exactly
  four base-4 digits and the word as exactly five (OUT-5). A line of `.ent`
  is `<symbol><TAB><address>` and a line of `.ext` is
  `<symbol><TAB><address>`, the address as exactly four base-4 digits.
  [owner decision, 2026-11-26] (Q-4). The widths are those of the example
  [booklet p.45], to which the moderator refers [forum: Output File Format,
  08/06/2025]; the sources do not define the separator, the indentation of
  the first line as printed on p.45, or the form of a zero length.
  [inferred] A file with `.extern` directives but no reference to any
  external symbol gets no `.ext` file, because it would be empty (OUT-2).
- **OUT-14** Before processing a file, the assembler removes any existing
  `<name>.am`, `<name>.ob`, `<name>.ent` and `<name>.ext`, where `<name>` is
  the command-line argument (OUT-1), so that after the run exactly the files
  produced by this run exist for that name. The source file `<name>.as` is
  never removed. [owner decision, 2026-11-26] (Q-31). This goes beyond the
  forum ruling recorded in OUT-12, which only says that deleting stale
  `.ent` and `.ext` files is not required [forum: Deleting previous .ent and
  .ext files, 17/07/2025]; it does not contradict it, and it does not touch
  OUT-11, which is about the `.am` file written by the current run.
  [inferred] The removal is done for every argument, before the attempt to
  open `<name>.as`, so it also happens for a file that cannot be opened
  (ERR-1): otherwise files that this run did not produce would remain.
- **OUT-15** A source file with no statements (an empty file, or a file
  that holds only empty lines and comment lines) assembles successfully.
  It produces an `.am` file (MAC-29; empty for an empty source) and an `.ob`
  file whose only line is `a<TAB>a`, that is the header of OUT-13 with both
  lengths zero, and no `.ent` or `.ext` file (OUT-2). Nothing is printed
  (ERR-R15) and the file counts as assembled without errors (CLI-9).
  [owner decision, 2026-11-26] (Q-32 c). The sources are silent on an empty
  program.

### 2.13 Rulings first reported by the owner, verified against the forum

The owner reported seven rulings from memory before the forum text was
available. Each was then looked up in `docs/forum.txt`.

| Reported ruling | Result | Citation | Rule | Booklet |
|---|---|---|---|---|
| Matrix brackets must be attached to the label with no whitespace. | Found, with a refinement: whitespace inside the brackets is allowed. | [forum: Spaces within square brackets in a matrix, 22/07/2025] | SYN-18, DIR-27 | Silent. |
| Whitespace is required between a directive name and its operand. | Found. | [forum: Spaces in directives, 22/07/2025] | SYN-17 | Silent for directives; explicit for instructions (p.28). |
| `.mat` symbols are "data" symbols. | Found. | [forum: Symbol characterization in the symbol table, 22/07/2025] | DIR-16 | Agrees (p.35 steps 5-6, p.41); the p.37 table is wrong. |
| One error message per input line is enough. | Found. | [forum: Error reporting, 22/07/2025] | ERR-R9 | Silent, compatible. |
| On memory allocation failure, free everything and exit. | Found, three times; the whole program exits, not only the current file. | [forum: Linked List for Macros, 17/06/2025; Memory Allocation Errors, 14/07/2025; Memory allocation error, 22/07/2025] | ERR-R10 | Silent. |
| No need to check for duplicate macro names. | Found, but three other moderator posts require or allow the report. The owner decided to report an error, which satisfies all four. | [forum: Macro name, 22/07/2025] against [forum: Do We Need to Check Multiple Definitions of the Same Macro?, 08/06/2025; Whitespace in Macros, 18/06/2025; Error Reporting, 25/06/2025] | MAC-15, MAC-28 | Silent. |
| A label in front of `.entry` or `.extern` is ignored (a warning is allowed). | **Not found in forum** as a ruling. The only related moderator post refers the asker back to the booklet. The rule itself stands on the booklet. | [forum: Questions I didn't find answers to in the booklet., 22/07/2025] (reference to the booklet only) | DIR-18, DIR-22; this project prints the warning (DIR-35) | States it (p.27, p.28). |

### 2.14 Which file does the line number in an error message refer to?

**Answer for this project: always the original `.as` file (ERR-R12).**
[owner decision, 2026-11-26] (choice C-1)

What the sources allow: either. The forum settles that the line number may be
the one in the original `.as` file or the one in the expanded `.am` file.
[forum: Printing Errors with Line Number in Case of Syntax Error in File,
02/07/2025]

The ruling was given to a student whose first pass reads a `.am` file from
which macro definitions, empty lines and comment lines had been removed, and
who asked whether the numbers must be mapped back to the source file. A later
thread asking the same question was answered only with "an answer exists in
the forum". [forum: Error Line Number in Assembler, 14/07/2025]

Consequences:

- An error found by the first or second pass reports the `.as` line number.
  Macro expansion moves lines, so the passes cannot count lines of the `.am`
  file; they use the line map that the pre-assembler builds (MAC-35).
- An error on a line that comes from a macro body reports the `.as` line of
  that body line inside the macro definition. If the macro is called several
  times, the message is printed once per call, each time with the same line
  number (ERR-R12; the booklet accepts the repetition, p.34).
- An error found by the pre-assembler can only refer to the `.as` file,
  because no `.am` file exists for a file that fails there (MAC-23).
  [inferred]
- The same holds for the one warning (WRN-1, DIR-35) and for the memory
  overflow error, which has a line since ALG-17. The only messages without
  a line number are ERR-1 and the allocation failure of ERR-R10.
- A line yields at most one message each time a stage reads it (ERR-R16).
  The messages of a stage are printed in the order the lines are read, which
  for the passes is the order of the `.am` file (ERR-R17); the `.as` numbers
  printed are therefore not always ascending.
- Tests assert the `.as` line number of every message.

Worked example. In the `.as` file of section 4.1 the macro body line
`mov M1[r3][r3],r3` is line 6 and the call `a_mc` is line 12. In the `.am`
file the same text is line 8. An error on that line would be reported as
`<name>.as:6: error: ...`; with a second call of `a_mc` it would be printed
twice, both times with line 6. The line `END: stop` is line 13 of the `.as`
file and line 10 of the `.am` file; an error there is reported with line 13.

For the record, the booklet alone is ambiguous. The sentences that bear on it
(translated):

1. p.34: "The assembler prints the error messages to the standard output
   stdout. In every error message the number of the line in the source file in
   which the error was identified must also be stated (the line count in the
   file starts at 1)."
2. p.25: "The 'expanded source file' is a 'source file' after macro expansion,
   as opposed to the 'initial source file', which is the input file to the
   system, including the macro definitions."
3. p.25: "The program after macro expansion is the program the assembler is to
   translate."
4. p.34: "Note that when the assembler checks for errors, it can no longer be
   identified that this is code expanded from a macro, so the duplicated error
   detection cannot be avoided."
5. p.35: "In each pass we start reading the source file from the beginning."

Sentence 1 says "the source file"; sentence 2 uses that term for both files;
sentences 3 to 5 point to the expanded file. No booklet sentence names `.as`
or `.am` in connection with the line number.

### 2.15 Choices the forum leaves to the implementer, and non-behavioural rulings

Choices ("at your discretion"). None of these is an open question about the
sources: the sources allow every option listed. C-1 to C-5 change what a test
can observe, and the owner decided each of them on 2026-11-26; the chosen
option is binding. C-6 to C-9 cannot be observed by a test (ERR-R13 leaves
the wording of messages free) and stay implementation choices.

| ID | Choice | Options allowed | Source | Chosen [owner decision, 2026-11-26] | Rule |
|---|---|---|---|---|---|
| C-1 | Line number base in pass error messages. | `.as` line or `.am` line. | [forum: Printing Errors with Line Number in Case of Syntax Error in File, 02/07/2025] | **The `.as` line**, through a line map from the pre-assembler; a macro body line reports its line inside the definition. | ERR-R12, MAC-35, section 2.14 |
| C-2 | Stage that checks the 80-character limit. | Pre-assembler or first pass. | [forum: Character Overflow Error, 09/06/2025; Checking Line Length Validity, 22/06/2025] | **The pre-assembler**; it is a pre-assembler error, so no `.am` and no passes. | SYN-2b, ERR-10 |
| C-3 | Empty lines and comment lines in `.am`. | Keep them or delete them. | [forum: Handling Empty Lines and Comment Lines, 09/06/2025; Should the pre-assembler also copy empty lines or can it ignore and delete them?, 17/07/2025] | **Keep them**, exactly as they are. | MAC-29 |
| C-4 | Extra text after a macro name on a call line. | Report in the pre-assembler (called "more logical" by the moderator) or leave the line for the first pass. | [forum: Macro Mention with Additional Text, 14/07/2025] | **Error reported by the pre-assembler.** | MAC-30, ERR-7 |
| C-5 | After reporting extra text on a `mcroend` line: close the macro or ignore the line. | Either. | [forum: Redundant Text After Command, 14/07/2025] | **Report the error and close the macro** at that line. | MAC-31, ERR-4 |
| C-6 | Where the encoding is done. | First and second pass as in the skeleton, or all in the second pass. | [forum: Is it Possible to Encode Only in the Second Pass?, 14/07/2025] | Not fixed; not observable. | ALG-13 |
| C-7 | Wording for an undefined `rNN` operand. | "Undefined label" or "register out of range". | [forum: How to Treat an Operand that is a Register and Not in Range?, 14/07/2025] | Not fixed; wording is free. | LBL-16, ERR-R13 |
| C-8 | Granularity of pre-assembler messages. | A general "invalid macro name" is enough. | [forum: Error Reporting, 25/06/2025] | Not fixed; wording is free. | MAC-26, ERR-R13 |
| C-9 | Internal storage of the tables. | Implementer's choice; any temporary file must be deleted on error and at the end. | [forum: Macro Tables / Labels, 01/07/2025] | Not fixed; bound only by the coding rules in `CLAUDE.md`. | ALG-12 |

Rulings about the code rather than the behaviour (recorded because they touch
the project's coding rules):

- **IMP-1** Arrays must not be defined as global variables; this answer was
  given to a question about a read-only (`const`) table of the instructions.
  Individual global variables are allowed. [forum: Defining Global Variables
  for Arrays, 14/07/2025; Variables, 30/06/2025] For this project: the
  instruction table is a `static const` array, private to the
  instruction-table module (defined in its `.c` file) and reachable from other
  modules only through access functions. No array, constant or not, has
  external linkage anywhere in the program. The sentence "const tables are
  allowed" in the coding rules of `CLAUDE.md` is to be read in this way: a
  const table is allowed as a `static` object inside one module, not as a
  global array. [owner decision, 2026-11-26] (Q-25), which satisfies both the
  moderator's ruling and `CLAUDE.md`. This rule has no effect on the
  assembler's output.
- **IMP-2** Dynamic allocation is recommended over a fixed 156-word array even
  for the code image. [forum: Memory overflow, 17/07/2025] (The booklet, p.46,
  allows a fixed maximum size for the machine-code image; ALG-12.)
- **IMP-3** The file that holds `main`, and the project folder, are named
  `assembler`. [forum: File Names, 17/06/2025]
- **IMP-4** The course submission had to include the examples from the booklet
  and at least three more examples. [forum: General Documentation for
  Functions + Test Files, 08/06/2025]

---

## 3. Instruction table

Source: [booklet p.35] as mandated by [correction 1]; operand counts from
[booklet p.22-24]. Modes: 0 immediate, 1 direct, 2 matrix, 3 register.

| Name | Opcode | Operands | Legal source modes | Legal destination modes |
|---|---|---|---|---|
| mov | 0 | 2 | 0,1,2,3 | 1,2,3 |
| cmp | 1 | 2 | 0,1,2,3 | 0,1,2,3 |
| add | 2 | 2 | 0,1,2,3 | 1,2,3 |
| sub | 3 | 2 | 0,1,2,3 | 1,2,3 |
| lea | 4 | 2 | 1,2 | 1,2,3 |
| clr | 5 | 1 | none | 1,2,3 |
| not | 6 | 1 | none | 1,2,3 |
| inc | 7 | 1 | none | 1,2,3 |
| dec | 8 | 1 | none | 1,2,3 |
| jmp | 9 | 1 | none | 1,2,3 |
| bne | 10 | 1 | none | 1,2,3 |
| jsr | 11 | 1 | none | 1,2,3 |
| red | 12 | 1 | none | 1,2,3 |
| prn | 13 | 1 | none | 0,1,2,3 |
| rts | 14 | 0 | none | none |
| stop | 15 | 0 | none | none |

---

## 4. Worked examples, corrected

All words below were derived independently from the rules of section 2 (with
a throw-away script, not with the project code) and then compared, word by
word, with the booklet tables on p.31-32, p.38, p.43-44 and the files on p.45.

**Result of the comparison: after applying corrections 1 and 3, the derivation
and the booklet agree on every word, every address, every symbol value and
every `.ent` / `.ext` line. No unexplained disagreement was found.**

The `.ob`, `.ent` and `.ext` listings below are byte-exact (OUT-13): the two
fields of every line are separated by one real TAB character (ASCII 9), which
is written as a TAB inside the code blocks of this file, not as spaces. An
editor or a viewer may display it as a run of spaces; anyone who copies these
listings into expected-output files must check that the TAB survived. The
`.as` and `.am` listings are indented with spaces, as transcribed from the
booklet.

### 4.1 Example 1 (booklet p.30-32, p.36-38)

Input (`.as`), p.30:

```
MAIN:     mov   M1[r2][r7],LENGTH
          add   r2,STR
LOOP:     jmp   END
          prn   #-5
          mcro  a_mc
          mov   M1[r3][r3],r3
          bne   LOOP
          mcroend
          sub   r1, r4
          inc   K

          a_mc
END:      stop
STR:      .string "abcdef"
LENGTH:   .data  6,-9,15
K:        .data  22
M1:       .mat  [2][2]  1,2,3,4
```

After macro expansion (`.am`), p.30. The empty line of the source is kept, as
MAC-29 requires and as the booklet prints it:

```
MAIN:     mov   M1[r2][r7],LENGTH
          add   r2,STR
LOOP:     jmp   END
          prn   #-5
          sub   r1, r4
          inc   K

          mov   M1[r3][r3],r3
          bne   LOOP
END:      stop
STR:      .string "abcdef"
LENGTH:   .data  6,-9,15
K:        .data  22
M1:       .mat  [2][2]  1,2,3,4
```

Symbol table after the first pass (p.32, p.37): MAIN 100 code, LOOP 107 code,
END 121 code, STR 122 data, LENGTH 129 data, K 132 data, M1 133 data.
ICF = 122, code length 22 words, data length 15 words.

Encoding (binary shown with the booklet's field separators):

| Addr | Statement / word | Binary | Base 4 |
|---|---|---|---|
| 100 | `mov M1[r2][r7],LENGTH` | 0000-10-01-00 | aacba |
| 101 | address of M1 (133) | 10000101-10 | cabbc |
| 102 | index registers r2, r7 | 0010-0111-00 | acbda |
| 103 | address of LENGTH (129) | 10000001-10 | caabc |
| 104 | `add r2,STR` | 0010-11-01-00 | acdba |
| 105 | source register r2 | 0010-0000-00 | acaaa |
| 106 | address of STR (122) | 01111010-10 | bdccc |
| 107 | `jmp END` | 1001-00-01-00 | cbaba |
| 108 | address of END (121) | 01111001-10 | bdcbc |
| 109 | `prn #-5` | **1101-00-00-00** | **dbaaa** |
| 110 | the number -5 | 11111011-00 | ddcda |
| 111 | `sub r1,r4` | 0011-11-11-00 | addda |
| 112 | registers r1, r4 | 0001-0100-00 | abbaa |
| 113 | `inc K` | 0111-00-01-00 | bdaba |
| 114 | address of K (132) | 10000100-10 | cabac |
| 115 | `mov M1[r3][r3],r3` | 0000-10-11-00 | aacda |
| 116 | address of M1 (133) | 10000101-10 | cabbc |
| 117 | index registers r3, r3 | 0011-0011-00 | adada |
| 118 | destination register r3 | 0000-0011-00 | aaada |
| 119 | `bne LOOP` | 1010-00-01-00 | ccaba |
| 120 | address of LOOP (107) | 01101011-10 | bccdc |
| 121 | `stop` | 1111-00-00-00 | ddaaa |
| 122 | 'a' | 0001100001 | abcab |
| 123 | 'b' | 0001100010 | abcac |
| 124 | 'c' | 0001100011 | abcad |
| 125 | 'd' | 0001100100 | abcba |
| 126 | 'e' | 0001100101 | abcbb |
| 127 | 'f' | 0001100110 | abcbc |
| 128 | string terminator | 0000000000 | aaaaa |
| 129 | 6 | 0000000110 | aaabc |
| 130 | -9 | 1111110111 | dddbd |
| 131 | 15 | 0000001111 | aaadd |
| 132 | 22 | 0000010110 | aabbc |
| 133 | 1 | 0000000001 | aaaab |
| 134 | 2 | 0000000010 | aaaac |
| 135 | 3 | 0000000011 | aaaad |
| 136 | 4 | 0000000100 | aaaba |

Expected `.ob` (the booklet prints no `.ob` for this example; this is derived
from the table above and OUT-4, OUT-5):

```
bbc	dd
bcba	aacba
bcbb	cabbc
bcbc	acbda
bcbd	caabc
bcca	acdba
bccb	acaaa
bccc	bdccc
bccd	cbaba
bcda	bdcbc
bcdb	dbaaa
bcdc	ddcda
bcdd	addda
bdaa	abbaa
bdab	bdaba
bdac	cabac
bdad	aacda
bdba	cabbc
bdbb	adada
bdbc	aaada
bdbd	ccaba
bdca	bccdc
bdcb	ddaaa
bdcc	abcab
bdcd	abcac
bdda	abcad
bddb	abcba
bddc	abcbb
bddd	abcbc
caaa	aaaaa
caab	aaabc
caac	dddbd
caad	aaadd
caba	aabbc
cabb	aaaab
cabc	aaaac
cabd	aaaad
caca	aaaba
```

Expected `.ent`: not created (no `.entry`). Expected `.ext`: not created (no
`.extern`). [OUT-2]

### 4.2 Example 2, `ps.as` (booklet p.43-45)

Program after macro expansion (`ps.am`; the booklet shows only this form):

```
; file ps.as

.entry LOOP
.entry LENGTH
.extern L3
.extern W
MAIN:     mov   M1[r2][r7],W
          add   r2,STR
LOOP:     jmp   W
          prn   #-5
          sub   r1, r4
          inc   K

          mov   M1[r3][r3],r3
          bne   L3
END:      stop
STR:      .string "abcdef"
LENGTH:   .data  6,-9,15
K:        .data  22
M1:       .mat [2][2]  1,2,3,4
```

Symbols: MAIN 100 code, LOOP 107 code + entry, END 121 code, STR 122 data,
LENGTH 129 data + entry, K 132 data, M1 133 data, L3 0 external, W 0 external.
Code length 22, data length 15.

Code words (data words 122 to 136 are identical to example 1):

| Addr | Statement / word | Binary | Base 4 |
|---|---|---|---|
| 100 | `mov M1[r2][r7],W` | 0000-10-01-00 | aacba |
| 101 | address of M1 (133) | 10000101-10 | cabbc |
| 102 | index registers r2, r7 | 0010-0111-00 | acbda |
| 103 | W (external) | 00000000-01 | aaaab |
| 104 | `add r2,STR` | 0010-11-01-00 | acdba |
| 105 | source register r2 | 0010-0000-00 | acaaa |
| 106 | address of STR (122) | 01111010-10 | bdccc |
| 107 | `jmp W` | 1001-00-01-00 | cbaba |
| 108 | W (external) | 00000000-01 | aaaab |
| 109 | `prn #-5` | **1101-00-00-00** | **dbaaa** |
| 110 | the number -5 | 11111011-00 | ddcda |
| 111 | `sub r1,r4` | 0011-11-11-00 | addda |
| 112 | registers r1, r4 | 0001-0100-00 | abbaa |
| 113 | `inc K` | 0111-00-01-00 | bdaba |
| 114 | address of K (132) | 10000100-10 | cabac |
| 115 | `mov M1[r3][r3],r3` | 0000-10-11-00 | aacda |
| 116 | address of M1 (133) | 10000101-10 | cabbc |
| 117 | index registers r3, r3 | 0011-0011-00 | adada |
| 118 | destination register r3 | 0000-0011-00 | aaada |
| 119 | `bne L3` | 1010-00-01-00 | ccaba |
| 120 | L3 (external) | **00000000-01** | **aaaab** |
| 121 | `stop` | 1111-00-00-00 | ddaaa |

Expected `ps.ob`:

```
bbc	dd
bcba	aacba
bcbb	cabbc
bcbc	acbda
bcbd	aaaab
bcca	acdba
bccb	acaaa
bccc	bdccc
bccd	cbaba
bcda	aaaab
bcdb	dbaaa
bcdc	ddcda
bcdd	addda
bdaa	abbaa
bdab	bdaba
bdac	cabac
bdad	aacda
bdba	cabbc
bdbb	adada
bdbc	aaada
bdbd	ccaba
bdca	aaaab
bdcb	ddaaa
bdcc	abcab
bdcd	abcac
bdda	abcad
bddb	abcba
bddc	abcbb
bddd	abcbc
caaa	aaaaa
caab	aaabc
caac	dddbd
caad	aaadd
caba	aabbc
cabb	aaaab
cabc	aaaac
cabd	aaaad
caca	aaaba
```

Expected `ps.ent` (line order is free, OUT-10):

```
LOOP	bccd
LENGTH	caab
```

Expected `ps.ext` (line order is free, OUT-10):

```
W	bcbd
W	bcda
L3	bdca
```

### 4.3 Words that differ from the printed booklet

Three distinct words change: one in example 1 and two in example 2.

| Example | Address | Statement | Booklet value | Corrected value | Cause | Printed where |
|---|---|---|---|---|---|---|
| 1 | 109 (`bcdb`) | `prn #-5`, first word | 1100-00-00-00 (`daaaa`) | 1101-00-00-00 (`dbaaa`) | correction 1 (prn is 13, not 12) | tables on p.31, p.37, p.38 |
| 2 | 109 (`bcdb`) | `prn #-5`, first word | 1100-00-00-00 (`daaaa`) | 1101-00-00-00 (`dbaaa`) | correction 1 | table on p.43; `ps.ob` on p.45 prints `daaaa` |
| 2 | 120 (`bdca`) | L3, external | 00000000-10 (`aaaac`) | 00000000-01 (`aaaab`) | correction 3 | table on p.43 only; `ps.ob` on p.45 already prints `aaaab` |

Notes:

- The change at address 109 is an **inferred consequence** of correction 1.
  The reasoning: the corrections sheet (also posted by the moderator,
  [forum: Clarifications and Corrections, 14/07/2025]) declares the p.35 table
  (prn = 13) the correct one, and the examples were evidently produced with
  the p.20 table (prn = 12). In that forum thread a student asked whether the
  example tables and the `.ob` file are correct with respect to the prn
  opcode, and another student replied that the tenth line of the object file
  should read `bcdb dbaaa` instead of `bcdb daaaa`. **Both are student posts;
  the moderator did not reply**, so the forum contains no ruling on this line.
  The value `dbaaa` rests on correction 1 plus the derivation above, and it
  agrees with the student's statement. The owner treats `dbaaa` as settled.
  [owner decision, 2026-11-26] (Q-24)
- `prn` is the only operation in the examples whose opcode differs between the
  two tables. mov, add, sub, inc, jmp, bne and stop have the same opcode in
  both, so no other first word changes.
- Correction 4 changes **no** word of the examples. They already place the
  source register in bits 9-6 (address 105: `0010-0000-00`; address 112:
  `0001-0100-00`) and the destination register in bits 5-2 (address 118:
  `0000-0011-00`).
- Correction 2 changes no word; the examples are already in unique base 4.
- The p.37 table (first pass state, with `?` for unresolved words) carries the
  same wrong word at address 109.

---

## 5. Error categories

Each condition is something the assembler must detect and report in the
format of ERR-R13, with the `.as` line number of ERR-R12. "Stage" names the
stage that reports it; the stage is binding, because it decides which files
exist afterwards and which other errors are reported in the same run.
"Source" names the rule and its origin. Conditions marked [inferred] are not
listed by the booklet in so many words; they follow from a stated syntax rule
together with the general duty to report syntax errors ("and other errors",
p.34; "detect and report all erroneous lines", p.46). Conditions that exist
because of an owner decision are marked [owner decision, 2026-11-26]; the
rule cited next to the mark gives the source that allows the decision.

ERR IDs are stable. ERR-10 keeps its number although it moved from the first
pass to the pre-assembler, and ERR-53 keeps its number although it is listed
with the first pass (5.3) and no longer with the second.

Rules that apply to every table below: a line yields exactly one message,
the first error found on it (ERR-R16); the messages appear in the order of
ERR-R17; and nothing but diagnostics is written to `stdout` (ERR-R15).

### 5.1 Command line

| ID | Condition | Stage | Source |
|---|---|---|---|
| ERR-1 | A source file named on the command line cannot be opened. The name tried is the argument with `.as` appended, so the argument `x.as` fails on `x.as.as`. The message has no line number: `<name>.as: error: <text>`. | Before the pre-assembler, per file | CLI-6 [forum: Input File Name, 22/06/2025]; CLI-8 [owner decision, 2026-11-26] |

Effect: nothing is written for that file, output files of the same name left
from an earlier run are removed (OUT-14), the assembler continues with the
next argument (CLI-11) and the exit status is 1 (CLI-9). The message stands
at the place of that file in the output (ERR-R17).

Not an error: a run with no arguments prints one usage line and exits with
status 0 (CLI-10).

### 5.2 Pre-assembler

| ID | Condition | Stage | Source |
|---|---|---|---|
| ERR-2 | The macro name is a reserved word: an operation name, a directive name without the dot, a register name `r0` to `r7`, `mcro` or `mcroend`. | Pre-assembler | MAC-9 [booklet p.25]; MAC-33, LBL-20 [owner decision, 2026-11-26] |
| ERR-3 | Extra non-whitespace characters after the macro name on the `mcro` line. | Pre-assembler | MAC-10 [booklet p.25]; MAC-19 |
| ERR-4 | Extra non-whitespace characters after the word `mcroend`. The definition is closed at that line all the same. | Pre-assembler | MAC-10 [booklet p.25]; MAC-31 [owner decision, 2026-11-26] |
| ERR-5 | `mcro` line with no macro name. | Pre-assembler | [inferred] from MAC-2 and MAC-9 ("the macro name is valid"). |
| ERR-6 | Macro name not syntactically valid: the first character is not a letter (this includes a leading `_`), a later character is not a letter, a digit or `_`, or the name is longer than 30 characters. | Pre-assembler | MAC-17 [forum: Valid Macro Names, 03/06/2025; Macro name, 22/07/2025]; MAC-33 [owner decision, 2026-11-26] |
| ERR-7 | Extra non-whitespace text after the macro name on a macro call line. | Pre-assembler | MAC-30 [owner decision, 2026-11-26], allowed by [forum: Macro Mention with Additional Text, 14/07/2025] |
| ERR-8 | A `mcro` line defines a macro name that is already in the macro table (second definition of the same name). Reported on the second `mcro` line. | Pre-assembler | MAC-28 [owner decision, 2026-11-26]; MAC-15 [forum: Do We Need to Check Multiple Definitions of the Same Macro?, 08/06/2025; Whitespace in Macros, 18/06/2025; Error Reporting, 25/06/2025] |
| ERR-9 | A label definition in front of `mcro`, in front of `mcroend`, or in front of a macro call: the first field of the line ends with `:` and the second field is `mcro`, `mcroend` or the name of a macro defined so far. `L: mcro x` still opens the definition of `x`, `L: mcroend` still closes the open definition, `L: m` is not expanded. | Pre-assembler | MAC-34, MAC-36, MAC-37 [owner decision, 2026-11-26]; MAC-21 [forum: Label at the beginning/end of macro definition, 22/07/2025] |
| ERR-10 | A line of the `.as` file is longer than 80 characters, not counting the newline or the carriage return of a CR LF line ending (SYN-25). Applies to every line, whatever its kind, except the lines discarded after a faulty `mcro` line (MAC-38). | Pre-assembler | SYN-2 [booklet p.25]; SYN-2a [forum: Line with More Than 80 Characters: Does the Assembler Ignore the Line and Continue or Stop?, 14/07/2025]; SYN-2b [owner decision, 2026-11-26] |

Effect of any pre-assembler error: report it with its `.as` line number, keep
scanning the file for further pre-assembler errors (MAC-22), leave no `.am`
file and do not run the passes for this source file (MAC-23), then continue
with the next source file (MAC-11). The exit status is 1 (CLI-9). Errors that
only the passes can find are therefore not reported for such a file in the
same run.

After a faulty `mcro` line (ERR-2, ERR-3, ERR-5, ERR-6, ERR-8) the error is
reported once on that line; the lines up to and including the matching
`mcroend` are discarded as the body of a definition and produce no further
message; the faulty macro is not entered into the macro table, and for
ERR-8 the first definition remains in force (MAC-38) [owner decision,
2026-11-26].

Not checked, by stated assumption: nested definitions (MAC-6), a missing
`mcroend` (MAC-7), use before definition (MAC-8), text before `mcroend`
(MAC-20), the content of the macro body (MAC-24). A test must not contain
these inputs, because the behaviour for them is undefined. The same holds,
until the owner decides, for the inputs of Q-35 and Q-36 (section 6.1).

Not a pre-assembler matter: a macro name on a line inside a macro body is
not expanded (MAC-32); the line reaches the first pass and fails there as
ERR-17. A label that has the name of a macro is ERR-14, in the first pass.

### 5.3 First pass

| ID | Condition | Stage | Source |
|---|---|---|---|
| ERR-11 | Label does not start with a letter, or contains a character that is not a letter or a digit. | First pass | LBL-2 [booklet p.28] [inferred as an error] |
| ERR-12 | Label longer than 30 characters. | First pass | LBL-3 [booklet p.28] [inferred as an error] |
| ERR-13 | Label is a reserved word: an operation name, a directive name without the dot (`data`, `string`, `mat`, `entry`, `extern`), a register name `r0` to `r7`, `mcro` or `mcroend`. `PSW` and `PC` are legal labels. | First pass | LBL-7 [booklet p.29]; LBL-14 [forum: Reserved Words for Label Names, 03/06/2025]; LBL-20 [owner decision, 2026-11-26] |
| ERR-14 | Label has the name of a macro defined anywhere in the same file, before or after the label. Reported on the line of the label definition; the first pass consults the macro table. | First pass | LBL-8 [booklet p.29]; LBL-15 [forum: Should we check if a label name is not a macro name or is it considered a given?, 22/07/2025]; LBL-21 [owner decision, 2026-11-26] |
| ERR-15 | Label (symbol) already in the symbol table: defined twice. Reported on the line of the later definition. (A label whose name was declared `.extern` earlier is ERR-53, not ERR-15.) | First pass | LBL-5 [booklet p.29, p.34, p.35 steps 6, 11] |
| ERR-16 | Whitespace between a label and its colon. | First pass | LBL-4 [booklet p.29] [inferred as an error] |
| ERR-17 | Unknown operation name. This includes a macro name left unexpanded inside a macro body (MAC-32). | First pass | SYN-11 [booklet p.34, p.35 step 12]; MAC-32 [owner decision, 2026-11-26] |
| ERR-18 | Unknown directive name. | First pass | [inferred] from DIR-2, DIR-4. |
| ERR-19 | Too many operands for the operation. | First pass | SYN-14 [booklet p.34] |
| ERR-20 | Too few operands for the operation. | First pass | SYN-14 [booklet p.34] ("wrong number of operands") |
| ERR-21 | Source operand addressing mode not legal for the operation. | First pass | INS-4 [booklet p.34, p.35] |
| ERR-22 | Destination operand addressing mode not legal for the operation. | First pass | INS-4 [booklet p.34, p.35] |
| ERR-23 | A matrix index that is not one of r0..r7. (A bare operand such as `r8` or `r20` is a label reference, LBL-16; it is an error only if no such label is defined, and then it is ERR-50.) | First pass | SYN-14 [booklet p.34]; LBL-16 [forum: How to Treat an Operand that is a Register and Not in Range?, 14/07/2025] |
| ERR-24 | Malformed immediate operand: `#` not followed immediately by a legal integer. | First pass | ADR-2, SYN-20 [booklet p.21, p.29] [inferred as an error] |
| ERR-25 | Malformed matrix operand: missing or unbalanced brackets, an index that is not a register, or not exactly two indices. | First pass | ADR-6 [booklet p.21] [inferred as an error] |
| ERR-26 | Comma errors between operands: missing comma, more than one comma, comma before the first operand or after the last. | First pass | SYN-13, DIR-6 [booklet p.26, p.28] |
| ERR-27 | Extra text after the last operand of an instruction (or after an instruction with no operands). | First pass | [inferred] from SYN-10 and SYN-14. |
| ERR-28 | `.data` with no values. | First pass | DIR-5 ("one or more") [booklet p.26] [inferred as an error] |
| ERR-29 | `.data` or `.mat` value that is not a legal integer. | First pass | DIR-5, SYN-20 [booklet p.26, p.29] [inferred as an error] |
| ERR-30 | `.data` or `.mat` list comma errors: missing comma, more than one comma between numbers, comma before the first or after the last number. | First pass | DIR-6 [booklet p.26] |
| ERR-31 | `.string` operand is not a legal string: missing opening or closing double quote, or characters outside the quotes. (`.string ""` is legal, DIR-32.) | First pass | DIR-9, SYN-22 [booklet p.26, p.29] [inferred as an error] |
| ERR-32 | `.string` contains a character outside ASCII 32..126. | First pass | SYN-22 [booklet p.29] [forum: What is Considered a Valid String for .string?, 25/06/2025] |
| ERR-33 | `.mat` dimensions malformed: missing brackets, whitespace between `]` and `[`, or a dimension that is not a number (for example a register). | First pass | DIR-12 [booklet p.27]; DIR-26, DIR-27 [forum: Can .mat be Defined by Non-Constant Values?, 14/07/2025; Spaces within square brackets in a matrix, 22/07/2025] |
| ERR-34 | `.entry` or `.extern` with a missing operand, more than one operand, or an operand that is not a legal label name. | First pass | DIR-17, DIR-20 [booklet p.27] [inferred as an error] |
| ERR-35 | Missing whitespace between an operation name and its first operand. | First pass | SYN-12 [booklet p.28] [inferred as an error] |
| ERR-36 | Missing whitespace between a directive name and its operand. | First pass | SYN-17 [forum: Spaces in directives, 22/07/2025] |
| ERR-37 | In a matrix operand, whitespace between the label and the first `[`, or between `]` and the second `[`. | First pass | SYN-18 [forum: Spaces within square brackets in a matrix, 22/07/2025] |
| ERR-38 | Missing whitespace between the colon of a label definition and the statement. | First pass | LBL-18 [forum: Is Whitespace Required Between a Colon (After a Label Name) and an Instruction/Directive?, 03/06/2025] |
| ERR-39 | A number outside its signed range: an immediate operand outside -128..127, or a `.data` / `.mat` value outside -512..511. | First pass | SYN-21 [forum: Maximum size of numbers using 2's complement method, 14/07/2025]; bounds [owner decision, 2026-11-26], implied by [booklet p.19, p.21, p.26] |
| ERR-40 | `.mat` with more initial values than rows × columns. | First pass | DIR-25 [forum: Initializing a Matrix with More Values Than its Size, 14/07/2025] |
| ERR-41 | Memory overflow: code plus data go beyond address 255 (more than 156 words in total). Reported once per file, on the line whose words first make the code length plus the data length exceed 156. The scan for other errors continues, but the overflow is not reported again. | First pass | ALG-15 [forum: Memory overflow, 14/07/2025; Memory size, 17/07/2025]; line and "once" ALG-17 [owner decision, 2026-11-26] |
| ERR-42 | Matrix operand with no label before the brackets. | First pass | SYN-23 [forum: Matrix definition, 17/07/2025] |
| ERR-43 | A `;` that is not the first character of the line (comment after leading whitespace or after a statement); it is reported as whatever syntax error the text produces. | First pass | SYN-9 [forum: Line with More Than 80 Characters: Does the Assembler Ignore the Line and Continue or Stop?, 14/07/2025] |
| ERR-44 | A `.mat` dimension that is 0, negative, or written with a sign (`+2`, `-2`). | First pass | DIR-33 [owner decision, 2026-11-26]; the sources are silent |
| ERR-45 | A label definition with nothing after the colon. | First pass | LBL-22 [owner decision, 2026-11-26], implied by [booklet p.25, p.28, p.35] |
| ERR-46 | A source line contains a NUL byte. | Pre-assembler | SYN-26 [project decision, 2026-11-26, pending the owner's confirmation] |
| ERR-47 | A source file opens but cannot be read (a directory, or a read error). Reported without a line number. | Driver and any stage that reads | CLI-12 [project decision, 2026-11-26, pending the owner's confirmation] |
| ERR-53 | A symbol declared `.extern` is also defined as a label in the same file, in either order. Reported once, on whichever of the two lines comes second: on the label definition line if the `.extern` line came first (then it is ERR-53, not ERR-15), and on the `.extern` line if the label definition came first. | First pass, for both orders | DIR-30 [forum: Translation Unit and Second Pass Errors, 02/07/2025]; [booklet p.35 steps 6, 11]; stage and line DIR-36 [owner decision, 2026-11-26] |

Rules that apply to all first-pass errors:

- A legal label on a line that has another error is still entered into the
  symbol table (LBL-23), so one bad line does not produce "undefined symbol"
  messages on other lines.
- All lines are still scanned (ERR-R4). The second pass is skipped
  (ERR-R7), so the errors of section 5.4 are not reported in the same run.
  No `.ob`, `.ent`, `.ext` are written (ERR-R6); the `.am` file stays
  (OUT-11). The exit status is 1 (CLI-9).

### 5.4 Second pass

| ID | Condition | Stage | Source |
|---|---|---|---|
| ERR-50 | Operand symbol not found in the symbol table (neither defined nor declared external). Reported on the line of the instruction that uses it. | Second pass | LBL-13 [booklet p.36 step 6] |
| ERR-51 | `.entry` names a symbol that is not in the symbol table. Reported on the `.entry` line. | Second pass | DIR-19 [booklet p.36 step 5] |
| ERR-52 | The same symbol is declared both `.entry` and `.extern`. Reported on the `.entry` line, where the second pass meets the conflict [inferred]. | Second pass | DIR-23 [booklet p.28]; stage from [booklet p.36 step 5] (`.entry` is handled only in the second pass) |

ERR-53 is a first-pass error and is listed in 5.3 (DIR-36).

Effect: no `.ob`, `.ent`, `.ext` are written (ERR-R6); the `.am` file stays
(OUT-11). The exit status is 1 (CLI-9).

### 5.5 Fatal condition

A memory allocation failure, in any stage: the assembler reports it, frees
all memory and the whole program exits (ERR-R10) with status 1 (CLI-9). It has
no ERR ID because a test cannot provoke it.

### 5.6 Warnings

A warning uses the format of ERR-R14, never suppresses output files and
never changes the exit status. The assembler prints exactly one kind of
warning.

| ID | Condition | Stage | Source |
|---|---|---|---|
| WRN-1 | A label in front of `.entry` or `.extern`: the first field of the line ends with `:` and the second field is `.entry` or `.extern`. The label is ignored completely: it is not entered into the symbol table and its name is not validated. The directive is processed as usual. | First pass | DIR-18, DIR-22 [booklet p.27, p.28]; DIR-35 [owner decision, 2026-11-26] |

If the directive on that line has a first-pass error, only the error is
printed (ERR-R16). A file whose only messages are warnings assembles: all
its output files are written and it does not make the exit status 1.

### 5.7 Constructs that are legal and must not be reported

Listed because each was once an open question or is easy to mistake for an
error.

| Construct | Behaviour | Rule |
|---|---|---|
| `.string ""` | Legal; one word holding 0. | DIR-32 |
| `.extern X` twice, or `.entry X` twice | Accepted silently; `X` appears once in `.ent`. | DIR-34 |
| `.mat [2][2]1,2,3,4` (no whitespace after `]`) | Legal. | DIR-33 |
| `.mat` with fewer values than cells | Legal; the rest are 0. | DIR-24 |
| A label named `PSW`, `PC`, `r8`, `r20` | Legal. | LBL-20, LBL-16 |
| `.data`, `.string`, `.mat` without a label | Legal. | DIR-28 |
| A program without `stop` | Legal. | ALG-16 |
| No file arguments | Not an error; one usage line on `stdout`, exit status 0. | CLI-5, CLI-9, CLI-10 |
| A label in front of `.entry` or `.extern`, whatever its name | Not an error; a warning (WRN-1), the label is ignored, outputs are written, exit status unchanged. | DIR-35 |
| A last line without a newline | Legal; a complete line. The `.am` file ends with a newline. | SYN-24 |
| CR LF line endings | Legal; the CR is dropped, does not count toward the 80 characters and is not copied to `.am`. | SYN-25 |
| A source file with no statements (empty, or only empty and comment lines) | Legal; `.am` and an `.ob` whose only line is `a<TAB>a`. | OUT-15 |
| A file that assembles without errors or warnings | Nothing is printed. | ERR-R15 |
| The same error message repeated for every call of a macro | Accepted. | ERR-R12, MAC-13 |

---

## 6. Open questions

This section lists what is still undecided and what was settled. Q numbers
are stable: a question
that has been settled keeps its number in the table in 6.2, so that older
references still resolve. Q-19 and Q-23 were never used.

### 6.1 Still open

Q-26 to Q-33 were decided by the owner on 2026-11-26 and are in the table in
6.2. Nothing that the owner has been asked is open.

Writing those decisions down exposed four small gaps, listed here. They are
not decided: the sources are silent on each, and each waits for the owner.
Until an item is decided, tests must stay away from the input it describes;
each item says how.

- **Q-34 A warning from the first pass and an error from the second pass on
  the same line.** `L: .entry X` gets the warning WRN-1 in the first pass
  (DIR-35). If `X` is undefined (ERR-51) or external (ERR-52), the second
  pass finds an error on the same line. ERR-R16 says that a warning and an
  error on one line "do not arise by construction" and that otherwise only
  the error is printed; ERR-R17 prints the first-pass messages before the
  second pass runs. Options: print both (the warning, later the error); or
  hold the warning back and print only the error. Until decided: no test
  puts a label in front of a `.entry` line whose operand produces ERR-51 or
  ERR-52.
- **Q-35 A line longer than 80 characters that is also a `mcro` line, a
  `mcroend` line or a macro call.** The line gets one message (ERR-10,
  ERR-R16) and "is not examined further"; whether it still opens or closes
  a definition, as a faulty `mcro` line does (MAC-38), is not stated, and it
  decides which later lines produce messages. Until decided: no test makes
  such a line longer than 80 characters.
- **Q-36 A `mcroend` line while no definition is open,** plain or with a
  label in front (`L: mcroend`, which MAC-36 recognises and reports). MAC-7
  covers only the opposite case. Options: a pre-assembler error; or an
  ordinary line that fails in the first pass as an unknown operation. Until
  decided: no test contains a `mcroend` line without an open definition.
- **Q-37 Memory overflow together with other errors.** ALG-17 counts the
  words of each line. Not stated: how many words a line with another error
  adds to the running total, and what is printed when the line that crosses
  the limit has another error as well (ERR-R16 allows one message for that
  line, which may then not be the overflow). Until decided: a test for
  ERR-41 has no other erroneous line at or before the line that crosses the
  limit.
- **Q-38 A `mcro` with no `mcroend` before the end of the file.** The
  booklet lets the assembler assume that every definition is closed (MAC
  rules), so nothing requires a check. Today the unfinished body is swallowed
  silently and the rest of the file assembles. Options: report an error on
  the `mcro` line at the end of the file; or keep the assumption. Until
  decided: no test leaves a definition open.
- **Q-39 A reserved word, an ill-formed name or an over-long name used as an
  operand, and a macro name used as the operand of `.extern`.** No ERR-n
  names these. Today each is a first-pass error on that line. The
  alternative is an "undefined symbol" error in the second pass (ERR-50).
  Until decided: tests assert only that the line gets exactly one error.
- **Q-40 Confirmation of SYN-26 and CLI-12.** Both were decided by the
  project after the review found the gaps; the owner has not yet confirmed
  them.

### 6.2 Resolved

"Owner" in the last column means [owner decision, 2026-11-26]; the rule cited
gives the source that allows or implies the decision.

| Q | Question | Resolution | Rule |
|---|---|---|---|
| Q-1 | Initial IC and load address (p.33 says 100, p.35 says 0). | 100. [forum: First Address for Loading into Memory, 08/06/2025] | ALG-2 |
| Q-2 | Line number in error messages: `.as` or `.am`? | The sources accept either [forum: Printing Errors with Line Number in Case of Syntax Error in File, 02/07/2025]. Owner (C-1): always the `.as` line, through a line map; a macro body line reports its line inside the definition. | ERR-R12, MAC-35, section 2.14 |
| Q-3 | File name and format of error messages. | Sources silent. Owner: `<name>.as:<line>: error: <text>` on `stdout`; `<name>.as: error: <text>` without a line; `warning:` for warnings; wording free; tests compare file, line and severity. | ERR-R13, ERR-R14 |
| Q-4 | Separators and layout of `.ob`, `.ent`, `.ext`. | Widths from the example [booklet p.45] [forum: Output File Format, 08/06/2025]. Owner: one TAB, no indentation, no trailing whitespace, no blank lines, newline after every line; header without leading `a`; zero is `a`. | OUT-13, OUT-5 |
| Q-5 | Program larger than memory. | Error; keep looking for more errors. [forum: Memory overflow, 14/07/2025] | ALG-15, ERR-41 |
| Q-6 | Exact numeric bounds. | Owner: signed only, -128..127 for an immediate operand and -512..511 for `.data` / `.mat`. Implied by [booklet p.19, p.21, p.26] and the overflow ruling [forum: Maximum size of numbers using 2's complement method, 14/07/2025]. | SYN-21, ERR-39 |
| Q-7 | `.mat` dimension 0, negative or signed; whitespace before the first value. | Sources silent. Owner: a dimension is unsigned decimal digits with value at least 1, anything else is an error; the whitespace is optional. | DIR-33, ERR-44 |
| Q-8 | `.extern X` twice, `.entry X` twice. | Owner: accepted silently; one line in `.ent`. Implied by the skeleton algorithms [booklet p.35 step 10, p.36 step 5]. | DIR-34 |
| Q-9 | Are directive names without the dot, `PSW` and `PC` reserved? | Owner: `data`, `string`, `mat`, `entry`, `extern` are reserved for labels and macros [booklet p.29]; `PSW` and `PC` are not (sources silent). | LBL-20, ERR-13, ERR-2 |
| Q-10 | A macro name inside the body of another macro. | Owner: not expanded; the line is copied as it is and the first pass reports an unknown operation. Implied by [booklet p.31 step 6] and [forum: Must a Macro Contain a Sequence of Instructions and/or Directives?, 25/06/2025]. | MAC-32, ERR-17 |
| Q-11 | Macro name starting with `_`; register name as a macro name; macro name equal to a label. | Owner: first character must be a letter; a register name is illegal; a label equal to a macro name is an error wherever the two stand, detected in the first pass with the macro table. Implied by [forum: Valid Macro Names, 03/06/2025], [booklet p.25, p.29] and [forum: Should we check if a label name is not a macro name or is it considered a given?, 22/07/2025]. | MAC-33, LBL-21, ERR-2, ERR-6, ERR-14 |
| Q-12 | Does the pre-assembler check errors (p.25 against p.34)? Is `.am` left after a failure? | It checks the macro name and the `mcro` / `mcroend` lines, finds all such errors, and leaves no `.am`. | MAC-18, MAC-22, MAC-23 |
| Q-13 | Where the 80-character limit is checked. | The sources leave it to the implementer. Owner (C-2): the pre-assembler; a pre-assembler error. | SYN-2a, SYN-2b, ERR-10 |
| Q-14 | Label alone on a line; label on a line with another error. | Owner: a label alone is an error (implied by [booklet p.25, p.28, p.35]); a label on an erroneous line is still entered into the symbol table (sources silent). | LBL-22, LBL-23, ERR-45 |
| Q-15 | Comments after leading whitespace or after a statement. | Not comments; `;` must be the first character of the line. | SYN-9 |
| Q-16 | Argument with `.as` attached; exit status. | Owner: the argument is always a name without extension, so `x.as` tries `x.as.as` [forum: Input File Name, 22/06/2025]; exit status 0 only when every file assembled without errors, otherwise 1 (sources silent). | CLI-8, CLI-9, ERR-1 |
| Q-17 | `.string ""`. | Sources silent. Owner: legal; one word holding 0. | DIR-32 |
| Q-18 | Attribute of a `.mat` symbol (blank on p.37). | "data". | DIR-16 |
| Q-20 | Rulings reported by the owner from memory. | Each was looked up in the forum text; see section 2.13. | section 2.13 |
| Q-21 | Duplicate macro names: moderator posts that disagree. | Owner: a second definition is a pre-assembler error. This satisfies all four posts. | MAC-15, MAC-28, ERR-8 |
| Q-22 | Label in front of `mcro`, `mcroend` or a macro call. | Owner: an error, the stricter reading of [forum: Label at the beginning/end of macro definition, 22/07/2025]. The reporting stage is settled by Q-28. | MAC-34, ERR-9 |
| Q-24 | The `prn` word in the worked examples. | Owner: `dbaaa` (opcode 13) is settled; it follows from correction 1. | INS-1, section 4.3 |
| Q-25 | Global const tables (`CLAUDE.md` against the moderator). | Owner: the instruction table is `static const`, private to its module, reached through access functions; no global arrays. [forum: Defining Global Variables for Arrays, 14/07/2025; Variables, 30/06/2025] | IMP-1 |
| Q-26 | What is written to `stdout` when there is no error; a run with no arguments. | Sources silent; a notice for no arguments is allowed by [forum: Number of Input Files, 14/07/2025]. Owner: nothing is printed for a file that assembles without errors or warnings, `stdout` carries only diagnostics; with no arguments one usage line (wording free) and exit status 0. | ERR-R15, CLI-10 |
| Q-27 | Label in front of `.entry` or `.extern`. | Owner: a warning on that line from the first pass; the label is ignored completely, not entered into the symbol table and not validated; outputs and exit status are unaffected. Offered by [booklet p.27, p.28]. | DIR-35, WRN-1, ERR-R14 |
| Q-28 | Stage that reports a label in front of `mcro`, `mcroend` or a macro call. | Sources silent on the stage. Owner: the pre-assembler; recognised by the first field ending with `:` and the second field being `mcro`, `mcroend` or a macro name defined so far; `L: mcro x` still opens the definition, `L: mcroend` still closes it, `L: m` is not expanded. | MAC-36, MAC-37, MAC-34, ERR-9 |
| Q-29 | Number and order of messages. | Owner: exactly one message per line with errors, the first error found; files in command-line order, within a file all pre-assembler messages or else first-pass then second-pass messages, within a stage in the order the lines are read (the `.am` order for the passes). Allowed by [forum: Error reporting, 22/07/2025]; the sources are silent on the order. | ERR-R16, ERR-R17 |
| Q-30 | Line and stage of ERR-41 and of ERR-53. | Sources silent. Owner: (a) memory overflow once per file, first pass, on the line whose words first exceed the memory; (b) ERR-53 in the first pass on whichever of the two lines comes second. | ALG-17, ERR-41; DIR-36, ERR-53 |
| Q-31 | Output files left from an earlier run. | Owner: before processing a file every existing `.am`, `.ob`, `.ent` and `.ext` of that name is removed. This goes beyond [forum: Deleting previous .ent and .ext files, 17/07/2025], which only says deleting is not required. | OUT-14, OUT-12 |
| Q-32 | Last line without a newline; CR LF line endings; a source with no statements. | Sources silent. Owner: (a) accepted as a complete line, `.am` always ends each line with a newline; (b) the CR before the newline is removed, is not counted toward 80 and is not copied, `.am` has LF endings, a CR elsewhere is whitespace; (c) assembles, `.ob` holds the single line `a<TAB>a`. | SYN-24, SYN-25, OUT-15, MAC-29 |
| Q-33 | A file that cannot be opened among several files. | Owner: reported (ERR-1), the assembler continues with the next argument, exit status 1. Follows [forum: Input File Name, 22/06/2025] and CLI-3. | CLI-11, ERR-1 |
| (none) | Lines after a faulty `mcro` line (gap found in review). | Sources silent. Owner: one message on the `mcro` line; the lines up to and including the matching `mcroend` are discarded as a body without further messages; the macro is not entered into the table, and for a duplicate name the first definition remains in force. | MAC-38 |

### 6.3 Slips in the booklet (not questions)

Recorded so that nobody changes the code to match them. p.26: "there are four
kinds of directive statements", followed by five. p.28: the label definition
sentence lists only `.data` and `.string` (p.29 adds `.mat`). p.29: "the three
bits in the A,R,E field", although the field has two bits. p.21: "in the six
bits 6-9" (fixed by correction 4). p.34: "the pre-assembler stage contains no
error detection stage" (narrowed by MAC-18). p.35: "starting at address 0" and
"IC ← 0" (overridden by ALG-2). p.37: blank attribute for `M1` (DIR-16).
p.23: jsr pushes "PC+2" regardless of the instruction length (run-time
semantics, outside the assembler). p.45: `ps.ob` prints `daaaa` at address
109 (section 4.3).
