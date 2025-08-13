#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Boolean type for C90 compatibility */
typedef enum
{
    FALSE = 0,
    TRUE = 1
} Boolean;

/* Error types */
typedef enum
{
    NO_ERROR = 0,
    LINE_TOO_LONG,
    INVALID_MACRO_NAME,
    LABEL_ON_MACRO_LINE,
    EXTRANEOUS_TEXT,
    MEMORY_ALLOCATION_ERROR,
    FILE_ERROR,
    MACRO_NOT_CLOSED,
    DUPLICATE_MACRO_NAME,
    MACRO_ERROR,
    INVALID_LABEL_NAME,
    RESERVED_WORD,
    DUPLICATE_LABEL_NAME,
    SYNTAX_ERROR
} ErrorType;

#define MAX_LINE_LENGTH 82
#define MAX_LABEL_LENGTH 31
#define MAX_FILENAME_LENGTH 256

/* Error handling */
void print_error(ErrorType error_type, int line_number, const char *message);

/* File extensions */
/* File extensions */
#define AS_EXTENSION ".as"
#define AM_EXTENSION ".am"
#define OB_EXTENSION ".ob"
#define ENT_EXTENSION ".ent"
#define EXT_EXTENSION ".ext"

/* Public API */
Boolean process_files(int argc, char *argv[]);
Boolean process_single_file(const char *filename);

#endif /* ASSEMBLER_H */