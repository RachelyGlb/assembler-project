#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preassembler.h"



/* File extensions */
#define OB_EXTENSION ".ob"
#define ENT_EXTENSION ".ent"
#define EXT_EXTENSION ".ext"

/* Function prototypes */

/* Pre-assembler functions */
Boolean process_files(int argc, char *argv[]);
Boolean process_single_file(const char *filename);
Boolean preprocess_file(const char *input_filename, const char *output_filename);


/* Memory management */
void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output);

#endif /* ASSEMBLER_H */