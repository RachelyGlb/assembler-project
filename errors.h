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
 *
 * The style of every <text>. SPEC leaves the wording free (ERR-R13); this
 * module fixes it, and every caller follows these rules:
 *
 *   1. One line of plain ASCII with no final period. It starts in lower
 *      case, or with the quoted text that is at fault.
 *   2. A statement of fact in the present tense, with the offending thing
 *      first. The same kind of fault has the same shape everywhere:
 *        absent      "<thing> is missing <where>"
 *        surplus     "unexpected <thing> <where>"
 *        forbidden   "<thing> is not allowed <where>"
 *        unknown     "unknown <thing> '<text>'"
 *        malformed   "<thing> '<text>' is not <what it must be>"
 *        too large   "<thing> '<text>' is longer than <limit>" or
 *                    "... is out of range; ... holds <minimum> to <maximum>"
 *        conflict    "the <thing> '<name>' is already <defined or declared>"
 *                    or "... is also <the other thing it is>"
 *        file        "cannot <open, read, create or write> the '<ext>' file"
 *   3. Where the fault does not say what is wanted, the expectation follows
 *      after a semicolon: "...; the operation takes 2".
 *   4. The offending token is named whenever there is one, and the limit is
 *      stated whenever a limit is exceeded.
 *   5. Single quotes surround everything that is written in a file exactly
 *      as shown: text copied from the line, names, keywords such as 'mcro'
 *      and '.extern', and file extensions. Punctuation is named in words
 *      (comma, colon, quotation mark, square bracket). A file is named by
 *      its extension, because the start of the line already names the file.
 *   6. Numbers and limits are never typed into a text: they come from the
 *      named constants of the program, as arguments of the format. A
 *      keyword or a character that has a named constant comes from it too.
 *   7. The vocabulary is that of any assembler: operation, directive,
 *      operand, label (a name defined with a colon), symbol (a name that is
 *      declared or used), macro, white space.
 *   8. The format is a string literal at the call, so that the compiler
 *      checks it against the arguments (see ERRORS_PRINTF_FORMAT).
 */
#ifndef ERRORS_H
#define ERRORS_H

#include <stdarg.h>

#include "common.h"

/*
 * Marks a function that takes a printf format, so that gcc checks every
 * call against the format. Other compilers see nothing.
 *   format_index    the position of the format among the parameters, from 1
 *   first_argument  the position of the first argument of the format, or 0
 *                   when the arguments arrive as a va_list
 */
#ifdef __GNUC__
#define ERRORS_PRINTF_FORMAT(format_index, first_argument) \
    __attribute__((format(printf, format_index, first_argument)))
#else
#define ERRORS_PRINTF_FORMAT(format_index, first_argument)
#endif

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
void errors_report(ErrorContext *context, int line, const char *format, ...)
    ERRORS_PRINTF_FORMAT(3, 4);

/*
 * Prints one error line and counts it, for a caller that is itself a
 * function with a variable argument list. The caller must be marked with
 * ERRORS_PRINTF_FORMAT, so that its own calls are checked.
 *   context    the context of the file the error belongs to
 *   line       the line number in the .as file, or NO_LINE
 *   format     a printf format for the text, without a newline
 *   arguments  the arguments of the format
 */
void errors_report_list(ErrorContext *context, int line, const char *format,
                        va_list arguments) ERRORS_PRINTF_FORMAT(3, 0);

/*
 * Prints one warning line. A warning is not counted as an error (ERR-R14).
 *   context  the context of the file the warning belongs to
 *   line     the line number in the .as file, or NO_LINE
 *   format   a printf format for the text, followed by its arguments
 */
void errors_warn(ErrorContext *context, int line, const char *format, ...)
    ERRORS_PRINTF_FORMAT(3, 4);

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
