/*
 * symbol_table.c - the symbols of one source file.
 *
 * The table is a singly linked list with a pointer to its last node, so
 * that a symbol is appended in constant time and the list keeps the order
 * of the definitions. See symbol_table.h for the contracts.
 */
#include "symbol_table.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

/* One symbol, a node of the list. */
struct Symbol
{
    char *name;      /* The name; owned. */
    int address;     /* The value. */
    SymbolKind kind; /* Code, data or external. */
    Boolean entry;   /* TRUE once a .entry directive named the symbol. */
    Symbol *next;    /* The symbol added after this one, or NULL. */
};

/* The symbols of one source file. */
struct SymbolTable
{
    Symbol *first; /* The first symbol added, or NULL. */
    Symbol *last;  /* The last symbol added, or NULL. */
};

/* Creates an empty symbol table; see symbol_table.h. */
SymbolTable *symbol_table_create(void)
{
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));

    if (table != NULL)
    {
        table->first = NULL;
        table->last = NULL;
    }
    return table;
}

/* Frees a symbol table with all its symbols; see symbol_table.h. */
void symbol_table_destroy(SymbolTable *table)
{
    Symbol *symbol;
    Symbol *next;

    if (table == NULL)
    {
        return;
    }
    for (symbol = table->first; symbol != NULL; symbol = next)
    {
        next = symbol->next;
        free(symbol->name);
        free(symbol);
    }
    free(table);
}

/* Adds a symbol at the end of the list; see symbol_table.h. */
Boolean symbol_table_add(SymbolTable *table, const char *name, int address,
                         SymbolKind kind)
{
    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));

    if (symbol == NULL)
    {
        return FALSE;
    }
    symbol->name = util_copy_string(name);
    if (symbol->name == NULL)
    {
        free(symbol);
        return FALSE;
    }
    symbol->address = address;
    symbol->kind = kind;
    symbol->entry = FALSE;
    symbol->next = NULL;
    if (table->last == NULL)
    {
        table->first = symbol;
    }
    else
    {
        table->last->next = symbol;
    }
    table->last = symbol;
    return TRUE;
}

/*
 * Looks up a symbol that the caller may change.
 *   table  the table to search
 *   name   the name to look for
 * Returns the symbol, or NULL when no symbol has that name.
 */
static Symbol *find_symbol(const SymbolTable *table, const char *name)
{
    Symbol *symbol;

    for (symbol = table->first; symbol != NULL; symbol = symbol->next)
    {
        if (strcmp(symbol->name, name) == 0)
        {
            return symbol;
        }
    }
    return NULL;
}

/* Looks up a symbol by its name; see symbol_table.h. */
const Symbol *symbol_table_find(const SymbolTable *table, const char *name)
{
    return find_symbol(table, name);
}

/* Gives a symbol the attribute "entry"; see symbol_table.h. */
Boolean symbol_table_mark_entry(SymbolTable *table, const char *name)
{
    Symbol *symbol = find_symbol(table, name);

    if (symbol == NULL)
    {
        return FALSE;
    }
    symbol->entry = TRUE;
    return TRUE;
}

/* Adds an offset to every data symbol (ALG-6); see symbol_table.h. */
void symbol_table_relocate_data(SymbolTable *table, int offset)
{
    Symbol *symbol;

    for (symbol = table->first; symbol != NULL; symbol = symbol->next)
    {
        if (symbol->kind == SYMBOL_DATA)
        {
            symbol->address += offset;
        }
    }
}

/* Calls a function for every symbol; see symbol_table.h. */
void symbol_table_for_each(const SymbolTable *table, SymbolVisitor visit,
                           void *argument)
{
    const Symbol *symbol;

    for (symbol = table->first; symbol != NULL; symbol = symbol->next)
    {
        visit(symbol, argument);
    }
}

/* Returns the name of a symbol; see symbol_table.h. */
const char *symbol_name(const Symbol *symbol)
{
    return symbol->name;
}

/* Returns the value of a symbol; see symbol_table.h. */
int symbol_address(const Symbol *symbol)
{
    return symbol->address;
}

/* Returns the kind of a symbol; see symbol_table.h. */
SymbolKind symbol_kind(const Symbol *symbol)
{
    return symbol->kind;
}

/* Tells whether a symbol is an entry; see symbol_table.h. */
Boolean symbol_is_entry(const Symbol *symbol)
{
    return symbol->entry;
}
