#include "assembler.h"
#include "preassembler.h"

/* Check if a file exists */
Boolean file_exists(const char *filename) {
    FILE *file;
    
    if (!filename) {
        return FALSE;
    }
    
    file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return TRUE;
    }
    
    return FALSE;
}

/* Create a filename with given extension */
char* create_filename_with_extension(const char *filename, const char *extension) {
    char *new_filename;
    
    if (!filename || !extension) {
        return NULL;
    }
    
    new_filename = malloc(strlen(filename) + strlen(extension) + 1);
    if (!new_filename) {
        return NULL;
    }
    
    strcpy(new_filename, filename);
    strcat(new_filename, extension);
    
    return new_filename;
}

/* Cleanup function for error situations */
void cleanup_and_exit(MacroTable *table, FILE *input, FILE *output) {
    if (table) {
        free_macro_table(table);
    }
    
    if (input) {
        fclose(input);
    }
    
    if (output) {
        fclose(output);
    }
}