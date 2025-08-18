/* instruction_table.h - instruction table */
#ifndef INSTRUCTION_TABLE_H
#define INSTRUCTION_TABLE_H

#include "types.h"

/* instruction table functions */
InstructionDef *find_instruction(const char *name);
Boolean is_valid_instruction(const char *name);
int get_instruction_operand_count(const char *name);

/* initialization functions */
void init_instruction_table(void);
void free_instruction_table(void);

#endif /* INSTRUCTION_TABLE_H */
