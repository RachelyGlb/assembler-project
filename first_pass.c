#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "first_pass.h"
#include "line_analysis.h"
#include "symbol_table.h"
#include "instruction_validation.h"


void error(int line_number, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (line_number > 0)
    {
        fprintf(stderr, "Error on line %d: ", line_number);
    }
    else
    {
        fprintf(stderr, "Error: ");
    }

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static void adjust_data_addresses_with_icf(int icf) {
    Symbol *s;
    for (s = symbol_table_head; s; s = s->next) {
        if (s->type == DATA) {
            s->address += icf;
        }
    }
}

void first_pass(const char *filename) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    int IC = 100;   /* Instruction Counter */
    int DC = 0;  /* Data Counter */
    int ICF = 0;     
    int has_errors = 0;

    FILE *inputFile = fopen(filename, "r");
    if (!inputFile) {
        perror("Error opening input file");
        return;
    }

    printf("Opened file '%s' successfully!\n", filename);

    printf("Starting first pass...\n");

    while (fgets(line, sizeof(line), inputFile)) {
        char label[MAX_LABEL_LENGTH] = "";
        int has_label = 0;
        const char *line_for_validation = NULL;
        int words = 0;
        Symbol *existing; 

        line_number++;

        if (is_comment_or_empty(line)) {
            continue;
        }

        printf("Line %d: %s", line_number, line);


        has_label = extract_label(line, label);
        if (has_label) {
            printf("  -> Label found!!!!: %s\n", label);
            /*printf("[DBG] label='%s' first=%d '%c'\n",
       label, (unsigned char)label[0], label[0]);*/


            if (!is_valid_label(label)) {
                printf("  !!!!!");
                error(line_number, "Invalid label name: %s", label);
                has_errors = 1;
                continue;
            }

            existing = find_symbol(label);
            if (existing) {
                if (existing->type == EXTERN_SYM) {
                    error(line_number, "Label '%s' declared extern earlier", label);
                    has_errors = 1;
                    continue;
                }
                if (existing->address != 0) {
                    error(line_number, "Duplicate label: %s", label);
                    has_errors = 1;
                    continue;
                }
            }
        }

        /* ===== .entry ===== */
        if (strstr(line, ".entry")) {
            char entry_label[MAX_LABEL_LENGTH];
            if (sscanf(line, "%*s %30s", entry_label) == 1) {
                if (!add_symbol_to_table(entry_label, 0, CODE, line_number, 1)) {
                    has_errors = 1;
                }
            } else {
                error(line_number, "Invalid .entry directive");
                has_errors = 1;
            }
            continue;
        }

        /* ===== .extern ===== */
        if (strstr(line, ".extern")) {
            char extern_label[MAX_LABEL_LENGTH];
            if (sscanf(line, "%*s %30s", extern_label) == 1) {
                if (!add_symbol_to_table(extern_label, 0, EXTERN_SYM, line_number, 0)) {
                    has_errors = 1;
                }
            } else {
                error(line_number, "Invalid .extern directive");
                has_errors = 1;
            }
            continue;
        }

        /* ===== Data ===== */
        if (is_data_or_string(line)) {
            printf("  -> This line is a data or string directive.\n");

            if (has_label) {
                if (!add_symbol_to_table(label, DC, DATA, line_number, 0)) {
                    has_errors = 1;
                    continue;
                }
            }

            if (strstr(line, ".data")) {
                DC += count_data_items(line);
            } else if (strstr(line, ".string")) {
                DC += count_string_length(line);
            } else if (strstr(line, ".mat")) {
                DC += count_matrix_items(line);
            }
        }
        /* ===== inst code ===== */
        else if (is_command(line)) {
            printf("  -> This line is a command.\n");

            if (has_label) {
                if (!add_symbol_to_table(label, IC, CODE, line_number, 0)) {
                    has_errors = 1;
                    continue;
                }
            }

            line_for_validation = line;
            if (has_label) {
                const char *colon = strchr(line, ':');
                if (colon) {
                    line_for_validation = colon + 1;
                    while (*line_for_validation && isspace((unsigned char)*line_for_validation))
                        line_for_validation++;
                }
            }

           
            words = count_command_words(has_label ? line_for_validation : line);

            if (!validate_command_line(line_for_validation, line_number)) {
                has_errors = 1;
                IC += words;     
                continue;
            }

            IC += words;         
        }else {
            has_errors= 1;
        }

        
    }

    ICF = IC; /* Instruction Counter Final */
    adjust_data_addresses_with_icf(ICF);

    printf("First pass completed.\n");
    if (has_errors) {
        printf("Errors found during first pass.\n");
    } else {
        printf("No errors found in first pass.\n");
    }
    printf("Final IC = %d (ICF)\n", IC);
    printf("Final DC = %d\n", DC);

    /* במידת הצורך: אפשר להדפיס כאן את טבלת הסמלים
       print_symbol_table();
    */
}
