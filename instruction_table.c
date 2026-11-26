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
    {"lea", 4, 2},
    {"clr", 5, 1},
    {"not", 6, 1},
    {"inc", 7, 1},
    {"dec", 8, 1},
    {"jmp", 9, 1},
    {"bne", 10, 1},
    {"jsr", 11, 1},
    {"red", 12, 1},
    {"prn", 13, 1},
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
