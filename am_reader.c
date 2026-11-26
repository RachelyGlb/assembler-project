/*
 * am_reader.c - the walk over the statements of the expanded source file.
 *
 * The line buffer, the count of the .am lines and the statement of the
 * current line belong to the walk and are freed here on every path. See
 * am_reader.h for the contract.
 */
#include "am_reader.h"

#include <stdlib.h>

#include "util.h"

/*
 * Parses one line of the .am file and hands the statement to the handler.
 *   line         the text of the line
 *   line_number  the number of the line in the .as file
 *   errors       the diagnostics context of the file
 *   handle       the function to call for the statement
 *   argument     the pointer to pass on to handle
 * Returns FALSE when memory is exhausted, here or in the handler.
 */
static Boolean handle_line(const char *line, int line_number,
                           ErrorContext *errors, StatementHandler handle,
                           void *argument)
{
    Statement *statement = statement_parse(line, line_number, errors);
    Boolean memory_left;

    if (statement == NULL)
    {
        return FALSE;
    }
    memory_left = handle(statement, line_number, argument);
    statement_destroy(statement);
    return memory_left;
}

/*
 * Reports that the .am file could not be read back (CLI-12, ERR-47). The
 * message has no line number, like the one for a file that does not open.
 *   errors  the diagnostics context of the file
 */
static void report_unreadable(ErrorContext *errors)
{
    errors_report(errors, NO_LINE, "cannot read the %s file",
                  EXPANDED_EXTENSION);
}

/* Calls a function for every line of a .am file; see am_reader.h. */
StageResult am_reader_for_each(FILE *am_file, const LineMap *lines,
                               ErrorContext *errors, StatementHandler handle,
                               void *argument)
{
    char *line = NULL;
    size_t capacity = 0;
    int am_line = FIRST_LINE_NUMBER;
    int errors_before = errors_count(errors);
    Boolean memory_left = TRUE;
    ReadStatus status;

    status = util_read_line(am_file, &line, &capacity, NULL, FALSE);
    while (status == READ_LINE && memory_left)
    {
        memory_left =
            handle_line(line, line_map_source_line(lines, am_line++), errors,
                        handle, argument);
        status = util_read_line(am_file, &line, &capacity, NULL, FALSE);
    }
    free(line);
    if (status == READ_NO_MEMORY || !memory_left)
    {
        return STAGE_FATAL;
    }
    if (status == READ_ERROR)
    {
        report_unreadable(errors);
    }
    return (errors_count(errors) > errors_before) ? STAGE_ERRORS : STAGE_OK;
}
