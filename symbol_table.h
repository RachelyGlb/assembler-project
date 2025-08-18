/* symbol_table.h - symbol table management */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "types.h" /* includes the definitions of Symbol and SymbolType */

/* single global variable - head of the symbol list */
extern Symbol *symbol_table_head;

/* symbol table functions */
void add_symbol(const char *name, int address, SymbolType type);
Symbol *find_symbol(const char *name);
void print_symbol_table(void);
void free_symbol_table(void);
int add_symbol_to_table(const char *label, int address, SymbolType type, int line_number, int is_entry);

/* helper functions */
Boolean is_symbol_defined(const char *name);
Boolean mark_symbol_as_entry(const char *name);
int get_symbol_count(void);

#endif /* SYMBOL_TABLE_H */
