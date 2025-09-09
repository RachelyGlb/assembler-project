#include "preassembler.h"

/* Check if a name is a macro name */
Boolean is_macro_name(GenericTable *table, const char *name)
{
    return find_in_generic_table(table, name) != NULL;
}

/* Check if a word is a reserved word */
Boolean is_reserved_word(const char *word)
{
    /* Assembly instructions */
    const char *instructions[] = {
        "mov", "cmp", "add", "sub", "lea", "clr", "not", "inc",
        "dec", "jmp", "bne", "jsr", "red", "prn", "rts", "stop"};

    /* Directives */
    const char *directives[] = {
        ".data", ".string", ".mat", ".entry", ".extern"};

    /* Registers */
    const char *registers[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"};

    /* Reserved words */
    const char *reserved[] = {
        MACRO_START, MACRO_END};

    int i;

    if (!word)
    {
        return FALSE;
    }

    /* Check instructions */
    for (i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++)
    {
        if (strcmp(word, instructions[i]) == 0)
        {
            return TRUE;
        }
    }

    /* Check directives */
    for (i = 0; i < sizeof(directives) / sizeof(directives[0]); i++)
    {
        if (strcmp(word, directives[i]) == 0)
        {
            return TRUE;
        }
    }

    /* Check registers */
    for (i = 0; i < sizeof(registers) / sizeof(registers[0]); i++)
    {
        if (strcmp(word, registers[i]) == 0)
        {
            return TRUE;
        }
    }

    /* Check reserved words */
    for (i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++)
    {
        if (strcmp(word, reserved[i]) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

GenericTable *create_generic_table(void)
{
    GenericTable *table = malloc(sizeof(GenericTable));
    if (!table)
    {
        return NULL;
    }

    table->head = NULL;
    table->count = 0;

    return table;
}

/* Free generic table with custom data cleanup function */
void free_generic_table(GenericTable *table, void (*cleanup_data)(void *))
{
    GenericNode *current;
    GenericNode *next;

    if (!table)
    {
        return;
    }

    current = table->head;
    while (current)
    {
        next = current->next;

        if (current->name)
        {
            free(current->name);
        }

        if (current->data && cleanup_data)
        {
            cleanup_data(current->data);
        }

        free(current);
        current = next;
    }

    free(table);
}

/* Add item to generic table */
Boolean add_to_generic_table(GenericTable *table, const char *name, int line_number, void *data)
{
    GenericNode *new_node;

    if (!table || !name)
    {
        return FALSE;
    }

    /* Check for duplicates */
    if (find_in_generic_table(table, name))
    {
        return FALSE;
    }

    /* Create new node */
    new_node = malloc(sizeof(GenericNode));
    if (!new_node)
    {
        return FALSE;
    }

    /* Copy name */
    new_node->name = malloc(strlen(name) + 1);
    if (!new_node->name)
    {
        free(new_node);
        return FALSE;
    }
    strcpy(new_node->name, name);

    new_node->line_number = line_number;
    new_node->data = data;
    new_node->next = table->head;
    table->head = new_node;
    table->count++;

    return TRUE;
}

/* Find item in generic table */
GenericNode *find_in_generic_table(GenericTable *table, const char *name)
{
    GenericNode *current;

    if (!table || !name)
    {
        return NULL;
    }

    current = table->head;
    while (current)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/* ===== Specific implementations using generic functions ===== */

/* Macro cleanup function */
void cleanup_macro_data(void *data)
{
    MacroData *macro_data = (MacroData *)data;
    int i;

    if (!macro_data)
        return;

    if (macro_data->content)
    {
        for (i = 0; i < macro_data->line_count; i++)
        {
            if (macro_data->content[i])
            {
                free(macro_data->content[i]);
            }
        }
        free(macro_data->content);
    }

    free(macro_data);
}

/* Create macro table using generic table */
GenericTable *create_macro_table(void)
{
    return create_generic_table();
}

/* Free macro table using generic cleanup */
void free_macro_table(GenericTable *table)
{
    free_generic_table(table, cleanup_macro_data);
}

/* Add macro using generic function */
Boolean add_macro(GenericTable *table, const char *name, char **content, int line_count)
{
    MacroData *macro_data;
    int i;

    /* Create macro data */
    macro_data = malloc(sizeof(MacroData));
    if (!macro_data)
    {
        return FALSE;
    }

    /* Copy content */
    macro_data->content = malloc(line_count * sizeof(char *));
    if (!macro_data->content)
    {
        free(macro_data);
        return FALSE;
    }

    for (i = 0; i < line_count; i++)
    {
        macro_data->content[i] = malloc(strlen(content[i]) + 1);
        if (!macro_data->content[i])
        {
            /* Cleanup on failure */
            int j;
            for (j = 0; j < i; j++)
            {
                free(macro_data->content[j]);
            }
            free(macro_data->content);
            free(macro_data);
            return FALSE;
        }
        strcpy(macro_data->content[i], content[i]);
    }

    macro_data->line_count = line_count;

    return add_to_generic_table(table, name, 0, macro_data);
}

/* Find macro using generic function */
GenericNode *find_macro(GenericTable *table, const char *name)
{
    return find_in_generic_table(table, name);
}

/* Create label table using generic table */
GenericTable *create_label_table(void)
{
    return create_generic_table();
}

/* Free label table (no special cleanup needed) */
void free_label_table(GenericTable *table)
{
    free_generic_table(table, NULL);
}

/* Add label using generic function */
Boolean add_label_to_table(GenericTable *table, const char *name, int line_number)
{
    return add_to_generic_table(table, name, line_number, NULL);
}

/* Check if label exists */
Boolean is_label_already_defined(GenericTable *table, const char *name)
{
    return find_in_generic_table(table, name) != NULL;
}

/* ===== Smart label processing functions ===== */

/* Enhanced string utilities using existing functions */
char *extract_until_char(const char *line, char delimiter)
{
    char *pos = strchr(line, delimiter);
    char *result;
    int len;

    if (!pos)
        return NULL;

    len = pos - line;
    result = malloc(len + 1);
    if (!result)
        return NULL;

    strncpy(result, line, len);
    result[len] = '\0';

    /* Trim whitespace using existing function */
    return trim_whitespace(result);
}

/* Extract label name (reusing existing string functions) */
char *extract_label_name(const char *line)
{
    return extract_until_char(line, ':');
}

/* Get content after character */
/* Get content after character */
char *get_content_after_char(const char *line, char delimiter)
{
    char *pos = strchr(line, delimiter);
    char *result;
    char *trimmed_result;

    if (!pos)
        return NULL;

    pos++; /* Skip the delimiter */

    /* Allocate new string for the result */
    result = malloc(strlen(pos) + 1);
    if (!result)
        return NULL;

    strcpy(result, pos);

    /* Now trim in place - this is safe because we own the memory */
    trimmed_result = trim_whitespace(result);

    /* If trimming moved the pointer, we need to move the content */
    if (trimmed_result != result)
    {
        memmove(result, trimmed_result, strlen(trimmed_result) + 1);
    }

    return result;
}

/* Get content after label */
char *get_content_after_label(const char *line)
{
    return get_content_after_char(line, ':');
}

/* Simple label detection */
Boolean has_label(const char *line)
{
    char *colon_pos = strchr(line, ':');
    char *trimmed;

    if (!colon_pos) return FALSE;

    trimmed = trim_whitespace((char *)line);
    if (trimmed == colon_pos)
        return FALSE;

    return TRUE;
}

/* Validate name (unified for both labels and macros) */
Boolean is_valid_name(const char *name, Boolean allow_underscore)
{
    int i;

    if (!name || strlen(name) == 0 || strlen(name) > MAX_LABEL_LENGTH)
    {
        return FALSE;
    }

    /* Must start with a letter */
    if (!isalpha(name[0]))
    {
        return FALSE;
    }

    /* Check each character */
    for (i = 1; i < strlen(name); i++)
    {
        if (!isalnum(name[i]))
        {
            if (!allow_underscore || name[i] != '_')
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/* Label validation (no underscore allowed) */
Boolean is_valid_label_name(const char *name)
{
    return is_valid_name(name, FALSE);
}

/* Macro validation (underscore allowed) */
Boolean is_valid_macro_name(const char *name)
{
    return is_valid_name(name, TRUE);
}
