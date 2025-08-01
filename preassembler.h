#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Constants */
#define MAX_LINE_LENGTH 81
#define MAX_LABEL_LENGTH 30
#define MAX_FILENAME_LENGTH 256
#define MEMORY_SIZE 256


/* Macro keywords */
#define MACRO_START "macro"
#define MACRO_END "macroend"

/* Macro structure */
typedef struct MacroNode {
    char *name;
    char **content;
    int line_count;
    struct MacroNode *next;
} MacroNode;

/* Macro table */
typedef struct {
    MacroNode *head;
    int count;
} MacroTable;

/* Error types */
typedef enum {
    NO_ERROR = 0,
    LINE_TOO_LONG,
    INVALID_MACRO_NAME,
    LABEL_ON_MACRO_LINE,
    EXTRANEOUS_TEXT,
    MEMORY_ALLOCATION_ERROR,
    FILE_ERROR,
    MACRO_NOT_CLOSED,
    DUPLICATE_MACRO_NAME
} ErrorType;

/* Macro handling functions */
MacroTable* create_macro_table(void);
void free_macro_table(MacroTable *table);
Boolean add_macro(MacroTable *table, const char *name, char **content, int line_count);
MacroNode* find_macro(MacroTable *table, const char *name);
Boolean is_macro_name(MacroTable *table, const char *name);