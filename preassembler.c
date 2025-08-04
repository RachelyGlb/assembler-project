#include "preassembler.h"

Boolean preassembler(const char* filename) {

    /* File handling setup */
    char *input_name = create_filename_with_extension(filename, AS_EXTENSION);
    FILE *input = fopen(input_name, "r");
    char *output_name = create_filename_with_extension(filename, AM_EXTENSION);
    FILE *output = fopen(output_name, "w");
    
    MacroTable *macro_table = create_macro_table();

    /* Processing state variables */
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    Boolean inside_macro = FALSE;           /* Track if we're inside macro definition */
    Boolean has_errors = FALSE;
    
    /* Temporary storage for current macro being defined */
    char current_macro_name[MAX_LABEL_LENGTH];
    char macro_lines[MAX_MACRO_LINES][MAX_LINE_LENGTH];
    int macro_line_count = 0;

    char *first_word = NULL;
    char *macro_name = NULL;
    char **macro_content = NULL;  /* This will be dynamically allocated for each macro */
    MacroNode *macro = NULL;

    int i, j, c,k;
    char *ptr,*word; 
    


    if(!input) {
        print_error(FILE_ERROR, 0, input_name);
        free(input_name);
        return FALSE;
    }
 
    if(!output) {
        print_error(FILE_ERROR, 0, output_name);
        free(output_name);
        fclose(input);
        return FALSE;
    }

    /* Initialize macro table */
    if(!macro_table) {
        print_error(MEMORY_ALLOCATION_ERROR, 0, filename);
        fclose(input);
        fclose(output);
        free(input_name);
        free(output_name);
        return FALSE;
    }

     
    /* Main processing loop - read line by line */
    while (fgets(line, sizeof(line), input) != NULL) {
        
        line_number++;

        if(is_line_too_long(line,line_number,input))
        {
            has_errors = TRUE;
        }

        while ((c = fgetc(input)) != '\n' && c != EOF);
        continue;
        

        /* Parse first word of line to determine action */
        first_word = get_first_word(line);  /* Fixed: removed * */
        
        if ((!first_word||is_empty_line(line))&&!inside_macro) {
            /* Empty line - copy to output only if not inside macro definition */
                fputs(line, output);
        }
        
        /* STATE 1: Check for macro definition start */
        if (strcmp(first_word, MACRO_START) == 0&&!inside_macro){
            
            /* Extract macro name from line */
            macro_name = get_second_word(line);  /* Fixed: removed  */
            if (!macro_name) {
                print_error(MACRO_ERROR, line_number, "Missing macro name");
                has_errors = TRUE;
                free(first_word);

                /* Skip until mcroend */
                while (fgets(line, MAX_LINE_LENGTH, input)) {
                    line_number++;
                    word = get_first_word(line);

                    if (word && strcmp(word, "mcroend") == 0) {
                        free(word);
                        break;
                    }
                    free(word);
                }
                continue;
            }

            ptr = strstr(line, macro_name);

            if(ptr) {
                ptr += strlen(macro_name);  /* Move past macro name */

                if(!is_empty_line(ptr)){
                    print_error(MACRO_ERROR, line_number, "Extra text after macro name");
                    has_errors = TRUE;
                    free(first_word);
                    free(macro_name);
                    continue;
                }
            }
    
            /* Validate macro name according to project rules */
            if (!is_valid_macro_len(macro_name)) {
                print_error(MACRO_ERROR, line_number, "Invalid macro name");
                has_errors = TRUE;
                free(first_word);
                free(macro_name);
                continue;
            }
            
            if (is_reserved_word(macro_name)) {
                print_error(MACRO_ERROR, line_number, "Macro name is reserved word");
                has_errors = TRUE;
                free(first_word);
                free(macro_name);
                continue;
            }
            
            /* Start macro collection mode */
            strcpy(current_macro_name, macro_name);
            inside_macro = TRUE;
            macro_line_count = 0;
            
            free(first_word);
            free(macro_name);
            continue;
        }

        /* STATE 2: Check for macro definition end */
        if (strcmp(first_word, MACRO_END) == 0) {
            if (!inside_macro) {
                print_error(MACRO_ERROR, line_number, "Unexpected macro end");
                has_errors = TRUE;
                free(first_word);
                continue;
            }

            /* Prepare macro content for storage - FIXED ALLOCATION */
            macro_content = malloc(macro_line_count * sizeof(char*));  /* Fixed: removed ** */
            if (!macro_content) {
                print_error(MEMORY_ALLOCATION_ERROR, line_number, "Failed to allocate macro");
                has_errors = TRUE;
                free(first_word);
                break;
            }
            
            /* Create array of pointers to macro lines - FIXED ALLOCATION */
            for (i = 0; i < macro_line_count; i++) {
                macro_content[i] = malloc(strlen(macro_lines[i]) + 1);
                if (!macro_content[i]) {
                    /* Clean up on allocation failure */
                    for (k = 0; k < i; k++) {
                        free(macro_content[k]);
                    }
                    free(macro_content);
                    print_error(MEMORY_ALLOCATION_ERROR, line_number, "Failed to allocate macro line");
                    has_errors = TRUE;
                    free(first_word);
                    break;
                }
                strcpy(macro_content[i], macro_lines[i]);
            }
            
            /* Add completed macro to table only if allocation succeeded */
            if (!has_errors) {
                if (!add_macro(macro_table, current_macro_name, macro_content, macro_line_count)) {
                    print_error(MACRO_ERROR, line_number, "Failed to add macro");
                    has_errors = TRUE;
                    /* Clean up allocated content */
                    for (i = 0; i < macro_line_count; i++) {
                        free(macro_content[i]);
                    }
                }
            }
            
            /* Clean up temporary allocation - the add_macro function copies the content */
            if (macro_content) {
                for (i = 0; i < macro_line_count; i++) {
                    if (macro_content[i]) {
                        free(macro_content[i]);
                    }
                }
                free(macro_content);
                macro_content = NULL;
            }
            
            inside_macro = FALSE;
            free(first_word);
            continue;
        }
        
        /* STATE 3: Inside macro definition - collect lines */
        if (inside_macro) {
            if (macro_line_count >= MAX_MACRO_LINES) {
                print_error(MACRO_ERROR, line_number, "Macro too long");
                has_errors = TRUE;
                free(first_word);
                continue;
            }
            /* Store line in temporary macro content */
            strcpy(macro_lines[macro_line_count], line);
            macro_line_count++;
            free(first_word);
            continue;
        }
        
        /* STATE 4: Regular processing - check for macro calls or copy line */
        macro = find_macro(macro_table, first_word);  /* Fixed: removed * */
        if (macro) {
            /* Macro call found - expand it by writing all its lines */
            for (j = 0; j < macro->line_count; j++) {
                fputs(macro->content[j], output);
            }
        } else {
            /* Regular line - copy as-is to output */
            fputs(line, output);
        }
        
        free(first_word);
    }
    
    /* Cleanup resources */
    free_macro_table(macro_table);
    fclose(input);
    fclose(output);
    free(input_name);
    free(output_name);
    
    return !has_errors;
}