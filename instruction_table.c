/* instruction_table.c - instruction table */
#include "instruction_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* static instruction table */
static InstructionDef instructions[] = {
    {"mov", 0, 2},
    {"cmp", 1, 2},
    {"add", 2, 2},
    {"sub", 3, 2},
    {"not", 4, 1},
    {"clr", 5, 1},
    {"lea", 6, 2},
    {"inc", 7, 1},
    {"dec", 8, 1},
    {"jmp", 9, 1},
    {"bne", 10, 1},
    {"red", 11, 1},
    {"prn", 12, 1},
    {"jsr", 13, 1},
    {"rts", 14, 0},
    {"stop", 15, 0}};

#define INSTRUCTION_COUNT 16

/* find instruction by name */
InstructionDef *find_instruction(const char *name)
{
    int i;

    if (!name)
        return NULL;

    for (i = 0; i < INSTRUCTION_COUNT; i++)
    {
        if (strcmp(instructions[i].name, name) == 0)
        {
            return &instructions[i];
        }
    }

    return NULL;
}

/* check if instruction is valid */
Boolean is_valid_instruction(const char *name)
{
    return (find_instruction(name) != NULL);
}

/* get operand count for an instruction */
int get_instruction_operand_count(const char *name)
{
    InstructionDef *instr = find_instruction(name);

    if (instr)
    {
        return instr->operands_count;
    }

    return -1; /* instruction not found */
}

/* initialize instruction table */
void init_instruction_table(void)
{
    /* table is static, no initialization required */
}

/* free instruction table memory */
void free_instruction_table(void)
{
    /* table is static, no free required */
}
