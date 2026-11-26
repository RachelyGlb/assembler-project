/*
 * errors.c - the one place that prints diagnostics.
 *
 * See errors.h for the formats. Every diagnostic goes through the static
 * function print_diagnostic, so the layout of a diagnostic line is written
 * down exactly once.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"

/* The severity words of SPEC ERR-R13 and ERR-R14. */
#define SEVERITY_ERROR "error"
#define SEVERITY_WARNING "warning"

/* The text of the one diagnostic that this module words itself (ERR-R10). */
#define NO_MEMORY_TEXT "out of memory; the assembler stops"

/* The diagnostics state of one source file. */
struct ErrorContext
{
    const char *base_name; /* The file name without extension; not owned. */
    int error_count;       /* The number of errors reported so far. */
};

/*
 * Prints the part of a diagnostic line that precedes the text.
 *   base_name  the file name without the extension
 *   line       the line number, or NO_LINE
 *   severity   SEVERITY_ERROR or SEVERITY_WARNING
 */
static void print_prefix(const char *base_name, int line, const char *severity)
{
    if (line == NO_LINE)
    {
        printf("%s%s: %s: ", base_name, SOURCE_EXTENSION, severity);
    }
    else
    {
        printf("%s%s:%d: %s: ", base_name, SOURCE_EXTENSION, line, severity);
    }
}

/*
 * Prints one complete diagnostic line.
 *   base_name  the file name without the extension
 *   line       the line number, or NO_LINE
 *   severity   SEVERITY_ERROR or SEVERITY_WARNING
 *   format     a printf format for the text
 *   arguments  the arguments of the format
 */
static void print_diagnostic(const char *base_name, int line,
                             const char *severity, const char *format,
                             va_list arguments)
{
    print_prefix(base_name, line, severity);
    vprintf(format, arguments);
    putchar('\n');
}

/* Creates the context of one source file; see errors.h. */
ErrorContext *errors_create(const char *base_name)
{
    ErrorContext *context = (ErrorContext *)malloc(sizeof(ErrorContext));

    if (context != NULL)
    {
        context->base_name = base_name;
        context->error_count = 0;
    }
    return context;
}

/* Prints one error line and counts it; see errors.h. */
void errors_report(ErrorContext *context, int line, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    print_diagnostic(context->base_name, line, SEVERITY_ERROR, format,
                     arguments);
    va_end(arguments);
    context->error_count++;
}

/* Prints one warning line, which is not counted; see errors.h. */
void errors_warn(ErrorContext *context, int line, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    print_diagnostic(context->base_name, line, SEVERITY_WARNING, format,
                     arguments);
    va_end(arguments);
}

/* Prints the error line for an allocation failure; see errors.h. */
void errors_report_no_memory(const char *base_name)
{
    print_prefix(base_name, NO_LINE, SEVERITY_ERROR);
    puts(NO_MEMORY_TEXT);
}

/* Returns the number of errors reported so far; see errors.h. */
int errors_count(const ErrorContext *context)
{
    return context->error_count;
}

/* Frees a context; see errors.h. */
void errors_destroy(ErrorContext *context)
{
    free(context);
}
