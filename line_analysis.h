#ifndef LINE_ANALYSIS_H
#define LINE_ANALYSIS_H

#include <stdbool.h>

#include "symbol_table.h"


extern Symbol *symbol_table_head;  


bool is_comment_or_empty(const char *line);
bool is_label(const char *line);
bool is_data_or_string(const char *line);
bool is_command(const char *line);
bool extract_label(const char *line, char *label);
bool is_valid_label(const char *label);
bool label_exists(const char *label);
int count_data_items(const char *line);
int count_string_length(const char *line);
int count_command_words(const char *line);
bool is_register(const char *operand);
bool is_immediate(const char *operand);
int parse_operands(const char *line, char *operand1, char *operand2);
bool is_matrix(const char *operand);
bool is_label_operand(const char *operand);
int count_matrix_items(const char *line);
bool validate_command_line(const char *line, int line_number);



#endif
