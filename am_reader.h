/*
 * am_reader.h - the walk over the statements of the expanded source file.
 *
 * Both passes read the .am file in the same way: line by line, every line
 * parsed into a statement by the syntax module and labelled with the number
 * of the .as line it came from (ERR-R12). This module holds that walk once;
 * a pass supplies only the function that handles one statement. The walk
 * lives in a module of its own because it needs the syntax module, which
 * the language-neutral util module must not know, and because neither pass
 * may depend on the other.
 */
#ifndef AM_READER_H
#define AM_READER_H

#include <stdio.h>

#include "common.h"
#include "errors.h"
#include "line_map.h"
#include "statement.h"

/*
 * The type of a function that am_reader_for_each calls for a statement.
 *   statement    the parsed line; it may be not valid, and it is freed by
 *                the reader after the call
 *   line_number  the number of the line in the .as file (ERR-R12)
 *   argument     the pointer that was given to am_reader_for_each
 * Returns FALSE when memory is exhausted, which ends the walk.
 */
typedef Boolean (*StatementHandler)(const Statement *statement,
                                    int line_number, void *argument);

/*
 * Reads an expanded source file to its end and calls a function for every
 * line, in the order of the file (ERR-R17). The syntax errors of a line are
 * reported while it is parsed, before the function is called. A file that
 * cannot be read is reported (CLI-12, ERR-47).
 *   am_file   the .am file, open for reading at its beginning; the caller
 *             closes it
 *   lines     the line map of the file, for the .as line numbers (MAC-35)
 *   errors    the diagnostics context of the file
 *   handle    the function to call for every statement
 *   argument  a pointer that is passed on to handle unchanged
 * Returns STAGE_OK when no error was reported during the walk, by this
 * module, by the syntax module or by handle; STAGE_ERRORS when at least one
 * was; STAGE_FATAL when memory ran out (the caller reports that, ERR-R10).
 */
StageResult am_reader_for_each(FILE *am_file, const LineMap *lines,
                               ErrorContext *errors, StatementHandler handle,
                               void *argument);

#endif /* AM_READER_H */
