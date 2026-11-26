/*
 * macro_table.h - the macros of one source file.
 *
 * The table stores every macro by name, with the lines of its body in the
 * order in which they were written and, for each line, its line number in
 * the .as file (MAC-35). The pre-assembler fills the table; the first pass
 * consults it for the clash between a label and a macro name (LBL-21), so a
 * table lives until the first pass of its file has ended. The number of
 * macros and of body lines is limited only by memory (MAC-27).
 */
#ifndef MACRO_TABLE_H
#define MACRO_TABLE_H

#include <stddef.h>

#include "common.h"

/* The macros of one source file. The structure is private. */
typedef struct MacroTable MacroTable;

/* One macro: a name and its body lines. The structure is private. */
typedef struct Macro Macro;

/*
 * Creates an empty macro table.
 * Returns the table, or NULL when memory is exhausted.
 */
MacroTable *macro_table_create(void);

/*
 * Frees a macro table with all its macros and their lines.
 *   table  the table to free; NULL is allowed and does nothing
 */
void macro_table_destroy(MacroTable *table);

/*
 * Adds a macro with an empty body. The table does not look for the name:
 * the caller asks macro_table_find first and adds only a name that is not
 * in the table, as with the symbol table.
 *   table  the table to add to
 *   name   the name of the macro, which is not in the table; it is copied
 * Returns the new macro, or NULL when memory is exhausted.
 */
Macro *macro_table_add(MacroTable *table, const char *name);

/*
 * Appends one line to the body of a macro.
 *   macro        the macro to extend
 *   line         the text of the line, without a line terminator; copied
 *   source_line  the number of the line in the .as file
 * Returns FALSE when memory is exhausted; the macro is then unchanged.
 */
Boolean macro_add_line(Macro *macro, const char *line, int source_line);

/*
 * Looks up a macro by its name. The comparison is case-sensitive.
 *   table  the table to search
 *   name   the name to look for
 * Returns the macro, or NULL when no macro has that name.
 */
const Macro *macro_table_find(const MacroTable *table, const char *name);

/*
 * Looks up a macro by a name that is given as a piece of a longer text.
 *   table   the table to search
 *   text    the first character of the name; no terminating zero is needed
 *   length  the number of characters of the name
 * Returns the macro, or NULL when no macro has exactly that name.
 */
const Macro *macro_table_find_text(const MacroTable *table, const char *text,
                                   size_t length);

/*
 * Returns the name of a macro.
 *   macro  the macro to ask
 */
const char *macro_name(const Macro *macro);

/*
 * Returns the number of lines in the body of a macro.
 *   macro  the macro to ask
 */
int macro_line_count(const Macro *macro);

/*
 * Returns one line of the body of a macro, exactly as it was added.
 *   macro  the macro to ask
 *   index  the position of the line, from 0 to macro_line_count - 1
 */
const char *macro_line(const Macro *macro, int index);

/*
 * Returns the .as line number of one line of the body of a macro.
 *   macro  the macro to ask
 *   index  the position of the line, from 0 to macro_line_count - 1
 */
int macro_line_source(const Macro *macro, int index);

#endif /* MACRO_TABLE_H */
