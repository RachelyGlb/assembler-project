/*
 * common.h - definitions shared by every module of the assembler.
 *
 * This header holds only plain definitions: the Boolean type, the limits and
 * addresses fixed by the specification, the file extensions, the keywords of
 * the macro language, the enumerations that several modules exchange and the
 * few small macros that more than one module needs. It defines no object, no
 * structure with behaviour and no function. Rule IDs in the comments refer to
 * docs/SPEC.md.
 */
#ifndef COMMON_H
#define COMMON_H

/* The truth type of the program (C90 has none of its own). */
typedef enum
{
    FALSE = 0,
    TRUE = 1
} Boolean;

/*
 * The outcome of one stage of the assembler (pre-assembler, first pass,
 * second pass, output) for one source file.
 */
typedef enum
{
    STAGE_OK,     /* The stage finished and found no error. */
    STAGE_ERRORS, /* The stage reported errors: this file stops here. */
    STAGE_FATAL   /* Memory ran out: the whole program must stop (ERR-R10). */
} StageResult;

/* The four addressing modes, numbered as in the machine word (ADR-1). */
typedef enum
{
    ADDRESSING_IMMEDIATE = 0,
    ADDRESSING_DIRECT = 1,
    ADDRESSING_MATRIX = 2,
    ADDRESSING_REGISTER = 3
} AddressingMode;

/* The number of addressing modes (ADR-1). */
#define ADDRESSING_MODE_COUNT 4

/* The longest legal source line, without its line terminator (SYN-2). */
#define MAX_SOURCE_LINE_LENGTH 80

/* The longest legal label name and macro name (LBL-3, MAC-17). */
#define MAX_NAME_LENGTH 30

/* The number of memory cells of the machine (M-4). */
#define MEMORY_SIZE 256

/* The address at which the code is loaded; the first value of IC (ALG-2). */
#define FIRST_LOAD_ADDRESS 100

/* The most words (code plus data) that a program may occupy (ALG-15). */
#define MAX_PROGRAM_WORDS (MEMORY_SIZE - FIRST_LOAD_ADDRESS)

/* The number of bits of a machine word and of a register (M-2, M-4). */
#define WORD_BITS 10

/*
 * The number of bits, bits 9-2 of an additional word, that hold an
 * immediate operand or an address (ADR-3, ADR-5).
 */
#define OPERAND_FIELD_BITS 8

/* The most operands that an instruction can have (ENC-1). */
#define MAX_OPERAND_COUNT 2

/* The number of general registers, r0 to r7 (M-2). */
#define REGISTER_COUNT 8

/* The first line of a file has this number (ERR-R3). */
#define FIRST_LINE_NUMBER 1

/* The "line number" of a diagnostic that belongs to no line (ERR-R13). */
#define NO_LINE 0

/* The extension of the source file and of the four output files (OUT-1). */
#define SOURCE_EXTENSION ".as"
#define EXPANDED_EXTENSION ".am"
#define OBJECT_EXTENSION ".ob"
#define ENTRIES_EXTENSION ".ent"
#define EXTERNALS_EXTENSION ".ext"

/* The two keywords that open and close a macro definition (MAC-2). */
#define MACRO_START_KEYWORD "mcro"
#define MACRO_END_KEYWORD "mcroend"

/* The character that makes a line a comment when it is the first (SYN-5). */
#define COMMENT_CHARACTER ';'

/* The character that ends a label definition (LBL-4). */
#define LABEL_END_CHARACTER ':'

/* The character that ends a line of a text file. */
#define NEWLINE_CHARACTER '\n'

/* The modes in which a text file is opened for reading and for writing. */
#define READ_MODE "r"
#define WRITE_MODE "w"

/* The number of elements of an array object. */
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#endif /* COMMON_H */
