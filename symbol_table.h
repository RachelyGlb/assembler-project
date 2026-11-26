/*
 * symbol_table.h - the symbols of one source file.
 *
 * The table stores every symbol by name, with its value, its kind (code,
 * data or external) and the "entry" attribute (LBL-12). The first pass
 * fills the table; the second pass looks symbols up and marks the entries;
 * the output writer walks it. A table belongs to one source file: the
 * driver creates it before the first pass and destroys it after the output
 * is written. The number of symbols is limited only by memory (LBL-19).
 */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"

/* The symbols of one source file. The structure is private. */
typedef struct SymbolTable SymbolTable;

/* One symbol. The structure is private. */
typedef struct Symbol Symbol;

/* Where the value of a symbol comes from (LBL-12). */
typedef enum
{
    SYMBOL_CODE,    /* A label of an instruction: its value is an IC. */
    SYMBOL_DATA,    /* A label of .data, .string or .mat: a DC (DIR-8). */
    SYMBOL_EXTERNAL /* Declared by .extern: the value is 0 (DIR-21). */
} SymbolKind;

/*
 * The type of a function that symbol_table_for_each calls for a symbol.
 *   symbol    the symbol visited
 *   argument  the pointer that was given to symbol_table_for_each
 */
typedef void (*SymbolVisitor)(const Symbol *symbol, void *argument);

/*
 * Creates an empty symbol table.
 * Returns the table, or NULL when memory is exhausted.
 */
SymbolTable *symbol_table_create(void);

/*
 * Frees a symbol table with all its symbols.
 *   table  the table to free; NULL is allowed and does nothing
 */
void symbol_table_destroy(SymbolTable *table);

/*
 * Adds a symbol. The table does not look for an older symbol of the same
 * name: the caller decides what a repeated name means (LBL-5, DIR-34,
 * DIR-36) and asks symbol_table_find first.
 *   table    the table to add to
 *   name     the name of the symbol; it is copied
 *   address  the value of the symbol
 *   kind     the kind of the symbol
 * Returns FALSE when memory is exhausted; the table is then unchanged.
 */
Boolean symbol_table_add(SymbolTable *table, const char *name, int address,
                         SymbolKind kind);

/*
 * Looks up a symbol by its name. The comparison is case-sensitive (LBL-6).
 *   table  the table to search
 *   name   the name to look for
 * Returns the symbol, or NULL when no symbol has that name.
 */
const Symbol *symbol_table_find(const SymbolTable *table, const char *name);

/*
 * Gives a symbol the attribute "entry" (DIR-19). Marking a symbol twice is
 * the same as marking it once (DIR-34).
 *   table  the table that holds the symbol
 *   name   the name of the symbol
 * Returns FALSE when no symbol has that name.
 */
Boolean symbol_table_mark_entry(SymbolTable *table, const char *name);

/*
 * Adds an offset to the value of every data symbol. The first pass calls it
 * once, with the final IC, because the data image follows the code image
 * (ALG-6).
 *   table   the table to change
 *   offset  the number to add
 */
void symbol_table_relocate_data(SymbolTable *table, int offset);

/*
 * Calls a function for every symbol, in the order in which the symbols were
 * added.
 *   table     the table to walk
 *   visit     the function to call
 *   argument  a pointer that is handed to every call unchanged
 */
void symbol_table_for_each(const SymbolTable *table, SymbolVisitor visit,
                           void *argument);

/*
 * Returns the name of a symbol.
 *   symbol  the symbol to ask
 */
const char *symbol_name(const Symbol *symbol);

/*
 * Returns the value of a symbol: an address, or 0 for an external symbol.
 *   symbol  the symbol to ask
 */
int symbol_address(const Symbol *symbol);

/*
 * Returns the kind of a symbol.
 *   symbol  the symbol to ask
 */
SymbolKind symbol_kind(const Symbol *symbol);

/*
 * Tells whether a symbol has the attribute "entry".
 *   symbol  the symbol to ask
 * Returns TRUE when a .entry directive named the symbol.
 */
Boolean symbol_is_entry(const Symbol *symbol);

#endif /* SYMBOL_TABLE_H */
