#ifndef PREASSEMBLER_H
#define PREASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Constants */
#define MAX_LINE_LENGTH 82
#define MAX_LABEL_LENGTH 31
#define MAX_FILENAME_LENGTH 256
#define MAX_MACRO_LINES 50
#define MEMORY_SIZE 256

/* Boolean type for C90 compatibility */
typedef enum {
    FALSE = 0,
    TRUE = 1
} Boolean;

/* File extensions */
#define AS_EXTENSION ".as"
#define AM_EXTENSION ".am"

/* Macro keywords */
#define MACRO_START "mcro"
#define MACRO_END "mcroend"

/* Macro structure */
typedef struct MacroNode {
    char *name;
    char **content;
    int line_count;
    struct MacroNode *next;
} MacroNode;

/* Macro table */
typedef struct {
    MacroNode *head;
    int count;
} MacroTable;

/* Error types */
typedef enum {
    NO_ERROR = 0,
    LINE_TOO_LONG,
    INVALID_MACRO_NAME,
    LABEL_ON_MACRO_LINE,
    EXTRANEOUS_TEXT,
    MEMORY_ALLOCATION_ERROR,
    FILE_ERROR,
    MACRO_NOT_CLOSED,
    DUPLICATE_MACRO_NAME,
    MACRO_ERROR
} ErrorType;

/* Function prototypes */

/* Utility functions */

void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output);
Boolean file_exists(const char *filename);
char* create_filename_with_extension(const char *base_name, const char *extension);


/* Macro table functions */
MacroTable* create_macro_table(void);
void free_macro_table(MacroTable *table);
Boolean add_macro(MacroTable *table, const char *name, char **content, int line_count);
MacroNode* find_macro(MacroTable *table, const char *name);
Boolean is_macro_name(MacroTable *table, const char *name);
Boolean has_extraneous_text_after_second_word(const char *line);


/* String processing functions */
char* get_first_word(const char *line);
char* get_second_word(const char *line);

/* Main preassembler function */
Boolean preassembler(const char* filename);

/* Error handling */
void print_error(ErrorType error_type, int line_number, const char *message);

/* Line processing functions */
Boolean is_empty_line(const char *line);
Boolean is_comment_line(const char *line);
Boolean is_line_too_long(const char *line, int line_number, FILE *input);
char* trim_whitespace(char *str);
Boolean is_valid_macro_len(const char *name);
Boolean is_reserved_word(const char *word);

#endif /* PREASSEMBLER_H */