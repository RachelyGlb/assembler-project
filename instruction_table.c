/*
 * instruction_table.c - the words of the assembly language.
 *
 * The three tables below are the only place where the operations, the
 * registers and the directives are listed. They are static and constant
 * (IMP-1). The set of legal addressing modes of an operand is stored as a
 * bit mask with one bit per mode, so a legality test is a single lookup.
 */
#include <stddef.h>
#include <string.h>

#include "instruction_table.h"

/* The bit that stands for one addressing mode in a set of modes. */
#define MODE_BIT(mode) (1u << (mode))

/* The sets of legal modes that occur in SPEC section 3. */
#define MODES_NONE 0u
#define MODES_ALL                                               \
    (MODE_BIT(ADDRESSING_IMMEDIATE) | MODE_BIT(ADDRESSING_DIRECT) | \
     MODE_BIT(ADDRESSING_MATRIX) | MODE_BIT(ADDRESSING_REGISTER))
#define MODES_ADDRESSABLE \
    (MODE_BIT(ADDRESSING_DIRECT) | MODE_BIT(ADDRESSING_MATRIX))
#define MODES_WRITABLE (MODES_ADDRESSABLE | MODE_BIT(ADDRESSING_REGISTER))

/* The number of elements of an array object. */
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

/* The position of the name of a directive after its leading dot. */
#define DIRECTIVE_DOT_LENGTH 1

/* One row of the instruction table. */
struct Instruction
{
    const char *name;               /* The operation name. */
    int opcode;                     /* The opcode, bits 9-6 of word one. */
    int operand_count;              /* The number of operands. */
    unsigned int source_modes;      /* The set of legal source modes. */
    unsigned int destination_modes; /* The set of legal destination modes. */
};

/* One row of the directive table. */
typedef struct
{
    const char *name;   /* The directive name, with its dot. */
    DirectiveKind kind; /* The directive it names. */
} Directive;

/* The instruction table of SPEC section 3 (INS-1, INS-3, INS-4). */
static const Instruction instruction_table[] = {
    {"mov", 0, 2, MODES_ALL, MODES_WRITABLE},
    {"cmp", 1, 2, MODES_ALL, MODES_ALL},
    {"add", 2, 2, MODES_ALL, MODES_WRITABLE},
    {"sub", 3, 2, MODES_ALL, MODES_WRITABLE},
    {"lea", 4, 2, MODES_ADDRESSABLE, MODES_WRITABLE},
    {"clr", 5, 1, MODES_NONE, MODES_WRITABLE},
    {"not", 6, 1, MODES_NONE, MODES_WRITABLE},
    {"inc", 7, 1, MODES_NONE, MODES_WRITABLE},
    {"dec", 8, 1, MODES_NONE, MODES_WRITABLE},
    {"jmp", 9, 1, MODES_NONE, MODES_WRITABLE},
    {"bne", 10, 1, MODES_NONE, MODES_WRITABLE},
    {"jsr", 11, 1, MODES_NONE, MODES_WRITABLE},
    {"red", 12, 1, MODES_NONE, MODES_WRITABLE},
    {"prn", 13, 1, MODES_NONE, MODES_ALL},
    {"rts", 14, 0, MODES_NONE, MODES_NONE},
    {"stop", 15, 0, MODES_NONE, MODES_NONE}};

/* The register names; the index of a name is the register number (M-2). */
static const char *const register_names[REGISTER_COUNT] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"};

/* The directive table (DIR-4). */
static const Directive directive_table[] = {
    {".data", DIRECTIVE_DATA},
    {".string", DIRECTIVE_STRING},
    {".mat", DIRECTIVE_MAT},
    {".entry", DIRECTIVE_ENTRY},
    {".extern", DIRECTIVE_EXTERN}};

/* Looks up an operation by its name; see instruction_table.h. */
const Instruction *instruction_find(const char *name)
{
    size_t index;

    for (index = 0; index < ARRAY_LENGTH(instruction_table); index++)
    {
        if (strcmp(instruction_table[index].name, name) == 0)
        {
            return &instruction_table[index];
        }
    }
    return NULL;
}

/* Returns the name of an operation; see instruction_table.h. */
const char *instruction_name(const Instruction *instruction)
{
    return instruction->name;
}

/* Returns the opcode of an operation; see instruction_table.h. */
int instruction_opcode(const Instruction *instruction)
{
    return instruction->opcode;
}

/* Returns the number of operands of an operation; see instruction_table.h. */
int instruction_operand_count(const Instruction *instruction)
{
    return instruction->operand_count;
}

/* Tells whether a source mode is legal; see instruction_table.h. */
Boolean instruction_allows_source(const Instruction *instruction,
                                  AddressingMode mode)
{
    return (instruction->source_modes & MODE_BIT(mode)) != 0 ? TRUE : FALSE;
}

/* Tells whether a destination mode is legal; see instruction_table.h. */
Boolean instruction_allows_destination(const Instruction *instruction,
                                       AddressingMode mode)
{
    return (instruction->destination_modes & MODE_BIT(mode)) != 0 ? TRUE
                                                                  : FALSE;
}

/* Looks up a register by its name; see instruction_table.h. */
int register_number(const char *name)
{
    int number;

    for (number = 0; number < REGISTER_COUNT; number++)
    {
        if (strcmp(register_names[number], name) == 0)
        {
            return number;
        }
    }
    return NOT_A_REGISTER;
}

/* Looks up a directive by its name with the dot; see instruction_table.h. */
DirectiveKind directive_find(const char *name)
{
    size_t index;

    for (index = 0; index < ARRAY_LENGTH(directive_table); index++)
    {
        if (strcmp(directive_table[index].name, name) == 0)
        {
            return directive_table[index].kind;
        }
    }
    return DIRECTIVE_NONE;
}

/*
 * Tells whether a word is the name of a directive written without its dot.
 *   word  the candidate name
 * Returns TRUE for "data", "string", "mat", "entry" and "extern".
 */
static Boolean is_directive_word(const char *word)
{
    size_t index;

    for (index = 0; index < ARRAY_LENGTH(directive_table); index++)
    {
        if (strcmp(directive_table[index].name + DIRECTIVE_DOT_LENGTH,
                   word) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* The one test for a reserved word (LBL-20); see instruction_table.h. */
Boolean is_reserved_word(const char *word)
{
    if (instruction_find(word) != NULL ||
        register_number(word) != NOT_A_REGISTER || is_directive_word(word))
    {
        return TRUE;
    }
    return (strcmp(word, MACRO_START_KEYWORD) == 0 ||
            strcmp(word, MACRO_END_KEYWORD) == 0)
               ? TRUE
               : FALSE;
}
