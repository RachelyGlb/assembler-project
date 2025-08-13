#ifndef PREASSEMBLER_H
#define PREASSEMBLER_H

#include "assembler.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* Constants */

/* Macro keywords */
#define MACRO_START "mcro"
#define MACRO_END "mcroend"

/* Generic structures for universal table handling */
typedef struct GenericNode
{
    char *name;
    int line_number;
    void *data;
    struct GenericNode *next;
} GenericNode;

typedef struct
{
    GenericNode *head;
    int count;
} GenericTable;

/* Macro-specific data structure */
typedef struct
{
    char **content;
    int line_count;
} MacroData;

/* Type aliases for clarity */
typedef GenericTable MacroTable;
typedef GenericTable LabelTable;
typedef GenericNode MacroNode;
typedef GenericNode LabelNode;

/* FUNCTION PROTOTYPES */

/* Generic table functions */
GenericTable *create_generic_table(void);
void free_generic_table(GenericTable *table, void (*cleanup_data)(void *));
Boolean add_to_generic_table(GenericTable *table, const char *name, int line_number, void *data);
GenericNode *find_in_generic_table(GenericTable *table, const char *name);

/* Specific table implementations */
GenericTable *create_macro_table(void);
void free_macro_table(GenericTable *table);
Boolean add_macro(GenericTable *table, const char *name, char **content, int line_count);
GenericNode *find_macro(GenericTable *table, const char *name);
Boolean is_macro_name(GenericTable *table, const char *name);

GenericTable *create_label_table(void);
void free_label_table(GenericTable *table);
Boolean add_label_to_table(GenericTable *table, const char *name, int line_number);
Boolean is_label_already_defined(GenericTable *table, const char *name);

/* String utilities */
char *extract_until_char(const char *line, char delimiter);
char *get_content_after_char(const char *line, char delimiter);
Boolean is_valid_name(const char *name, Boolean allow_underscore);

/* Label processing functions */
Boolean has_label(const char *line);
char *extract_label_name(const char *line);
char *get_content_after_label(const char *line);
Boolean is_valid_label_name(const char *name);
Boolean is_valid_macro_name(const char *name);

/* Basic string processing functions */
char *get_first_word(const char *line);
char *get_second_word(const char *line);
Boolean has_extraneous_text_after_words(const char *line, int expected_words);

/* File utilities */
Boolean file_exists(const char *filename);
void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output);
void cleanup_macro_lines(char **lines, int count);
char *create_filename_with_extension(const char *base_name, const char *extension);

/* Line processing functions */
Boolean is_empty_line(const char *line);
Boolean is_comment_line(const char *line);
Boolean is_line_too_long(const char *line, int line_number, FILE *input);
char *trim_whitespace(char *str);
Boolean is_reserved_word(const char *word);

/* Main preassembler function */
Boolean preassembler(const char *filename);

/* Error handling macros */
#define REPORT_CRITICAL_ERROR_AND_EXIT(error_type, line_num, msg, ptr1, ptr2) \
    print_error(error_type, line_num, msg);                                   \
    if (ptr1)                                                                 \
    {                                                                         \
        free(ptr1);                                                           \
    }                                                                         \
    if (ptr2)                                                                 \
    {                                                                         \
        free(ptr2);                                                           \
    }                                                                         \
    exit(1)

#define REPORT_ERROR_AND_CONTINUE(error_type, line_num, msg, ptr1, ptr2) \
    print_error(error_type, line_num, msg);                              \
    has_errors = TRUE;                                                   \
    if (ptr1)                                                            \
    {                                                                    \
        free(ptr1);                                                      \
    }                                                                    \
    if (ptr2)                                                            \
    {                                                                    \
        free(ptr2);                                                      \
    }                                                                    \
    continue

#define REPORT_ERROR_AND_BREAK(error_type, line_num, msg, ptr1, ptr2) \
    print_error(error_type, line_num, msg);                           \
    has_errors = TRUE;                                                \
    if (ptr1)                                                         \
    {                                                                 \
        free(ptr1);                                                   \
    }                                                                 \
    if (ptr2)                                                         \
    {                                                                 \
        free(ptr2);                                                   \
    }                                                                 \
    break

#define REPORT_ERROR_ONLY(error_type, line_num, msg, ptr1, ptr2) \
    print_error(error_type, line_num, msg);                      \
    has_errors = TRUE;                                           \
    if (ptr1)                                                    \
    {                                                            \
        free(ptr1);                                              \
    }                                                            \
    if (ptr2)                                                    \
    {                                                            \
        free(ptr2);                                              \
    }

#endif /* PREASSEMBLER_H */