#include "symbol_table.h"

extern Symbol *symbol_table_head;

void add_symbol(const char *name, int address, SymbolType type) {
    Symbol *new_symbol = (Symbol *)malloc(sizeof(Symbol));
    if (!new_symbol) {
        fprintf(stderr, "Memory allocation failed for symbol.\n");
        exit(1);
    }

    strncpy(new_symbol->name, name, 30);
    new_symbol->name[30] = '\0';
    new_symbol->address = address;
    new_symbol->type = type;
    new_symbol->is_entry = 0;
    new_symbol->next = symbol_table_head;
    symbol_table_head = new_symbol;
}

int add_symbol_to_table(const char *label, int address, SymbolType type,
                        int line_number, int is_entry) {
    int addr_to_set;
    Symbol *exists;  
    if (!label || strlen(label) == 0) {
        fprintf(stderr, "Error: Empty label at line %d\n", line_number);
        return 0;
    }

    exists = find_symbol(label);
    if (exists) {
        if (is_entry) {
            if (exists->type == EXTERN_SYM) {
                fprintf(stderr, "Error: .entry on extern label '%s' at line %d\n", label, line_number);
                return 0;
            }
            exists->is_entry = 1;
            return 1;
        }

       
        if ((type == CODE || type == DATA) &&
            exists->type != EXTERN_SYM &&
            exists->address == 0) {
            exists->address = address;
            exists->type = type;
            return 1;
        }

        if (type == EXTERN_SYM) {
            if (exists->type != EXTERN_SYM) {
                fprintf(stderr, "Error: Label '%s' already defined (not extern) at line %d\n",
                        label, line_number);
                return 0;
            }
            return 1;
        }

        fprintf(stderr, "Error: Duplicate label '%s' at line %d\n", label, line_number);
        return 0;
    }

    addr_to_set = (type == EXTERN_SYM) ? 0 : address;
    add_symbol(label, addr_to_set, type);

    if (is_entry) {
        if (type == EXTERN_SYM) {
            fprintf(stderr, "Error: .entry cannot be applied to extern label '%s' (line %d)\n",
                    label, line_number);
            return 0;
        }
        symbol_table_head->is_entry = 1;
    }

    return 1;
}

Symbol *find_symbol(const char *name) {
    Symbol *current = symbol_table_head;
    while (current) {
        if (strcmp(current->name, name) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

void print_symbol_table() {
    Symbol *current = symbol_table_head;
    printf("Symbol Table:\n");
    while (current) {
        const char *t =
            (current->type == CODE) ? "CODE" :
            (current->type == DATA) ? "DATA" : "EXTERN";
        printf("Name: %s, Address: %d, Type: %s, Entry: %s\n",
               current->name, current->address, t,
               current->is_entry ? "YES" : "NO");
        current = current->next;
    }
}

void free_symbol_table() {
    Symbol *current = symbol_table_head;
    while (current) {
        Symbol *next = current->next;
        free(current);
        current = next;
    }
    symbol_table_head = NULL;
}
