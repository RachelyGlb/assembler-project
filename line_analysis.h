
#ifndef LINE_ANALYSIS_H
#define LINE_ANALYSIS_H

#include "types.h"


Boolean is_comment_or_empty(const char *line);
Boolean is_label(const char *line);
Boolean is_data_or_string(const char *line); 
Boolean is_command(const char *line);
Boolean is_register(const char *operand);
Boolean is_immediate(const char *operand);
Boolean is_matrix(const char *operand);
Boolean is_label_operand(const char *operand);


int extract_label(const char *line, char *label);
Boolean is_valid_label(const char *label);
Boolean label_exists(const char *label);
int count_data_items(const char *line);
int count_string_length(const char *line);
int count_command_words(const char *line);
int count_matrix_items(const char *line);
int parse_operands(const char *line, char *operand1, char *operand2);


int validate_command_line(const char *line, int line_number);


Boolean is_empty_line(const char *line);
Boolean is_comment_line(const char *line);
char *trim_whitespace(char *str);

#endif /* LINE_ANALYSIS_H */