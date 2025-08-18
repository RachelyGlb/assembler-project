#include "preassembler.h"
#include "line_analysis.h"


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
