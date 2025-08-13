#include "preassembler.h"

/* Check if a line is empty (contains only whitespace) */
Boolean is_empty_line(const char *line) {
    int i;
    
    if (!line) {
        return TRUE;
    }
    
    for (i = 0; line[i] != '\0' && line[i] != '\n'; i++) {
        if (!isspace(line[i])&& (line[i] != '\n')&& line[i] != '\t') {
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


/* Check if a line is too long */
Boolean is_line_too_long(const char *line, int line_number, FILE *input) {
    
    if (!line) {
        return FALSE;
    }
    
    /* Count characters until \n or \0 */
    if (line[0] != '\0' &&line[strlen(line) - 1] != '\n' && !feof(input))
    {
        print_error(LINE_TOO_LONG, line_number, NULL);
        return TRUE;
    }
    return FALSE;
    
    
}
