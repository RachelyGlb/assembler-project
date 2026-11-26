# SPEC: two-pass assembler for the 10-bit imaginary machine

This file is the verified digest of the course documents for Assignment 14 of
course 20465. Every rule has a stable ID and ends with its source. Code, tests
and audits cite the rule IDs.

## 1. Sources and precedence

| Tag | Source | Notes |
|---|---|---|
| `[booklet p.NN]` | `docs/booklet.pdf`, the assignment booklet (Hebrew) | Printed pages 17 to 46. PDF page N is printed page N+16. All citations use printed page numbers. |
| `[correction N]` | `docs/corrections.pdf` / `docs/corrections.txt`, the official corrections sheet, items 1 to 4 | Overrides the booklet. |
| `[forum: <thread title>, <date>]` | `docs/forum.txt`, the course forum Q&A (English; extracted from `docs/Task_14.pdf`, 90 pages) | Highest priority. Only posts marked "Posted by moderator" are rulings; the date is the date of the moderator post (dd/mm/yyyy). Student posts are never rulings; where one is mentioned it is labelled "student post, not a ruling". Names of students are not reproduced here. |
| `[inferred]` | A conclusion drawn by the spec author | Always accompanied by its reasoning. Not an official statement. |

Precedence, highest first: forum moderator rulings, corrections sheet,
booklet. A moderator ruling overrides the corrections sheet and the booklet;
every rule below that is changed by the forum says so.

Notes on the forum text:

- The whole forum text was read. `docs/forum.txt` contains the same set of
  threads twice (the first copy is cut short, the second copy is complete);
  the two copies agree. Unclear passages were checked against the PDF.
- The corrections sheet is also posted in the forum by the moderator.
  [forum: Clarifications and Corrections, 14/07/2025]
- Several moderator answers say only "an answer exists in the forum" or "at
  your discretion". The first kind adds nothing; the second kind is recorded
  in section 2.15 as a choice left to the implementer.
- When two moderator posts disagree, both are cited and the later one is
  applied; the disagreement is also listed in section 6.
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
  the line length is greater than 80 characters?, 22/07/2025] The stage that
  performs the check is the implementer's choice (section 2.15).

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
  answers to in the booklet., 22/07/2025])
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
  [booklet p.28]
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
- **DIR-31** A string given to `.string` does not need, and is not checked
  for, any terminator of its own; the assembler always adds the terminating
  zero word in memory. [forum: Questions I didn't find answers to in the
  booklet., 22/07/2025]

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
  [booklet p.29]
- **LBL-8** The same symbol must not serve both as a label and as a macro
  name. The assembler must check this. [booklet p.29]
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
  method, 14/07/2025] The moderator gave no numeric range. [inferred] In two's
  complement the widths hold -128..127 for an immediate operand and -512..511
  for a `.data` / `.mat` value. Whether the unsigned upper halves (128..255,
  512..1023) are also accepted is Q-6.
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
  [forum: What is Considered a Valid String for .string?, 23/06/2025]
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
  removed. All other lines are copied unchanged. [booklet p.30, p.31]
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
  to the `.as` file. [booklet p.25, p.29-30, p.39]
- **MAC-15** There is no need to check for duplicate macro names.
  [forum: Macro name, 22/07/2025] The booklet is silent. **Conflict inside the
  forum:** an earlier moderator post answered "Yes" to "should an error be
  output for several macros with the same name?" [forum: Do We Need to Check
  Multiple Definitions of the Same Macro?, 08/06/2025]. The later post is
  applied here; see Q-21.
- **MAC-17** A macro name follows the rules of a label name (starts with a
  letter, then letters and digits), and in addition the character `_` is
  legal in a macro name. [forum: Valid Macro Names, 03/06/2025] Its length is
  limited like a label name, 30 characters. [forum: Macro name, 22/07/2025]
  The booklet gives no syntax for macro names.
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
  22/07/2025] (The one-word answer does not say whether this is an assumption
  or an error to report; see Q-22.)
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
  booklet's "identical to the source file" (MAC-14).
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
  and the assembler goes on looking for further errors. The final IC and DC
  are what the check uses. [forum: Memory overflow, 14/07/2025; Memory size,
  17/07/2025; Questions Regarding the First Pass, 22/06/2025] [inferred] With
  the load address 100 (ALG-2) and addresses 0..255 (M-4), the limit is
  code words + data words ≤ 156, that is the last used address ≤ 255.
