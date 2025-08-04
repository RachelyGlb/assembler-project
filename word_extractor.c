#include "assembler.h"
#include "preassembler.h"

/* String processing helper functions */
char* get_first_word(const char *line) {
    char *word;
    char *temp_line;
    char *token;
    
    if (!line) {
        return NULL;
    }
    
    /* Create a copy of the line to work with */
    temp_line = malloc(strlen(line) + 1);
    if (!temp_line) {
        return NULL;
    }
    strcpy(temp_line, line);
    
    /* Get first token */
    token = strtok(temp_line, " \t\n\r");
    if (!token) {
        free(temp_line);
        return NULL;
    }
    
    /* Allocate and copy the first word */
    word = malloc(strlen(token) + 1);
    if (!word) {
        free(temp_line);
        return NULL;
    }
    strcpy(word, token);
    
    free(temp_line);
    return word;
}

char* get_second_word(const char *line) {
    char *word;
    char *temp_line;
    char *token;
    
    if (!line) {
        return NULL;
    }
    
    /* Create a copy of the line to work with */
    temp_line = malloc(strlen(line) + 1);
    if (!temp_line) {
        return NULL;
    }
    strcpy(temp_line, line);
    
    /* Get first token (skip it) */
    token = strtok(temp_line, " \t\n\r");
    if (!token) {
        free(temp_line);
        return NULL;
    }
    
    /* Get second token */
    token = strtok(NULL, " \t\n\r");
    if (!token) {
        free(temp_line);
        return NULL;
    }
    
    /* Allocate and copy the second word */
    word = malloc(strlen(token) + 1);
    if (!word) {
        free(temp_line);
        return NULL;
    }
    strcpy(word, token);
    
    free(temp_line);
    return word;
}



/* Check if there's extraneous text after the second word */
Boolean has_extraneous_text_after_second_word(const char *line) {
    char *temp_line;
    char *token;
    int word_count = 0;
    
    if (!line) {
        return FALSE;
    }
    
    /* Create a copy of the line to work with */
    temp_line = malloc(strlen(line) + 1);
    if (!temp_line) {
        return FALSE;
    }
    strcpy(temp_line, line);
    
    /* Count words */
    token = strtok(temp_line, " \t\n\r");
    while (token != NULL) {
        word_count++;
        token = strtok(NULL, " \t\n\r");
    }
    
    free(temp_line);
    
    /* Should have exactly 2 words: "macro" and "macro_name" */
    return (word_count > 2);
}
