#include "assembler.h"

/* Check if a line is empty (contains only whitespace) */
Boolean is_empty_line(const char *line) {
    int i;
    
    if (!line) {
        return TRUE;
    }
    
    for (i = 0; line[i] != '\0' && line[i] != '\n'; i++) {
        if (!isspace(line[i])) {
            return FALSE;
        }
    }
    
    return TRUE;
}

/* Check if a line is a comment (starts with ';') */
Boolean is_comment_line(const char *line) {
    int i;
    
    if (!line) {
        return FALSE;
    }
    
    /* Skip leading whitespace */
    for (i = 0; line[i] != '\0' && line[i] != '\n'; i++) {
        if (!isspace(line[i])) {
            return line[i] == ';';
        }
    }
    
    return FALSE;
}

/* Check if a line is too long */
Boolean is_line_too_long(const char *line) {
    int length = 0;
    
    if (!line) {
        return FALSE;
    }
    
    /* Count characters until \n or \0 */
    while (line[length] != '\0' && line[length] != '\n') {
        length++;
    }
    
    return length > MAX_LINE_LENGTH;
}

/* Trim leading and trailing whitespace from a string */
char* trim_whitespace(char *str) {
    char *end;
    
    if (!str) {
        return NULL;
    }
    
    /* Trim leading space */
    while (isspace(*str)) {
        str++;
    }
    
    if (*str == 0) {
        return str;
    }
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && (isspace(*end) || *end == '\n')) {
        end--;
    }
    
    /* Write new null terminator */
    *(end + 1) = '\0';
    
    return str;
}

/* Print error message */
void print_error(ErrorType error, int line_number, const char *filename) {
    printf("Error");
    
    if (filename) {
        printf(" in file %s", filename);
    }
    
    if (line_number > 0) {
        printf(" at line %d", line_number);
    }
    
    printf(": ");
    
    switch (error) {
        case LINE_TOO_LONG:
            printf("Line exceeds maximum length of %d characters", MAX_LINE_LENGTH);
            break;
        case INVALID_MACRO_NAME:
            printf("Invalid macro name");
            break;
        case LABEL_ON_MACRO_LINE:
            printf("Label not allowed on macro definition line");
            break;
        case EXTRANEOUS_TEXT:
            printf("Extraneous text found");
            break;
        case MEMORY_ALLOCATION_ERROR:
            printf("Memory allocation failed");
            break;
        case FILE_ERROR:
            printf("File operation failed");
            break;
        case MACRO_NOT_CLOSED:
            printf("Macro definition not properly closed");
            break;
        case DUPLICATE_MACRO_NAME:
            printf("Macro name already defined");
            break;
        default:
            printf("Unknown error");
            break;
    }
    
    printf("\n");
}

/* Check if a file exists */
Boolean file_exists(const char *filename) {
    FILE *file;
    
    if (!filename) {
        return FALSE;
    }
    
    file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return TRUE;
    }
    
    return FALSE;
}

/* Create a filename with given extension */
char* create_filename_with_extension(const char *base_name, const char *extension) {
    char *filename;
    int total_length;
    
    if (!base_name || !extension) {
        return NULL;
    }
    
    total_length = strlen(base_name) + strlen(extension) + 1;
    filename = malloc(total_length);
    
    if (!filename) {
        return NULL;
    }
    
    strcpy(filename, base_name);
    strcat(filename, extension);
    
    return filename;
}

/* Cleanup function for error situations */
void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output) {
    if (table) {
        free_macro_table(table);
    }
    
    if (input) {
        fclose(input);
    }
    
    if (output) {
        fclose(output);
    }
}
