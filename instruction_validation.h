/* instruction_validation.h - instruction validation */
#ifndef INSTRUCTION_VALIDATION_H
#define INSTRUCTION_VALIDATION_H

#include "types.h"
#include "line_analysis.h"

/* validation functions */
int validate_command_line(const char *line, int line_number);
int is_lowercase_only(const char *str);

#endif /* INSTRUCTION_VALIDATION_H */