- **ALG-16** The instructions are only encoded, never executed. No rule
  requires any particular instruction (for example `stop`) to be present in a
  program. [forum: Jumping to a Label, 17/06/2025; Must the stop command
  appear at least once?, 22/07/2025]

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
  02/07/2025] See section 2.14.
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
  error is", but it is not a stated rule. See Q-3.
- **ERR-R9** One error message per input line is sufficient; every line of
  the file must still be examined. [forum: Error reporting, 22/07/2025] The
  booklet is silent and compatible (p.34, p.46).
- **ERR-R10** On a memory allocation failure the assembler reports an error,
  releases all allocated memory, and the entire program exits (it does not
  move on to the next file). [forum: Linked List for Macros, 17/06/2025;
  Memory Allocation Errors, 14/07/2025; Memory allocation error, 22/07/2025]
  The booklet is silent.
- **ERR-R11** Every file opened while processing a source file is closed when
  the processing of that file ends. [forum: A Few Questions That Came Up,
  22/07/2025]

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
- **CLI-6** The argument is the file name without the extension. The assembler
  tries to open the name with `.as` appended and reports an error if the file
  does not open; this must be checked, not assumed. [forum: Input File Name,
  22/06/2025]
- **CLI-7** There is no limit on the number of input files. [forum: Limit on
  Number of Input Files, 14/07/2025]
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
  authority for the widths; the separator characters cannot be read from a PDF
  and stay open (Q-4). (The same widths, with the reasoning "the header is
  just a number, an address needs 8 bits, a word 10 bits", appear in student
  posts, not rulings.)
- **OUT-6** The column heading "Base 4 address / Base 4 code" printed above
  the example is not part of the file. [booklet p.45]
- **OUT-7** Lines of `.ob` after the first are in ascending address order,
  starting at 100, code words then data words. [booklet p.42, p.45]
- **OUT-8** The `.ent` file is made of text lines. Each line has the name of a
  symbol declared as entry and its value as found in the symbol table (after
  ALG-6), in unique base 4. Example: `LOOP bccd`, `LENGTH caab`.
  [booklet p.42, p.45]
