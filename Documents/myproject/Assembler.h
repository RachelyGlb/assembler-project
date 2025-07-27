#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Constants */
#define MAX_LINE_LENGTH 80
#define MAX_LABEL_LENGTH 30
#define MAX_FILENAME_LENGTH 256
#define MEMORY_SIZE 256

/* File extensions */
#define AS_EXTENSION ".as"
#define AM_EXTENSION ".am"
#define OB_EXTENSION ".ob"
#define ENT_EXTENSION ".ent"
#define EXT_EXTENSION ".ext"

/* Macro keywords */
#define MACRO_START "mcro"
#define MACRO_END "mcroend"

/* Boolean definitions */
typedef enum {
    FALSE = 0,
    TRUE = 1
} Boolean;

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
    DUPLICATE_MACRO_NAME
} ErrorType;

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

/* Function prototypes */

/* Pre-assembler functions */
Boolean process_files(int argc, char *argv[]);
Boolean process_single_file(const char *filename);
Boolean preprocess_file(const char *input_filename, const char *output_filename);

/* Macro handling functions */
MacroTable* create_macro_table(void);
void free_macro_table(MacroTable *table);
Boolean add_macro(MacroTable *table, const char *name, char **content, int line_count);
MacroNode* find_macro(MacroTable *table, const char *name);
Boolean is_macro_name(MacroTable *table, const char *name);

/* Line processing functions */
Boolean is_empty_line(const char *line);
Boolean is_comment_line(const char *line);
Boolean is_line_too_long(const char *line);
char* trim_whitespace(char *str);
Boolean is_valid_label_name(const char *name);
Boolean is_reserved_word(const char *word);

/* Utility functions */
void print_error(ErrorType error, int line_number, const char *filename);
Boolean file_exists(const char *filename);
char* create_filename_with_extension(const char *base_name, const char *extension);

/* Memory management */
void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output);

#endif /* ASSEMBLER_H */