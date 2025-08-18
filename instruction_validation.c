/* instruction_validation.c - instruction validation */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "instruction_table.h"
#include "line_analysis.h"
#include "symbol_table.h"

/* operand types */
typedef enum
{
    OT_INVALID = -1,
    OT_IMMEDIATE = 0, /* #n        */
    OT_DIRECT,        /* label     */
    OT_MATRIX,        /* label[rX][rY]*/
    OT_REGISTER       /* r0..r7    */
} OperandType;


/* check if a string contains only lowercase letters */
int is_lowercase_only(const char *str)
{
    while (*str)
    {
        if (!islower((unsigned char)*str))
            return 0;
        str++;
    }
    return 1;
}



/* classify operand */
static OperandType classify_operand(const char *op)
{
    const char *i1, *i2, *p, *p2, *e1, *e2;

    if (!op || !*op)
        return OT_INVALID;

    if (is_immediate(op))
        return OT_IMMEDIATE;
    if (is_register(op))
        return OT_REGISTER;

    if (is_matrix(op))
    {
        p = strchr(op, '[');
        if (!p)
            return OT_INVALID;

        i1 = p + 1;
        e1 = strchr(i1, ']');
        if (!e1)
            return OT_INVALID;

        p2 = strchr(e1 + 1, '[');
        if (!p2)
            return OT_INVALID;

        i2 = p2 + 1;
        e2 = strchr(i2, ']');
        if (!e2)
            return OT_INVALID;

        /* verify that the indices are valid registers */
        if (!((e1 - i1) == 2 && i1[0] == 'r' && i1[1] >= '0' && i1[1] <= '7'))
            return OT_INVALID;
        if (!((e2 - i2) == 2 && i2[0] == 'r' && i2[1] >= '0' && i2[1] <= '7'))
            return OT_INVALID;

        return OT_MATRIX;
    }

    if (is_label_operand(op))
        return OT_DIRECT;

    return OT_INVALID;
}

/* validate operand count */
static int validate_operands_count(const InstructionDef *instr,
                                   const char *op1, const char *op2,
                                   int line_number)
{
    if (instr->operands_count == 0)
        return 1;

    if (instr->operands_count == 1)
    {
        if (!op1 || !*op1)
        {
            printf("Error on line %d: Missing operand for instruction '%s'.\n",
                   line_number, instr->name);
            return 0;
        }
        return 1;
    }

    if (instr->operands_count == 2)
    {
        if (!op1 || !*op1 || !op2 || !*op2)
        {
            printf("Error on line %d: Missing operands for instruction '%s'.\n",
                   line_number, instr->name);
            return 0;
        }
        return 1;
    }

    printf("Error on line %d: Internal: unsupported operands_count=%d for '%s'.\n",
           line_number, instr->operands_count, instr->name);
    return 0;
}

/* check whether operand types are allowed for the instruction */
static int is_type_allowed_for_instruction(const InstructionDef *ins,
                                           OperandType srcType,
                                           OperandType dstType,
                                           int line_number)
{
    const char *name = ins->name;

    if (ins->operands_count == 0)
        return 1;
    if (ins->operands_count >= 1 && dstType == OT_INVALID)
        return 0;
    if (ins->operands_count == 2 && srcType == OT_INVALID)
        return 0;

    if (!strcmp(name, "cmp"))
    {
        return 1; /* cmp accepts any operand type */
    }

    if (!strcmp(name, "mov") || !strcmp(name, "add") || !strcmp(name, "sub"))
    {
        return (dstType != OT_IMMEDIATE); /* destination cannot be immediate */
    }

    if (!strcmp(name, "lea"))
    {
        int src_ok = (srcType == OT_DIRECT || srcType == OT_MATRIX);
        int dst_ok = (dstType == OT_DIRECT || dstType == OT_REGISTER);
        return src_ok && dst_ok;
    }

    if (!strcmp(name, "not") || !strcmp(name, "clr") ||
        !strcmp(name, "inc") || !strcmp(name, "dec"))
    {
        return (dstType == OT_DIRECT || dstType == OT_MATRIX || dstType == OT_REGISTER);
    }

    if (!strcmp(name, "red"))
    {
        return (dstType == OT_DIRECT || dstType == OT_MATRIX || dstType == OT_REGISTER);
    }

    if (!strcmp(name, "prn"))
    {
        return 1; /* prn accepts any operand type */
    }

    if (!strcmp(name, "jmp") || !strcmp(name, "bne") || !strcmp(name, "jsr"))
    {
        return (dstType == OT_DIRECT || dstType == OT_MATRIX);
    }

    if (!strcmp(name, "rts") || !strcmp(name, "stop"))
    {
        return 1; /* no operands */
    }

    return 0;
}

