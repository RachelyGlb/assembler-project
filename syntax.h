/*
 * syntax.h - the parser of one line of the expanded source file.
 *
 * syntax_parse turns one line of the .am file into a Statement (see
 * statement.h): the optional label, the kind of the statement, the
 * operation and its operands with their addressing modes and values, or the
 * directive with its data words or its symbol. It reports every syntax
 * error that can be decided from the line alone (SPEC sections 5.3, ERR-11
 * to ERR-13, ERR-16 to ERR-40, ERR-42 to ERR-45), one message per line
 * (ERR-R16), through the errors module. Errors that need the symbol table,
 * the macro table or the memory counters are left to the passes.
 *
 * This module is the only one that looks at the text of a line. It builds
 * the statement through the functions of statement.h and does not know its
 * structure; the statement module does not know the parser.
 */
#ifndef SYNTAX_H
#define SYNTAX_H

#include "errors.h"
#include "statement.h"

/*
 * Parses one line. When the line has a syntax error, the first error found
 * is reported and the statement is returned all the same, marked as not
 * valid: it still carries its label, if the label itself is legal, so that
 * the first pass can define it (LBL-23).
 *   line         the text of the line, without its line terminator
 *   line_number  the number of the line in the .as file, for the message
 *                (ERR-R12)
 *   errors       the diagnostics context of the file
 * Returns the statement, which the caller frees with statement_destroy, or
 * NULL when memory is exhausted (nothing is reported about that here).
 */
Statement *syntax_parse(const char *line, int line_number,
                        ErrorContext *errors);

#endif /* SYNTAX_H */