- **OUT-9** The `.ext` file is made of text lines. Each line has the name of a
  symbol declared external and the address, in unique base 4, of a machine
  word that encodes an operand referring to that symbol. Every reference gets
  its own line, so a symbol used several times appears on several lines.
  Example: `W bcbd`, `W bcda`, `L3 bdca`. [booklet p.39, p.42, p.45]
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
| No need to check for duplicate macro names. | Found, but an earlier moderator post says the opposite. | [forum: Macro name, 22/07/2025] against [forum: Do We Need to Check Multiple Definitions of the Same Macro?, 08/06/2025] | MAC-15, Q-21 | Silent. |
| A label in front of `.entry` or `.extern` is ignored (a warning is allowed). | **Not found in forum** as a ruling. The only related moderator post refers the asker back to the booklet. The rule itself stands on the booklet. | [forum: Questions I didn't find answers to in the booklet., 22/07/2025] (reference to the booklet only) | DIR-18, DIR-22 | States it (p.27, p.28). |

### 2.14 Which file does the line number in an error message refer to?

**Answer: either. The forum settles it: the line number may be the one in the
original `.as` file or the one in the expanded `.am` file.**
[forum: Printing Errors with Line Number in Case of Syntax Error in File,
02/07/2025]

The ruling was given to a student whose first pass reads a `.am` file from
which macro definitions, empty lines and comment lines had been removed, and
who asked whether the numbers must be mapped back to the source file. A later
thread asking the same question was answered only with "an answer exists in
the forum". [forum: Error Line Number in Assembler, 14/07/2025]

Consequences:

- An error found by the first or second pass may report the `.am` line number
  or the `.as` line number. The project must pick one and use it consistently;
  this choice belongs to the owner (section 2.15, item C-1).
- An error found by the pre-assembler can only refer to the `.as` file,
  because no `.am` file exists for a file that fails there (MAC-23).
  [inferred]
- Tests must not assume a particular choice unless the owner fixes it.

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
sources: the sources allow every option listed. Each needs a project decision
so that tests can be written.

| ID | Choice | Options allowed | Source |
|---|---|---|---|
| C-1 | Line number base in pass error messages. | `.as` line or `.am` line. | [forum: Printing Errors with Line Number in Case of Syntax Error in File, 02/07/2025] |
| C-2 | Stage that checks the 80-character limit. | Pre-assembler or first pass. | [forum: Character Overflow Error, 09/06/2025; Checking Line Length Validity, 22/06/2025] |
| C-3 | Empty lines and comment lines in `.am`. | Keep them or delete them. | [forum: Handling Empty Lines and Comment Lines, 09/06/2025; Should the pre-assembler also copy empty lines or can it ignore and delete them?, 17/07/2025] |
| C-4 | Extra text after a macro name on a call line. | Report in the pre-assembler (called "more logical" by the moderator) or leave the line for the first pass. | [forum: Macro Mention with Additional Text, 14/07/2025] |
| C-5 | After reporting extra text on a `mcroend` line: close the macro or ignore the line. | Either. | [forum: Redundant Text After Command, 14/07/2025] |
| C-6 | Where the encoding is done. | First and second pass as in the skeleton, or all in the second pass. | [forum: Is it Possible to Encode Only in the Second Pass?, 14/07/2025] |
| C-7 | Wording for an undefined `rNN` operand. | "Undefined label" or "register out of range". | [forum: How to Treat an Operand that is a Register and Not in Range?, 14/07/2025] |
| C-8 | Granularity of pre-assembler messages. | A general "invalid macro name" is enough. | [forum: Error Reporting, 25/06/2025] |
| C-9 | Internal storage of the tables. | Implementer's choice; any temporary file must be deleted on error and at the end. | [forum: Macro Tables / Labels, 01/07/2025] |

Rulings about the code rather than the behaviour (recorded because they touch
the project's coding rules):

- **IMP-1** Arrays must not be defined as global variables; this answer was
  given to a question about a read-only (`const`) table of the instructions.
  Individual global variables are allowed. [forum: Defining Global Variables
  for Arrays, 14/07/2025; Variables, 30/06/2025] This is stricter than the
  rule in `CLAUDE.md` that allows const tables; see Q-25.
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

The separator between the two fields of a line is shown as one space; the
booklet does not define it (Q-4).

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

After macro expansion (`.am`), p.30:

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
bbc dd
bcba aacba
bcbb cabbc
bcbc acbda
bcbd caabc
bcca acdba
bccb acaaa
bccc bdccc
bccd cbaba
bcda bdcbc
bcdb dbaaa
bcdc ddcda
bcdd addda
bdaa abbaa
bdab bdaba
bdac cabac
bdad aacda
bdba cabbc
bdbb adada
bdbc aaada
bdbd ccaba
bdca bccdc
bdcb ddaaa
bdcc abcab
bdcd abcac
bdda abcad
bddb abcba
bddc abcbb
bddd abcbc
caaa aaaaa
caab aaabc
caac dddbd
caad aaadd
caba aabbc
cabb aaaab
cabc aaaac
cabd aaaad
caca aaaba
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
bbc dd
bcba aacba
bcbb cabbc
bcbc acbda
bcbd aaaab
bcca acdba
bccb acaaa
bccc bdccc
bccd cbaba
bcda aaaab
bcdb dbaaa
bcdc ddcda
bcdd addda
bdaa abbaa
bdab bdaba
bdac cabac
bdad aacda
bdba cabbc
bdbb adada
bdbc aaada
bdbd ccaba
bdca aaaab
bdcb ddaaa
bdcc abcab
bdcd abcac
bdda abcad
bddb abcba
bddc abcbb
bddd abcbc
caaa aaaaa
caab aaabc
caac dddbd
caad aaadd
caba aabbc
cabb aaaab
cabc aaaac
cabd aaaad
caca aaaba
```

Expected `ps.ent` (line order is free, OUT-10):

```
LOOP bccd
LENGTH caab
```

Expected `ps.ext` (line order is free, OUT-10):

```
W bcbd
W bcda
L3 bdca
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
  agrees with the student's statement. See Q-24.
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

Each condition is something the assembler must detect. "Source" names the rule
and page. Conditions marked [inferred] are not listed by the booklet in so
many words; they follow from a stated syntax rule together with the general
duty to report syntax errors ("and other errors", p.34; "detect and report all
erroneous lines", p.46).

### 5.1 Pre-assembler

| ID | Condition | Source |
|---|---|---|
| ERR-1 | A source file named on the command line cannot be opened (name + `.as`). | CLI-6 [forum: Input File Name, 22/06/2025] |
| ERR-2 | The macro name is the name of an instruction or of a directive ("and the like"). | MAC-9 [booklet p.25] |
| ERR-3 | Extra characters after the macro name on the `mcro` line. | MAC-10 [booklet p.25] |
| ERR-4 | Extra characters on the `mcroend` line. | MAC-10 [booklet p.25] |
| ERR-5 | `mcro` line with no macro name. | [inferred] from MAC-2 and MAC-9 ("the macro name is valid"). |
| ERR-6 | Macro name not syntactically valid: does not start with a letter, contains a character other than letters, digits and `_`, or is longer than 30 characters. | MAC-17 [forum: Valid Macro Names, 03/06/2025; Macro name, 22/07/2025] |
| ERR-7 | Extra text after the macro name on a macro call line. Reporting it in the pre-assembler is the moderator's recommendation, not an obligation; otherwise the line is left for the first pass to reject. | [forum: Macro Mention with Additional Text, 14/07/2025] |

Effect of any pre-assembler error: report it with its line number, keep
scanning the file for further pre-assembler errors (MAC-22), leave no `.am`
file and do not run the passes for this source file (MAC-23), then continue
with the next source file (MAC-11).

Not checked, by stated assumption: nested definitions (MAC-6), missing
`mcroend` (MAC-7), use before definition (MAC-8), text before `mcroend`
(MAC-20), the macro body (MAC-24), duplicate macro names (MAC-15, but see
Q-21). A label in front of `mcro`, `mcroend` or a macro call is ruled out by
MAC-21; whether it must be reported is Q-22.

### 5.2 First pass

| ID | Condition | Source |
|---|---|---|
| ERR-10 | Line longer than 80 characters (excluding the newline). May be detected in the pre-assembler instead (section 2.15). | SYN-2, SYN-2a [booklet p.25] [forum: Line with More Than 80 Characters: Does the Assembler Ignore the Line and Continue or Stop?, 14/07/2025] |
| ERR-11 | Label does not start with a letter, or contains a character that is not a letter or a digit. | LBL-2 [booklet p.28] [inferred as an error] |
| ERR-12 | Label longer than 30 characters. | LBL-3 [booklet p.28] [inferred as an error] |
| ERR-13 | Label is a reserved word (operation, directive or register name, `mcro`, `mcroend`). | LBL-7 [booklet p.29]; LBL-14 [forum: Reserved Words for Label Names, 03/06/2025] |
| ERR-14 | Label is the same as a macro name. | LBL-8 [booklet p.29] |
| ERR-15 | Label (symbol) already in the symbol table: defined twice. | LBL-5 [booklet p.29, p.34, p.35 steps 6, 11] |
| ERR-16 | Whitespace between a label and its colon. | LBL-4 [booklet p.29] [inferred as an error] |
| ERR-17 | Unknown operation name. | SYN-11 [booklet p.34, p.35 step 12] |
| ERR-18 | Unknown directive name. | [inferred] from DIR-2, DIR-4. |
| ERR-19 | Too many operands for the operation. | SYN-14 [booklet p.34] |
| ERR-20 | Too few operands for the operation. | SYN-14 [booklet p.34] ("wrong number of operands") |
| ERR-21 | Source operand addressing mode not legal for the operation. | INS-4 [booklet p.34, p.35] |
| ERR-22 | Destination operand addressing mode not legal for the operation. | INS-4 [booklet p.34, p.35] |
| ERR-23 | A matrix index that is not one of r0..r7. (A bare operand such as `r8` or `r20` is a label reference, LBL-16; it is an error only if no such label is defined, and then it is ERR-50, with either wording.) | SYN-14 [booklet p.34]; LBL-16 [forum: How to Treat an Operand that is a Register and Not in Range?, 14/07/2025] |
| ERR-24 | Malformed immediate operand: `#` not followed immediately by a legal integer. | ADR-2, SYN-20 [booklet p.21, p.29] [inferred as an error] |
| ERR-25 | Malformed matrix operand: missing or unbalanced brackets, an index that is not a register, or not exactly two indices. | ADR-6 [booklet p.21] [inferred as an error] |
| ERR-26 | Comma errors between operands: missing comma, more than one comma, comma before the first operand or after the last. | SYN-13, DIR-6 [booklet p.26, p.28] |
| ERR-27 | Extra text after the last operand of an instruction (or after an instruction with no operands). | [inferred] from SYN-10 and SYN-14. |
| ERR-28 | `.data` with no values. | DIR-5 ("one or more") [booklet p.26] [inferred as an error] |
| ERR-29 | `.data` or `.mat` value that is not a legal integer. | DIR-5, SYN-20 [booklet p.26, p.29] [inferred as an error] |
| ERR-30 | `.data` or `.mat` list comma errors: missing comma, more than one comma between numbers, comma before the first or after the last number. | DIR-6 [booklet p.26] |
| ERR-31 | `.string` operand is not a legal string: missing opening or closing double quote, or characters outside the quotes. | DIR-9, SYN-22 [booklet p.26, p.29] [inferred as an error] |
| ERR-32 | `.string` contains a character outside ASCII 32..126. | SYN-22 [booklet p.29] [forum: What is Considered a Valid String for .string?, 25/06/2025] |
| ERR-33 | `.mat` dimensions malformed: missing brackets, whitespace between `]` and `[`, or a dimension that is not a number (for example a register). | DIR-12 [booklet p.27]; DIR-26, DIR-27 [forum: Can .mat be Defined by Non-Constant Values?, 14/07/2025; Spaces within square brackets in a matrix, 22/07/2025] |
| ERR-34 | `.entry` or `.extern` with a missing operand, more than one operand, or an operand that is not a legal label name. | DIR-17, DIR-20 [booklet p.27] [inferred as an error] |
| ERR-35 | Missing whitespace between an operation name and its first operand. | SYN-12 [booklet p.28] [inferred as an error] |
| ERR-36 | Missing whitespace between a directive name and its operand. | SYN-17 [forum: Spaces in directives, 22/07/2025] |
| ERR-37 | In a matrix operand, whitespace between the label and the first `[`, or between `]` and the second `[`. | SYN-18 [forum: Spaces within square brackets in a matrix, 22/07/2025] |
| ERR-38 | Missing whitespace between the colon of a label definition and the statement. | LBL-18 [forum: Is Whitespace Required Between a Colon (After a Label Name) and an Instruction/Directive?, 03/06/2025] |
| ERR-39 | A number overflows its field: an immediate operand that does not fit in 8 bits, or a `.data` / `.mat` value that does not fit in 10 bits. | SYN-21 [forum: Maximum size of numbers using 2's complement method, 14/07/2025]; exact bounds: Q-6 |
| ERR-40 | `.mat` with more initial values than rows × columns. | DIR-25 [forum: Initializing a Matrix with More Values Than its Size, 14/07/2025] |
| ERR-41 | Memory overflow: code plus data go beyond address 255 (more than 156 words in total). Reported once the sizes are known; the scan for other errors continues. | ALG-15 [forum: Memory overflow, 14/07/2025; Memory size, 17/07/2025] |
| ERR-42 | Matrix operand with no label before the brackets. | SYN-23 [forum: Matrix definition, 17/07/2025] |
| ERR-43 | A `;` that is not the first character of the line (comment after leading whitespace or after a statement); it is reported as whatever syntax error the text produces. | SYN-9 [forum: Line with More Than 80 Characters: Does the Assembler Ignore the Line and Continue or Stop?, 14/07/2025] |

Warnings (not errors, optional): a label in front of `.entry` or `.extern`
(DIR-18, DIR-22) [booklet p.27, p.28].

Effect: all lines are still scanned (ERR-R4); the second pass is skipped and
no `.ob`, `.ent`, `.ext` are written (ERR-R6, ERR-R7).

### 5.3 Second pass

| ID | Condition | Source |
|---|---|---|
| ERR-50 | Operand symbol not found in the symbol table (neither defined nor declared external). | LBL-13 [booklet p.36 step 6] |
| ERR-51 | `.entry` names a symbol that is not in the symbol table. | DIR-19 [booklet p.36 step 5] |
| ERR-52 | The same symbol is declared both `.entry` and `.extern`. | DIR-23 [booklet p.28] |
| ERR-53 | A symbol declared `.extern` is also defined as a label in the same file, in either order. (May be detected in the first pass.) | DIR-30 [forum: Translation Unit and Second Pass Errors, 02/07/2025] |

Fatal condition, any stage: a memory allocation failure. The assembler
reports it, frees all memory and the whole program exits (ERR-R10).

Effect: no `.ob`, `.ent`, `.ext` are written (ERR-R6).

### 5.4 Conditions the sources do not classify

These are deliberately **not** given ERR IDs, because neither the forum nor
the booklet says whether they are errors. Each is an open question in section
6: a number in the unsigned upper half of its field (Q-6), zero or negative
`.mat` dimensions (Q-7), `.extern` or `.entry` repeated for the same symbol
(Q-8), a label alone on a line (Q-14), an empty string (Q-17), duplicate macro
names (Q-21), a label in front of `mcro` / `mcroend` / a macro call (Q-22).

---

## 6. Open questions

This section lists only what neither the forum nor the booklet settles. Q
numbers are stable: numbers of questions that the forum resolved are kept in
the table in 6.2 so that older references still resolve.

### 6.1 Still open

- **Q-3 File name in error messages.** No source requires it. The booklet
  requires the line number (p.34) and enough detail to know "what and where"
  each error is (p.46); the forum adds only "with the line number"
  [forum: Error Reporting, 25/06/2025]. No message format is given anywhere.
  Options: print `file:line: message`, or line and message only.
- **Q-4 Separators in `.ob`, `.ent`, `.ext`.** The fields are defined (OUT-4,
  OUT-8, OUT-9) and the widths follow the booklet example, to which the
  moderator refers [forum: Output File Format, 08/06/2025]. What cannot be
  read from the PDF: whether fields are separated by a space or a tab and how
  many; whether the first line of `.ob` is indented as printed on p.45;
  whether a header value of zero is printed as `a`. A student pointed out the
  PDF problem and got no reply.
- **Q-6 Exact numeric bounds.** The moderator ruled that overflow must be
  checked [forum: Maximum size of numbers using 2's complement method,
  14/07/2025] but gave no bounds. Readings: signed only (immediate -128..127,
  data -512..511); or also the unsigned upper half (immediate up to 255, data
  up to 1023). A student argued for -128..127; that is not a ruling.
- **Q-7 `.mat` dimension details.** Settled: partial initialisation (DIR-24),
  too many values (DIR-25), non-numeric dimensions (DIR-26), bracket
  whitespace (DIR-27). Still open: whether a dimension may be 0, negative or
  written with a sign; whether whitespace is required between the last `]` and
  the first initial value.
- **Q-8 Repeated `.extern` or `.entry` for the same symbol.** Settled: a
  symbol both defined in the file and declared `.extern` is an error
  (DIR-30). Still open: `.extern X` twice, `.entry X` twice (error, warning or
  silently accepted). The skeleton algorithm (p.35 step 10) has no check.
- **Q-9 Reserved words.** Settled: `mcro` and `mcroend` are reserved
  (LBL-14); `r20`-style names are ordinary label names (LBL-16). Still open:
  whether directive names without the dot (`data`, `string`, `mat`, `entry`,
  `extern`) are reserved; whether `PSW` and `PC` are reserved.
- **Q-10 A macro name inside the body of another macro.** Settled: a call is
  the name alone on a line (MAC-3) and no label may precede it (MAC-21). Still
  open: whether a call to an earlier macro inside a later macro's body is
  expanded. A student wrote that it need not be supported; no moderator reply.
- **Q-11 Remaining macro-name details.** Settled: label rules plus `_`, at
  most 30 characters (MAC-17). Still open: whether `_` may be the first
  character ("valid like a label name" suggests a letter first); whether a
  register name is forbidden (the booklet's "an instruction name and the
  like", p.25); how a macro name equal to a label defined *earlier* in the
  file is caught, since labels are collected only in the first pass (LBL-8
  requires the check, the stage is not stated).
- **Q-14 Edge cases of label definitions.** Settled: whitespace after the
  colon is mandatory (LBL-18). Still open: whether a label alone on a line is
  legal; whether a label on a line that fails with another error still enters
  the symbol table.
- **Q-16 Command-line edge cases.** Settled: no arguments is not an error
  (CLI-5); a file that does not open is an error (CLI-6). Still open: an
  argument given with the `.as` suffix already attached (reading CLI-6
  literally, `x.as.as` is tried and fails); the exit status of the program.
- **Q-17 Empty string.** Settled: characters 32..126 only, inner quotation
  marks and backslashes are ordinary characters (SYN-22). Still open: whether
  `.string ""` is legal (it would allocate only the terminator word).
- **Q-21 Duplicate macro names: two moderator answers.** "Should an error be
  output if several macros have the same name?" was answered "Yes"
  [forum: Do We Need to Check Multiple Definitions of the Same Macro?,
  08/06/2025]. Later, "do we need to check for collisions in macro names?" was
  answered "there is no need to check for duplicate macro names" [forum: Macro
  name, 22/07/2025]. (A third post, "Of course", answers a multi-part question
  that included duplicates [forum: Whitespace in Macros, 18/06/2025].) SPEC
  applies the latest (MAC-15). Options for the project: report an error
  (allowed by both, required by the first); or do not check, in which case the
  behaviour for a second definition (first wins, last wins) is undefined.
- **Q-22 Label in front of `mcro`, `mcroend` or a macro call.** The moderator
  answered "No" to "can such situations exist?" [forum: Label at the
  beginning/end of macro definition, 22/07/2025]. Readings: (a) such input
  never occurs and need not be handled; (b) such input is illegal and must be
  reported. Under SPEC as written, `label: mcro x` is not recognised as a
  definition and `label: a_mc` is not a call (MAC-3), so the first pass
  rejects both lines as unknown operations.
- **Q-24 The prn word in the worked examples.** `bcdb dbaaa` (opcode 13) is
  what correction 1 implies and what a student states in the corrections
  thread; the direct question to the moderator was not answered
  [forum: Clarifications and Corrections, 14/07/2025, thread]. There is no
  reading of the sources under which `daaaa` is right, but it is recorded here
  because the confirmation is not a ruling.
- **Q-25 Global const tables.** The moderator ruled "do not define arrays as
  global" in reply to a question about a read-only instruction table (IMP-1).
  `CLAUDE.md` allows const tables at file scope. The two disagree; this is a
  coding-rule decision for the owner, with no effect on the assembler's
  output.

### 6.2 Resolved since the first version of this file

| Q | Question | Resolution |
|---|---|---|
| Q-1 | Initial IC and load address (p.33 says 100, p.35 says 0). | 100. ALG-2 [forum: First Address for Loading into Memory, 08/06/2025] |
| Q-2 | Line number in error messages: `.as` or `.am`? | Either is accepted. Section 2.14 [forum: Printing Errors with Line Number in Case of Syntax Error in File, 02/07/2025] |
| Q-5 | Program larger than memory. | Error; keep looking for more errors. ALG-15 [forum: Memory overflow, 14/07/2025] |
| Q-12 | Does the pre-assembler check errors (p.25 against p.34)? Is `.am` left after a failure? | It checks the macro name and the `mcro` / `mcroend` lines, finds all such errors, and leaves no `.am`. MAC-18, MAC-22, MAC-23 |
| Q-13 | Where the 80-character limit is checked. | Implementer's choice; the error must be reported and scanning continues. SYN-2a, C-2 |
| Q-15 | Comments after leading whitespace or after a statement. | Not comments; `;` must be the first character of the line. SYN-9 |
| Q-18 | Attribute of a `.mat` symbol (blank on p.37). | "data". DIR-16 |
| Q-20 | Unverified owner-reported rulings. | Verified; see section 2.13. |

### 6.3 Slips in the booklet (not questions)

Recorded so that nobody changes the code to match them. p.26: "there are four
kinds of directive statements", followed by five. p.28: the label definition
sentence lists only `.data` and `.string` (p.29 adds `.mat`). p.29: "the three
bits in the A,R,E field", although the field has two bits. p.21: "in the six
bits 6-9" (fixed by correction 4). p.34: "the pre-assembler stage contains no
error detection stage" (narrowed by MAC-18). p.35: "starting at address 0" and
"IC ← 0" (overridden by ALG-2). p.37: blank attribute for `M1` (DIR-16).
p.23: jsr pushes "PC+2" regardless of the instruction length (run-time
semantics, outside the assembler).
