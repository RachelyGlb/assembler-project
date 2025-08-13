#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "first_pass.h"
#include "line_analysis.h"
#include "symbol_table.h"


/* הדפסת שגיאה נוחה */
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

/* בסוף pass1 מוסיפים ICF לכתובות של סמלי DATA */
static void adjust_data_addresses_with_icf(int icf) {
    for (Symbol *s = symbol_table_head; s; s = s->next) {
        if (s->type == DATA) {
            s->address += icf;
        }
    }
}

void first_pass(const char *filename) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    int IC = 100;   /* Instruction Counter */
    int DC = 0;     /* Data Counter */
    bool has_errors = false;

    FILE *inputFile = fopen(filename, "r");
    if (!inputFile) {
        perror("Error opening input file");
        return;
    }

    printf("Opened file '%s' successfully!\n", filename);

    printf("Starting first pass...\n");

    while (fgets(line, sizeof(line), inputFile)) {
        char label[MAX_LABEL_LENGTH] = "";
        line_number++;

        if (is_comment_or_empty(line)) {
            continue;
        }

        printf("Line %d: %s", line_number, line);

        /* תווית בתחילת השורה? */
        bool has_label = extract_label(line, label);
        if (has_label) {
            printf("  -> Label found: %s\n", label);

            if (!is_valid_label(label)) {
                error(line_number, "Invalid label name: %s", label);
                has_errors = true;
                continue;
            }

            /* כפילות "חכמה": אם יש placeholder מ-.entry (address==0) לא נזרוק שגיאה.
               אם extern – אסור להגדיר מקומית; אם כבר עם כתובת – כפילות. */
            Symbol *existing = find_symbol(label);
            if (existing) {
                if (existing->type == EXTERN_SYM) {
                    error(line_number, "Label '%s' declared extern earlier", label);
                    has_errors = true;
                    continue;
                }
                if (existing->address != 0) {
                    error(line_number, "Duplicate label: %s", label);
                    has_errors = true;
                    continue;
                }
            }
        }

        /* ===== .entry ===== */
        if (strstr(line, ".entry")) {
            char entry_label[MAX_LABEL_LENGTH];
            if (sscanf(line, "%*s %30s", entry_label) == 1) {
                if (!add_label_to_table(entry_label, 0, CODE, line_number, true)) {
                    has_errors = true;
                }
            } else {
                error(line_number, "Invalid .entry directive");
                has_errors = true;
            }
            continue;
        }

        /* ===== .extern ===== */
        if (strstr(line, ".extern")) {
            char extern_label[MAX_LABEL_LENGTH];
            if (sscanf(line, "%*s %30s", extern_label) == 1) {
                if (!add_label_to_table(extern_label, 0, EXTERN_SYM, line_number, false)) {
                    has_errors = true;
                }
            } else {
                error(line_number, "Invalid .extern directive");
                has_errors = true;
            }
            continue;
        }

        /* ===== הנחיות נתונים ===== */
        if (is_data_or_string(line)) {
            printf("  -> This line is a data or string directive.\n");

            if (has_label) {
                if (!add_label_to_table(label, DC, DATA, line_number, false)) {
                    has_errors = true;
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
        /* ===== פקודות קוד ===== */
        else if (is_command(line)) {
            printf("  -> This line is a command.\n");

            if (has_label) {
                if (!add_label_to_table(label, IC, CODE, line_number, false)) {
                    has_errors = true;
                    continue;
                }
            }

            /* מצביע לתוכן אחרי התווית (אם יש) */
            const char *line_for_validation = line;
            if (has_label) {
                const char *colon = strchr(line, ':');
                if (colon) {
                    line_for_validation = colon + 1;
                    while (*line_for_validation && isspace((unsigned char)*line_for_validation))
                        line_for_validation++;
                }
            }

            /* נספור תמיד את המילים ונעדכן IC גם אם הוולידציה נכשלת */
            int words = count_command_words(has_label ? line_for_validation : line);

            if (!validate_command_line(line_for_validation, line_number)) {
                has_errors = true;
                IC += words;      /* נשמור עקביות כתובות */
                continue;
            }

            IC += words;          /* תקין – מקדמים רגיל */
        }
        
    }

    /* ==== סוף המעבר הראשון: עדכון סמלי DATA עם ICF ==== */
    const int ICF = IC; /* Instruction Counter Final */
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
