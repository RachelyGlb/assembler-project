/*
 * errors.h - the one place that prints diagnostics.
 *
 * Every error and every warning of the assembler is printed by this module,
 * on the standard output, in the formats of SPEC ERR-R13 and ERR-R14:
 *
 *     <name>.as:<line>: error: <text>
 *     <name>.as: error: <text>            (a diagnostic without a line)
 *
 * and the same two with "warning:". An ErrorContext belongs to one source
 * file: it knows the name of the file and counts the errors reported for it.
 * Warnings are printed but not counted.
 */
#ifndef ERRORS_H
#define ERRORS_H

#include "common.h"

/* The diagnostics state of one source file. The structure is private. */
typedef struct ErrorContext ErrorContext;

/*
 * Creates the context of one source file.
 *   base_name  the name of the file as given on the command line, without
 *              the extension (CLI-8). The string is not copied: it must stay
 *              valid until the context is destroyed.
 * Returns the new context, or NULL when memory is exhausted.
 */
ErrorContext *errors_create(const char *base_name);

/*
 * Prints one error line and counts it.
 *   context  the context of the file the error belongs to
 *   line     the line number in the .as file (ERR-R12), or NO_LINE for an
 *            error that belongs to no line
 *   format   a printf format for the text, followed by its arguments; the
 *            text must not contain a newline
 */
void errors_report(ErrorContext *context, int line, const char *format, ...);

/*
 * Prints one warning line. A warning is not counted as an error (ERR-R14).
 *   context  the context of the file the warning belongs to
 *   line     the line number in the .as file, or NO_LINE
 *   format   a printf format for the text, followed by its arguments
 */
void errors_warn(ErrorContext *context, int line, const char *format, ...);

/*
 * Prints the error line for an allocation failure (ERR-R10). It needs no
 * context, because the context itself may be what could not be allocated.
 *   base_name  the name of the file that was being processed
 */
void errors_report_no_memory(const char *base_name);

/*
 * Returns the number of errors reported through the context so far.
 *   context  the context to ask
 */
int errors_count(const ErrorContext *context);

/*
 * Frees a context.
 *   context  the context to free; NULL is allowed and does nothing
 */
void errors_destroy(ErrorContext *context);

#endif /* ERRORS_H */
