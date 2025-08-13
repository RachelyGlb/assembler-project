#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#include "symbol_table.h"


typedef enum { CODE, DATA, EXTERN_SYM } SymbolType;

typedef struct Symbol {
    char name[31];         // שם התווית (מוגבל ל-30 תווים)
    int address;           // כתובת (תעודכן בהתאם)
    SymbolType type;       // סוג הסימן (קוד או נתון)
     bool is_entry; 
    struct Symbol *next;   // הצבעה לסימן הבא (רשימה מקושרת)
} Symbol;

extern Symbol *symbol_table_head;

void add_symbol(const char *name, int address, SymbolType type);
Symbol *find_symbol(const char *name);
void print_symbol_table();
void free_symbol_table();
bool add_label_to_table(const char *label, int address, SymbolType type, int line_number, bool is_entry);


#endif
