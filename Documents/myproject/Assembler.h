#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



/* File extensions */
#define AS_EXTENSION ".as"
#define AM_EXTENSION ".am"
#define OB_EXTENSION ".ob"
#define ENT_EXTENSION ".ent"
#define EXT_EXTENSION ".ext"



/* Boolean definitions */
typedef enum {
    FALSE = 0,
    TRUE = 1
} Boolean;





/* Function prototypes */

/* Pre-assembler functions */
Boolean process_files(int argc, char *argv[]);
Boolean process_single_file(const char *filename);
Boolean preprocess_file(const char *input_filename, const char *output_filename);



/* Line processing functions */
Boolean is_empty_line(const char *line);
Boolean is_comment_line(const char *line);
Boolean is_line_too_long(const char *line);
char* trim_whitespace(char *str);
Boolean is_valid_macro_len(const char *name);
Boolean is_reserved_word(const char *word);

/* Utility functions */
void print_error(ErrorType error, int line_number, const char *filename);
Boolean file_exists(const char *filename);
char* create_filename_with_extension(const char *base_name, const char *extension);

/* Memory management */
void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output);

#endif /* ASSEMBLER_H */