/* validate operand types */
static int validate_operands_types(const InstructionDef *instr,
                                   const char *op1, const char *op2,
                                   int line_number)
{
    if (instr->operands_count == 0)
        return 1;

    if (instr->operands_count == 1)
    {
        OperandType t = classify_operand(op1);
        if (t == OT_INVALID)
        {
            printf("Error on line %d: Invalid operand '%s'.\n", line_number, op1);
            return 0;
        }
        if (!is_type_allowed_for_instruction(instr, OT_INVALID, t, line_number))
        {
            if (!strcmp(instr->name, "prn"))
            {
                printf("Error on line %d: Operand type not allowed for '%s'.\n",
                       line_number, instr->name);
            }
            else
            {
                printf("Error on line %d: Operand for '%s' must be direct/matrix/register (not immediate).\n",
                       line_number, instr->name);
            }
            return 0;
        }
        return 1;
    }

    if (instr->operands_count == 2)
    {
        OperandType t1 = classify_operand(op1);
        OperandType t2 = classify_operand(op2);
        if (t1 == OT_INVALID)
        {
            printf("Error on line %d: Invalid source operand '%s'.\n",
                   line_number, op1);
            return 0;
        }
        if (t2 == OT_INVALID)
        {
            printf("Error on line %d: Invalid destination operand '%s'.\n",
                   line_number, op2);
            return 0;
        }

        if (!is_type_allowed_for_instruction(instr, t1, t2, line_number))
        {
            if (!strcmp(instr->name, "lea"))
            {
                if (!(t1 == OT_DIRECT || t1 == OT_MATRIX))
                {
                    printf("Error on line %d: Source for 'lea' must be direct/matrix.\n", line_number);
                }
                else
                {
                    printf("Error on line %d: Destination for 'lea' must be direct/register.\n", line_number);
                }
            }
            else if (!strcmp(instr->name, "mov") || !strcmp(instr->name, "add") || !strcmp(instr->name, "sub"))
            {
                if (t2 == OT_IMMEDIATE)
                {
                    printf("Error on line %d: Destination cannot be immediate for '%s'.\n",
                           line_number, instr->name);
                }
                else
                {
                    printf("Error on line %d: Operand types not allowed for '%s'.\n",
                           line_number, instr->name);
                }
            }
            else
            {
                printf("Error on line %d: Operand types not allowed for '%s'.\n",
                       line_number, instr->name);
            }
            return 0;
        }
        return 1;
    }

    return 0;
}


/* validate an instruction line */
int validate_command_line(const char *line, int line_number)
{
    InstructionDef *instr;
    int count;
    char command[32] = "";
    char rest[256] = "";
    char op1[64] = "", op2[64] = "";

    if (sscanf(line, " %31s %255[^\n]", command, rest) < 1)
    {
        printf("Error on line %d: Empty or invalid line format.\n", line_number);
        return 0;
    }

    if (!is_lowercase_only(command))
    {
        printf("Error on line %d: Command name must contain only lowercase letters: '%s'\n",
               line_number, command);
        return 0;
    }

    instr = find_instruction(command);
    if (!instr)
    {
        printf("Error on line %d: Unknown instruction '%s'\n", line_number, command);
        return 0;
    }

    count = parse_operands(rest, op1, op2);

    if (count != instr->operands_count)
    {
        printf("Error on line %d: Instruction '%s' expects %d operands, got %d.\n",
               line_number, instr->name, instr->operands_count, count);
        return 0;
    }

    if (!validate_operands_count(instr, op1, op2, line_number))
        return 0;

    if (!validate_operands_types(instr, op1, op2, line_number))
        return 0;

    return 1;
}
