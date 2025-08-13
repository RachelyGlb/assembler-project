#include "preassembler.h"


/* Print error message */
void print_error(ErrorType error, int line_number, const char *message) {
    /* Print base error message with line number if available */
    if (line_number > 0)
    {
        fprintf(stderr, "Error on line %d: ", line_number);
    }
    else
    {
        fprintf(stderr, "Error: ");
    }

    /* Print specific error type */
    switch (error)
    {
    case LINE_TOO_LONG:
        fprintf(stderr, "Line is too long");
        break;
    case INVALID_MACRO_NAME:
        fprintf(stderr, "Invalid macro name");
        break;
    case LABEL_ON_MACRO_LINE:
        fprintf(stderr, "Label not allowed on macro definition line");
        break;
    case EXTRANEOUS_TEXT:
        fprintf(stderr, "Extraneous text");
        break;
    case MEMORY_ALLOCATION_ERROR:
        fprintf(stderr, "Memory allocation failed");
        break;
    case FILE_ERROR:
        fprintf(stderr, "File error");
        break;
    case MACRO_NOT_CLOSED:
        fprintf(stderr, "Macro definition not properly closed");
        break;
    case DUPLICATE_MACRO_NAME:
        fprintf(stderr, "Macro name already defined");
        break;
    case INVALID_LABEL_NAME:
        fprintf(stderr, "Invalid label name");
        break;
    case RESERVED_WORD:
        fprintf(stderr, "Reserved word used");
        break;
    case DUPLICATE_LABEL_NAME:
        fprintf(stderr, "Label name already defined");
        break;
    case SYNTAX_ERROR:
        fprintf(stderr, "Syntax error");
        break;
    default:
        fprintf(stderr, "Unknown error");
        break;
    }

    /* Add additional message if provided */
    if (message && *message)
    {
        fprintf(stderr, " - %s", message);
    }

    fprintf(stderr, "\n");
}
