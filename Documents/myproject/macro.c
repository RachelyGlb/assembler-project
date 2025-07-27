#include "assembler.h"

/* Create a new macro table */
MacroTable* create_macro_table(void) {
    MacroTable *table = malloc(sizeof(MacroTable));
    if (!table) {
        return NULL;
    }
    
    table->head = NULL;
    table->count = 0;
    
    return table;
}

/* Free all memory used by the macro table */
void free_macro_table(MacroTable *table) {
    MacroNode *current;
    MacroNode *next;
    int i;
    
    if (!table) {
        return;
    }
    
    current = table->head;
    while (current) {
        next = current->next;
        
        /* Free macro name */
        if (current->name) {
            free(current->name);
        }
        
        /* Free macro content */
        if (current->content) {
            for (i = 0; i < current->line_count; i++) {
                if (current->content[i]) {
                    free(current->content[i]);
                }
            }
            free(current->content);
        }
        
        free(current);
        current = next;
    }
    
    free(table);
}

/* Add a new macro to the table */
Boolean add_macro(MacroTable *table, const char *name, char **content, int line_count) {
    MacroNode *new_node;
    int i;
    
    if (!table || !name || !content) {
        return FALSE;
    }
    
    /* Check for duplicate macro names */
    if (find_macro(table, name)) {
        return FALSE; /* Duplicate macro name */
    }
    
    /* Create new macro node */
    new_node = malloc(sizeof(MacroNode));
    if (!new_node) {
        return FALSE;
    }
    
    /* Allocate and copy name */
    new_node->name = malloc(strlen(name) + 1);
    if (!new_node->name) {
        free(new_node);
        return FALSE;
    }
    strcpy(new_node->name, name);
    
    /* Allocate content array */
    new_node->content = malloc(line_count * sizeof(char*));
    if (!new_node->content) {
        free(new_node->name);
        free(new_node);
        return FALSE;
    }
    
    /* Copy content lines */
    for (i = 0; i < line_count; i++) {
        new_node->content[i] = malloc(strlen(content[i]) + 1);
        if (!new_node->content[i]) {
            /* Cleanup on failure */
            int j;
            for (j = 0; j < i; j++) {
                free(new_node->content[j]);
            }
            free(new_node->content);
            free(new_node->name);
            free(new_node);
            return FALSE;
        }
        strcpy(new_node->content[i], content[i]);
    }
    
    new_node->line_count = line_count;
    new_node->next = table->head;
    table->head = new_node;
    table->count++;
    
    return TRUE;
}

/* Find a macro by name */
MacroNode* find_macro(MacroTable *table, const char *name) {
    MacroNode *current;
    
    if (!table || !name) {
        return NULL;
    }
    
    current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/* Check if a name is a macro name */
Boolean is_macro_name(MacroTable *table, const char *name) {
    return find_macro(table, name) != NULL;
}

/* Check if a string is a valid label name */
Boolean is_valid_label_name(const char *name) {
    int i;
    
    if (!name || strlen(name) == 0 || strlen(name) > MAX_LABEL_LENGTH) {
        return FALSE;
    }
    
    /* Must start with a letter */
    if (!isalpha(name[0])) {
        return FALSE;
    }
    
    /* Can contain letters, digits, and underscores */
    for (i = 1; i < strlen(name); i++) {
        if (!isalnum(name[i]) && name[i] != '_') {
            return FALSE;
        }
    }
    
    return TRUE;
}

/* Check if a word is a reserved word */
Boolean is_reserved_word(const char *word) {
    /* Assembly instructions */
    const char *instructions[] = {
        "mov", "cmp", "add", "sub", "not", "clr", "lea", "inc", 
        "dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop"
    };
    
    /* Directives */
    const char *directives[] = {
        ".data", ".string", ".mat", ".entry", ".extern"
    };
    
    /* Registers */
    const char *registers[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
    };
    
    /* Reserved words */
    const char *reserved[] = {
        MACRO_START, MACRO_END
    };
    
    int i;
    
    if (!word) {
        return FALSE;
    }
    
    /* Check instructions */
    for (i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
        if (strcmp(word, instructions[i]) == 0) {
            return TRUE;
        }
    }
    
    /* Check directives */
    for (i = 0; i < sizeof(directives) / sizeof(directives[0]); i++) {
        if (strcmp(word, directives[i]) == 0) {
            return TRUE;
        }
    }
    
    /* Check registers */
    for (i = 0; i < sizeof(registers) / sizeof(registers[0]); i++) {
        if (strcmp(word, registers[i]) == 0) {
            return TRUE;
        }
    }
    
    /* Check reserved words */
    for (i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++) {
        if (strcmp(word, reserved[i]) == 0) {
            return TRUE;
        }
    }
    
    return FALSE;
}