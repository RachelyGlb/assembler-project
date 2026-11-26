/*
 * macro_table.c - the macros of one source file.
 *
 * The table is a singly linked list of macros in the order of definition.
 * The body of a macro is an array of lines that doubles when it is full, so
 * a line is reached by its index in constant time. See macro_table.h for
 * the contracts.
 */
#include <stdlib.h>
#include <string.h>

#include "macro_table.h"
#include "util.h"

/* The first size of the array of body lines, and its growth factor. */
#define BODY_INITIAL_CAPACITY 8
#define BODY_GROWTH_FACTOR 2

/* One line of a macro body. */
typedef struct
{
    char *text;      /* The line as written, without a terminator; owned. */
    int source_line; /* Its line number in the .as file. */
} BodyLine;

/* One macro. */
struct Macro
{
    char *name;         /* The name; owned. */
    BodyLine *lines;    /* The body lines; owned; NULL while empty. */
    int line_count;     /* The number of body lines in use. */
    int line_capacity;  /* The number of body lines allocated. */
    struct Macro *next; /* The macro defined after this one. */
};

/* The macros of one source file. */
struct MacroTable
{
    Macro *first; /* The macro defined first, or NULL. */
    Macro *last;  /* The macro defined last, or NULL. */
};

/* Creates an empty macro table; see macro_table.h. */
MacroTable *macro_table_create(void)
{
    MacroTable *table = (MacroTable *)malloc(sizeof(MacroTable));

    if (table != NULL)
    {
        table->first = NULL;
        table->last = NULL;
    }
    return table;
}

/*
 * Frees one macro with its name and its body lines.
 *   macro  the macro to free
 */
static void macro_destroy(Macro *macro)
{
    int index;

    for (index = 0; index < macro->line_count; index++)
    {
        free(macro->lines[index].text);
    }
    free(macro->lines);
    free(macro->name);
    free(macro);
}

/* Frees a macro table with everything in it; see macro_table.h. */
void macro_table_destroy(MacroTable *table)
{
    Macro *macro;
    Macro *next;

    if (table == NULL)
    {
        return;
    }
    for (macro = table->first; macro != NULL; macro = next)
    {
        next = macro->next;
        macro_destroy(macro);
    }
    free(table);
}

/* Adds a macro with an empty body; see macro_table.h. */
Macro *macro_table_add(MacroTable *table, const char *name)
{
    Macro *macro;

    if (macro_table_find(table, name) != NULL)
    {
        return NULL;
    }
    macro = (Macro *)malloc(sizeof(Macro));
    if (macro == NULL)
    {
        return NULL;
    }
    macro->name = util_copy_string(name);
    if (macro->name == NULL)
    {
        free(macro);
        return NULL;
    }
    macro->lines = NULL;
    macro->line_count = 0;
    macro->line_capacity = 0;
    macro->next = NULL;
    if (table->last == NULL)
    {
        table->first = macro;
    }
    else
    {
        table->last->next = macro;
    }
    table->last = macro;
    return macro;
}

/*
 * Makes sure that the body of a macro has room for one more line.
 *   macro  the macro whose body may have to grow
 * Returns FALSE when the body had to grow and memory is exhausted.
 */
static Boolean reserve_body_line(Macro *macro)
{
    int capacity;
    BodyLine *lines;

    if (macro->line_count < macro->line_capacity)
    {
        return TRUE;
    }
    capacity = (macro->line_capacity == 0)
                   ? BODY_INITIAL_CAPACITY
                   : macro->line_capacity * BODY_GROWTH_FACTOR;
    lines = (BodyLine *)realloc(macro->lines,
                                (size_t)capacity * sizeof(BodyLine));
    if (lines == NULL)
    {
        return FALSE;
    }
    macro->lines = lines;
    macro->line_capacity = capacity;
    return TRUE;
}

/* Appends one line to the body of a macro; see macro_table.h. */
Boolean macro_add_line(Macro *macro, const char *line, int source_line)
{
    char *text;

    if (!reserve_body_line(macro))
    {
        return FALSE;
    }
    text = util_copy_string(line);
    if (text == NULL)
    {
        return FALSE;
    }
    macro->lines[macro->line_count].text = text;
    macro->lines[macro->line_count].source_line = source_line;
    macro->line_count++;
    return TRUE;
}

/* Looks up a macro by its name; see macro_table.h. */
const Macro *macro_table_find(const MacroTable *table, const char *name)
{
    return macro_table_find_text(table, name, strlen(name));
}

/* Looks up a macro by a piece of a longer text; see macro_table.h. */
const Macro *macro_table_find_text(const MacroTable *table, const char *text,
                                   size_t length)
{
    const Macro *macro;

    for (macro = table->first; macro != NULL; macro = macro->next)
    {
        if (strlen(macro->name) == length &&
            strncmp(macro->name, text, length) == 0)
        {
            return macro;
        }
    }
    return NULL;
}

/* Returns the name of a macro; see macro_table.h. */
const char *macro_name(const Macro *macro)
{
    return macro->name;
}

/* Returns the number of body lines of a macro; see macro_table.h. */
int macro_line_count(const Macro *macro)
{
    return macro->line_count;
}

/* Returns one body line of a macro; see macro_table.h. */
const char *macro_line(const Macro *macro, int index)
{
    return macro->lines[index].text;
}

/* Returns the .as line number of one body line; see macro_table.h. */
int macro_line_source(const Macro *macro, int index)
{
    return macro->lines[index].source_line;
}
