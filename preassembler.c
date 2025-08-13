
#include "preassembler.h"

void cleanup_macro_lines(char **lines, int count)
{
    int i;
    if (!lines)
        return;

    for (i = 0; i < count; i++)
    {
        if (lines[i])
        {
            free(lines[i]);
        }
    }
    free(lines);
}

Boolean preassembler(const char *filename)
{

    /* File handling setup */
    char *input_name = create_filename_with_extension(filename, AS_EXTENSION);
    FILE *input = fopen(input_name, "r");
    char *output_name = create_filename_with_extension(filename, AM_EXTENSION);
    FILE *output = fopen(output_name, "w");

    GenericTable *macro_table = create_macro_table();
    GenericTable *label_table = create_label_table();
    MacroData *macro_data = NULL;

    /* Processing state variables */
    char line[MAX_LINE_LENGTH];
    int line_number;
    Boolean inside_macro = FALSE; /* Track if we're inside macro definition */
    Boolean inside_macro_definition = FALSE;
    Boolean has_errors = FALSE;

    /* Temporary storage for current macro being defined */
    char current_macro_name[MAX_LABEL_LENGTH];
    char **macro_lines = NULL;
    int macro_capacity = 0;
    int macro_line_count;

    char *first_word = NULL;
    char *macro_name = NULL;
    char **macro_content = NULL; /* This will be dynamically allocated for each macro */
    MacroNode *macro = NULL;

    char *label_name = NULL;
    char *content_after_label = NULL;
    char *first_word_after_label = NULL;

    int i, j, c, k;

    line_number = 0;
    macro_line_count = 0;

    if (!input)
    {
        REPORT_CRITICAL_ERROR_AND_EXIT(FILE_ERROR, 0, "Cannot open input file", input_name, NULL);
    }

    if (!output)
    {
        fclose(input);
        REPORT_CRITICAL_ERROR_AND_EXIT(FILE_ERROR, 0, "Cannot create output file", input_name, output_name);
    }

    /* Initialize macro table */
    if (!macro_table)
    {
        fclose(input);
        if (output)
            fclose(output);
        REPORT_CRITICAL_ERROR_AND_EXIT(MEMORY_ALLOCATION_ERROR, 0, "Failed to create macro table", input_name, output_name);
    }

    /* Initialize label table */
    if (!label_table)
    {
        fclose(input);
        fclose(output);
        free_macro_table(macro_table); /* Free only the successfully allocated macro_table */
        REPORT_CRITICAL_ERROR_AND_EXIT(MEMORY_ALLOCATION_ERROR, 0, "Failed to create label table", input_name, output_name);
    }

    /* Main processing loop - read line by line */
    while (fgets(line, sizeof(line), input) != NULL)
    {

        line_number++;

        if (is_line_too_long(line, line_number, input))
        {
            has_errors = TRUE;
            while ((c = fgetc(input)) != '\n' && c != EOF)
            {
                /* Skip the rest of the line */
            };
            continue;
        }

        if (is_empty_line(line) || is_comment_line(line))
        {
            continue;
        }

        /* Parse first word of line to determine action */
        first_word = get_first_word(line); /* Fixed: removed * */

        /* STATE 1: Check for macro/Label definition start */

        if (first_word && strcmp(first_word, MACRO_START) == 0)
        {
            inside_macro_definition = TRUE;

            /* Extract macro name from line */
            macro_name = get_second_word(line);
            if (!macro_name)
            {
                REPORT_ERROR_AND_CONTINUE(MACRO_ERROR, line_number, "Missing macro name", first_word, NULL);
            }

            if (has_extraneous_text_after_words(line, 2))
            {
                REPORT_ERROR_AND_CONTINUE(EXTRANEOUS_TEXT, line_number, "Extra text after macro name", first_word, macro_name);
            }

            /* Validate macro name according to project rules */
            if (!is_valid_macro_name(macro_name))
            {
                REPORT_ERROR_AND_CONTINUE(INVALID_MACRO_NAME, line_number, "Invalid macro name", first_word, macro_name);
            }

            if (is_reserved_word(macro_name))
            {
                REPORT_ERROR_AND_CONTINUE(INVALID_MACRO_NAME, line_number, "Macro name is reserved word", first_word, macro_name);
            }

            if (is_label_already_defined(label_table, macro_name))
            {
                REPORT_ERROR_AND_CONTINUE(INVALID_MACRO_NAME, line_number, "Macro name cannot be label name", first_word, macro_name);
            }

            if (find_macro(macro_table, macro_name))
            {
                REPORT_ERROR_AND_CONTINUE(DUPLICATE_MACRO_NAME, line_number, "Macro already defined", first_word, macro_name);
            }

            /* Start macro collection mode */

            if (!has_errors)
            {
                strcpy(current_macro_name, macro_name);
                inside_macro = TRUE;

                /* Clean up previous macro if exists */
                if (macro_lines)
                {
                    cleanup_macro_lines(macro_lines, macro_line_count);
                }

                macro_line_count = 0;
                macro_capacity = 10;
                macro_lines = malloc(macro_capacity * sizeof(char *));
            }

            free(first_word);
            /*if (macro_name)*/
            free(macro_name);
            continue;
        }

        /* STATE 2: Check for macro definition end */

        if (first_word && strcmp(first_word, MACRO_END) == 0)
        {
            inside_macro_definition = FALSE;

            if (has_extraneous_text_after_words(line, 1))
            {
                REPORT_ERROR_AND_CONTINUE(EXTRANEOUS_TEXT, line_number, "Extra text after mcroend", first_word, NULL);
            }

            if (inside_macro)
            {
                /* Prepare macro content for storage */
                macro_content = malloc(macro_line_count * sizeof(char *));
                if (!macro_content)
                {
                    REPORT_CRITICAL_ERROR_AND_EXIT(MEMORY_ALLOCATION_ERROR, line_number, "Failed to allocate macro", first_word, NULL);
                }

                /* Create array of pointers to macro lines */
                for (i = 0; i < macro_line_count; i++)
                {
                    macro_content[i] = malloc(strlen(macro_lines[i]) + 1);
                    if (!macro_content[i])
                    {
                        /* Clean up on allocation failure */
                        for (k = 0; k < i; k++)
                        {
                            free(macro_content[k]);
                        }
                        free(macro_content);
                        REPORT_CRITICAL_ERROR_AND_EXIT(MEMORY_ALLOCATION_ERROR, line_number, "Failed to allocate macro line", first_word, NULL);
                    }
                    strcpy(macro_content[i], macro_lines[i]);
                }

                /* Add completed macro to table */
                if (!add_macro(macro_table, current_macro_name, macro_content, macro_line_count))
                {
                    REPORT_ERROR_ONLY(MACRO_ERROR, line_number, "Failed to add macro", NULL, NULL);
                    /* Clean up allocated content */
                    for (i = 0; i < macro_line_count; i++)
                    {
                        free(macro_content[i]);
                    }
                    free(macro_content);
                    free(first_word);
                    continue;
                }

                /* Clean up temporary allocation */
                for (i = 0; i < macro_line_count; i++)
                {
                    free(macro_content[i]);
                }
                free(macro_content);
                macro_content = NULL;
            }

            inside_macro = FALSE;
            free(first_word);
            continue;
        }

        if (inside_macro_definition && !inside_macro)
        {
            free(first_word);
            continue;
        }

        if (has_label(line))
        {
            label_name = extract_label_name(line);
            content_after_label = get_content_after_label(line);
            first_word_after_label = get_first_word(content_after_label);

            if (label_name)
            {
                if (!is_valid_label_name(label_name))
                {
                    REPORT_ERROR_ONLY(INVALID_LABEL_NAME, line_number, "Invalid label name", label_name, content_after_label);
                    free(first_word);
                    if (first_word_after_label)
                        free(first_word_after_label);
                    continue;
                }

                if (is_reserved_word(label_name))
                {
                    REPORT_ERROR_ONLY(INVALID_LABEL_NAME, line_number, "Label cannot be reserved word", label_name, content_after_label);
                    free(first_word);
                    if (first_word_after_label)
                        free(first_word_after_label);
                    continue;
                }

                if (find_macro(macro_table, label_name))
                {
                    REPORT_ERROR_ONLY(INVALID_LABEL_NAME, line_number, "Label cannot be macro name", label_name, content_after_label);
                    free(first_word);
                    if (first_word_after_label)
                        free(first_word_after_label);
                    continue;
                }

                if (is_label_already_defined(label_table, label_name))
                {
                    REPORT_ERROR_ONLY(DUPLICATE_LABEL_NAME, line_number, "Label already defined", label_name, content_after_label);
                    free(first_word);
                    if (first_word_after_label)
                        free(first_word_after_label);
                    continue;
                }

                if (first_word_after_label &&
                    (strcmp(first_word_after_label, MACRO_START) == 0 ||
                     strcmp(first_word_after_label, MACRO_END) == 0 ||
                     find_macro(macro_table, first_word_after_label)))
                {
                    REPORT_ERROR_ONLY(LABEL_ON_MACRO_LINE, line_number, "Label not allowed on macro line", label_name, content_after_label);
                    free(first_word);
                    if (first_word_after_label)
                        free(first_word_after_label);
                    continue;
                }

                if (!add_label_to_table(label_table, label_name, line_number))
                {
                    REPORT_CRITICAL_ERROR_AND_EXIT(MEMORY_ALLOCATION_ERROR, line_number, "Failed to add label to table", label_name, content_after_label);
                }
            }

            if (label_name)
                free(label_name);
            if (content_after_label)
                free(content_after_label);
            if (first_word_after_label)
                free(first_word_after_label);
        }

        /* STATE 3: Inside macro definition - collect lines */
        if (inside_macro)
        {
            /* Store line in temporary macro content */
            /* Check if we need to expand */
            if (macro_line_count >= macro_capacity)
            {
                char **new_lines;
                macro_capacity *= 2;
                new_lines = realloc(macro_lines, macro_capacity * sizeof(char *));
                if (!new_lines)
                {
                    print_error(MEMORY_ALLOCATION_ERROR, line_number, "Failed to expand macro storage");
                    cleanup_macro_lines(macro_lines, macro_line_count);
                    macro_lines = NULL;
                    has_errors = TRUE;
                    free(first_word);
                    continue;
                }
                macro_lines = new_lines;
            }

            /* Allocate and copy line */
            macro_lines[macro_line_count] = malloc(strlen(line) + 1);
            if (!macro_lines[macro_line_count])
            {
                print_error(MEMORY_ALLOCATION_ERROR, line_number, "Failed to allocate macro line");
                cleanup_macro_lines(macro_lines, macro_line_count);
                macro_lines = NULL;
                has_errors = TRUE;
                free(first_word);
                continue;
            }
            strcpy(macro_lines[macro_line_count], line);
            macro_line_count++;
            free(first_word);
            continue;
        }

        /* STATE 4: Regular processing - check for macro calls or copy line */
        macro = find_macro(macro_table, first_word); /* Fixed: removed * */
        if (macro)
        {
            if (has_extraneous_text_after_words(line, 1))
            {
                print_error(EXTRANEOUS_TEXT, line_number, "Extra text after macro call");
                has_errors = TRUE;
            }
            else
            {
                macro_data = (MacroData *)macro->data;
                if (macro_data)
                {
                    for (j = 0; j < macro_data->line_count; j++)
                    {
                        fputs(macro_data->content[j], output);
                    }
                }
            }
        }
        else
        {
            /* Regular line - copy as-is to output */
            fputs(line, output);
        }

        if (first_word)
            free(first_word);
    }

    /* Cleanup resources */
    if (macro_table)
        free_macro_table(macro_table);
    if (label_table)
        free_label_table(label_table);
    cleanup_macro_lines(macro_lines, macro_line_count);
    macro_lines = NULL;
    macro_capacity = 0;
    if (input)
        fclose(input);
    if (output)
        fclose(output);
    if (input_name)
        free(input_name);

    if (has_errors && output_name && file_exists(output_name))
    {
        remove(output_name);
    }

    if (output_name)
        free(output_name);
    return !has_errors;
} /* End of preassembler function */
