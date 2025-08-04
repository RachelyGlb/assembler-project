#include "assembler.h"

/* Pre-process a single file - main pre-assembler function */
Boolean preprocess_file(const char *input_filename, const char *output_filename) {
    FILE *input_file = NULL;
    FILE *output_file = NULL;
    MacroTable *macro_table = NULL;
    char line[MAX_LINE_LENGTH + 2]; /* +2 for \n and \0 */
    int line_number = 0;
    Boolean has_macros = FALSE;
    Boolean has_errors = FALSE;
    Boolean in_macro_definition = FALSE;
    char current_macro_name[MAX_LABEL_LENGTH + 1];
    char **macro_content = NULL;
    int macro_line_count = 0;
    int macro_capacity = 0;
    char *trimmed;
    char *macro_name;
    char *after_name;
    MacroNode *macro;
    int i;
    
    /* Open input file */
    input_file = fopen(input_filename, "r");
    if (!input_file) {
        print_error(FILE_ERROR, 0, input_filename);
        return FALSE;
    }
    
    /* Create macro table */
    macro_table = create_macro_table();
    if (!macro_table) {
        print_error(MEMORY_ALLOCATION_ERROR, 0, input_filename);
        fclose(input_file);
        return FALSE;
    }
    
    /* First pass - collect macros and check for errors */
    while (fgets(line, sizeof(line), input_file)) {
        line_number++;
        
        /* Check line length */
        if (is_line_too_long(line)) {
            print_error(LINE_TOO_LONG, line_number, input_filename);
            has_errors = TRUE;
            continue;
        }
        
        /* Skip empty lines and comments in first pass */
        if (is_empty_line(line) || is_comment_line(line)) {
            continue;
        }
        
        /* Process macro-related lines */
        if (!in_macro_definition) {
            /* Check for macro start */
            trimmed = trim_whitespace(line);
            if (strncmp(trimmed, MACRO_START, strlen(MACRO_START)) == 0) {
                has_macros = TRUE;
                in_macro_definition = TRUE;
                
                /* Extract macro name */
                macro_name = trimmed + strlen(MACRO_START);
                macro_name = trim_whitespace(macro_name);
                
                /* Validate macro name */
                if (!is_valid_label_name(macro_name) || is_reserved_word(macro_name)) {
                    print_error(INVALID_MACRO_NAME, line_number, input_filename);
                    has_errors = TRUE;
                    in_macro_definition = FALSE;
                    continue;
                }
                
                /* Check for extraneous text after macro name */
                after_name = macro_name + strlen(macro_name);
                after_name = trim_whitespace(after_name);
                if (strlen(after_name) > 0) {
                    print_error(EXTRANEOUS_TEXT, line_number, input_filename);
                    has_errors = TRUE;
                    in_macro_definition = FALSE;
                    continue;
                }
                
                /* Initialize macro collection */
                strcpy(current_macro_name, macro_name);
                macro_line_count = 0;
                macro_capacity = 10;
                macro_content = malloc(macro_capacity * sizeof(char*));
                if (!macro_content) {
                    print_error(MEMORY_ALLOCATION_ERROR, line_number, input_filename);
                    has_errors = TRUE;
                    in_macro_definition = FALSE;
                    continue;
                }
            }
            /* Check for macro call */
            else if (is_macro_name(macro_table, trimmed)) {
                /* Check for extraneous text after macro call */
                after_name = trimmed + strlen(trimmed);
                after_name = trim_whitespace(after_name);
                if (strlen(after_name) > 0) {
                    print_error(EXTRANEOUS_TEXT, line_number, input_filename);
                    has_errors = TRUE;
                }
            }
        }
        else {
            /* Inside macro definition */
            trimmed = trim_whitespace(line);
            
            /* Check for macro end */
            if (strcmp(trimmed, MACRO_END) == 0) {
                in_macro_definition = FALSE;
                
                /* Add macro to table */
                if (!add_macro(macro_table, current_macro_name, macro_content, macro_line_count)) {
                    print_error(MEMORY_ALLOCATION_ERROR, line_number, input_filename);
                    has_errors = TRUE;
                }
                
                macro_content = NULL; /* Ownership transferred to macro table */
            }
            else {
                /* Add line to macro content */
                if (macro_line_count >= macro_capacity) {
                    macro_capacity *= 2;
                    macro_content = realloc(macro_content, macro_capacity * sizeof(char*));
                    if (!macro_content) {
                        print_error(MEMORY_ALLOCATION_ERROR, line_number, input_filename);
                        has_errors = TRUE;
                        in_macro_definition = FALSE;
                        continue;
                    }
                }
                
                macro_content[macro_line_count] = malloc(strlen(line) + 1);
                if (!macro_content[macro_line_count]) {
                    print_error(MEMORY_ALLOCATION_ERROR, line_number, input_filename);
                    has_errors = TRUE;
                    continue;
                }
                strcpy(macro_content[macro_line_count], line);
                macro_line_count++;
            }
        }
    }
    
    /* Check if macro was not closed */
    if (in_macro_definition) {
        print_error(MACRO_NOT_CLOSED, line_number, input_filename);
        has_errors = TRUE;
    }
    
    /* If errors found, don't create output file */
    if (has_errors) {
        cleanup_and_exit(macro_table, input_file, NULL);
        return FALSE;
    }
    
    /* Second pass - generate output file */
    rewind(input_file);
    
    output_file = fopen(output_filename, "w");
    if (!output_file) {
        print_error(FILE_ERROR, 0, output_filename);
        cleanup_and_exit(macro_table, input_file, NULL);
        return FALSE;
    }
    
    line_number = 0;
    in_macro_definition = FALSE;
    
    while (fgets(line, sizeof(line), input_file)) {
        line_number++;
        
        /* Skip empty lines and comments if file has macros */
        if (has_macros && (is_empty_line(line) || is_comment_line(line))) {
            continue;
        }
        
        /* If no macros, copy everything as-is */
        if (!has_macros) {
            fputs(line, output_file);
            continue;
        }
        
        /* Process macro-related lines */
        if (!in_macro_definition) {
            trimmed = trim_whitespace(line);
            
            /* Skip macro definitions */
            if (strncmp(trimmed, MACRO_START, strlen(MACRO_START)) == 0) {
                in_macro_definition = TRUE;
                continue;
            }
            
            /* Expand macro calls */
            macro = find_macro(macro_table, trimmed);
            if (macro) {
                for (i = 0; i < macro->line_count; i++) {
                    fputs(macro->content[i], output_file);
                }
                continue;
            }
            
            /* Regular line - copy as-is */
            fputs(line, output_file);
        }
        else {
            /* Inside macro definition - check for end */
            trimmed = trim_whitespace(line);
            if (strcmp(trimmed, MACRO_END) == 0) {
                in_macro_definition = FALSE;
            }
            /* Skip all macro definition lines */
        }
    }
    
    /* Cleanup */
    fclose(input_file);
    fclose(output_file);
    free_macro_table(macro_table);
    
    return TRUE;
}
