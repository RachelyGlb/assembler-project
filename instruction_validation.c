#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "instruction_table.h"
#include "line_analysis.h"
#include "symbol_table.h"

/* ===== שם פקודה באותיות קטנות ===== */
bool is_lowercase_only(const char *str) {
    while (*str) {
        if (!islower((unsigned char)*str)) return false;
        str++;
    }
    return true;
}

/* ===== חיפוש פקודה בטבלה ===== */
InstructionDef *find_instruction(const char *name) {
    for (int i = 0; instruction_table[i].name != NULL; i++) {
        if (strcmp(name, instruction_table[i].name) == 0)
            return &instruction_table[i];
    }
    return NULL;
}

/* ===== סיווג אופרנד ===== */
typedef enum {
    OT_INVALID = -1,
    OT_IMMEDIATE = 0, /* #n        */
    OT_DIRECT,        /* label     */
    OT_MATRIX,        /* label[rX][rY] – שני האינדקסים אוגרים */
    OT_REGISTER       /* r0..r7    */
} OperandType;

static OperandType classify_operand(const char *op) {
    if (!op || !*op) return OT_INVALID;

    if (is_immediate(op)) return OT_IMMEDIATE;
    if (is_register(op))  return OT_REGISTER;

    if (is_matrix(op)) {
    /* מצא את זוג הסוגריים הראשון והשני */
    const char *p  = strchr(op, '[');
    if (!p) return OT_INVALID;

    const char *i1 = p + 1;
    const char *e1 = strchr(i1, ']');
    if (!e1) return OT_INVALID;

    const char *p2 = strchr(e1 + 1, '[');
    if (!p2) return OT_INVALID;

    const char *i2 = p2 + 1;
    const char *e2 = strchr(i2, ']');
    if (!e2) return OT_INVALID;

    /* כל אינדקס חייב להיות בדיוק "rX" כאשר X ב-0..7 */
    if (!((e1 - i1) == 2 && i1[0] == 'r' && i1[1] >= '0' && i1[1] <= '7'))
        return OT_INVALID;
    if (!((e2 - i2) == 2 && i2[0] == 'r' && i2[1] >= '0' && i2[1] <= '7'))
        return OT_INVALID;

    return OT_MATRIX;

    }

    if (is_label_operand(op)) return OT_DIRECT;

    return OT_INVALID;
}

/* ===== בדיקת כמות אופרנדים ===== */
static bool validate_operands_count(const InstructionDef *instr,
                                    const char *op1, const char *op2,
                                    int line_number) {
    if (instr->operand_count == 0) return true;

    if (instr->operand_count == 1) {
        if (!op1 || !*op1) {
            printf("Error on line %d: Missing operand for instruction '%s'.\n",
                   line_number, instr->name);
            return false;
        }
        return true;
    }

    if (instr->operand_count == 2) {
        if (!op1 || !*op1 || !op2 || !*op2) {
            printf("Error on line %d: Missing operands for instruction '%s'.\n",
                   line_number, instr->name);
            return false;
        }
        return true;
    }

    printf("Error on line %d: Internal: unsupported operand_count=%d for '%s'.\n",
           line_number, instr->operand_count, instr->name);
    return false;
}

/* ===== כללי ההרשאה לפי שם פקודה — עבור כולן ===== */
static bool is_type_allowed_for_instruction(const InstructionDef *ins,
                                            OperandType srcType,
                                            OperandType dstType,
                                            int line_number) {
    const char *name = ins->name;

    if (ins->operand_count == 0) return true;
    if (ins->operand_count >= 1 && dstType == OT_INVALID) return false;
    if (ins->operand_count == 2 && srcType == OT_INVALID) return false;

    if (!strcmp(name, "cmp")) {
        /* שני הצדדים: כל סוג */
        return true;
    }

    if (!strcmp(name, "mov") || !strcmp(name, "add") || !strcmp(name, "sub")) {
        /* מקור: כל סוג; יעד: לא immediate */
        return (dstType != OT_IMMEDIATE);
    }

    if (!strcmp(name, "lea")) {
        bool src_ok = (srcType == OT_DIRECT || srcType == OT_MATRIX);
        bool dst_ok = (dstType == OT_DIRECT || dstType == OT_REGISTER);
        return src_ok && dst_ok;
    }

    if (!strcmp(name, "not") || !strcmp(name, "clr") ||
        !strcmp(name, "inc") || !strcmp(name, "dec")) {
        /* יחיד: direct/matrix/register */
        return (dstType == OT_DIRECT || dstType == OT_MATRIX || dstType == OT_REGISTER);
    }

    if (!strcmp(name, "red")) {
        return (dstType == OT_DIRECT || dstType == OT_MATRIX || dstType == OT_REGISTER);
    }

    if (!strcmp(name, "prn")) {
        /* מותר הכל (כולל immediate) */
        return true;
    }

    if (!strcmp(name, "jmp") || !strcmp(name, "bne") || !strcmp(name, "jsr")) {
        return (dstType == OT_DIRECT || dstType == OT_MATRIX);
    }

    if (!strcmp(name, "rts") || !strcmp(name, "stop")) {
        return true;
    }

    return false;
}

/* ===== בדיקת סוגי אופרנדים ===== */
static bool validate_operands_types(const InstructionDef *instr,
                                    const char *op1, const char *op2,
                                    int line_number) {
    if (instr->operand_count == 0) return true;

    if (instr->operand_count == 1) {
        OperandType t = classify_operand(op1);
        if (t == OT_INVALID) {
            printf("Error on line %d: Invalid operand '%s'.\n", line_number, op1);
            return false;
        }
        if (!is_type_allowed_for_instruction(instr, OT_INVALID, t, line_number)) {
            if (!strcmp(instr->name, "prn")) {
                printf("Error on line %d: Operand type not allowed for '%s'.\n",
                       line_number, instr->name);
            } else {
                printf("Error on line %d: Operand for '%s' must be direct/matrix/register (not immediate).\n",
                       line_number, instr->name);
            }
            return false;
        }
        return true;
    }

    if (instr->operand_count == 2) {
        OperandType t1 = classify_operand(op1); /* מקור */
        OperandType t2 = classify_operand(op2); /* יעד  */

        if (t1 == OT_INVALID) {
            printf("Error on line %d: Invalid source operand '%s'.\n",
                   line_number, op1);
            return false;
        }
        if (t2 == OT_INVALID) {
            printf("Error on line %d: Invalid destination operand '%s'.\n",
                   line_number, op2);
            return false;
        }

        if (!is_type_allowed_for_instruction(instr, t1, t2, line_number)) {
            if (!strcmp(instr->name, "lea")) {
                if (!(t1 == OT_DIRECT || t1 == OT_MATRIX)) {
                    printf("Error on line %d: Source for 'lea' must be direct/matrix.\n", line_number);
                } else {
                    printf("Error on line %d: Destination for 'lea' must be direct/register.\n", line_number);
                }
            } else if (!strcmp(instr->name, "mov") || !strcmp(instr->name, "add") || !strcmp(instr->name, "sub")) {
                if (t2 == OT_IMMEDIATE) {
                    printf("Error on line %d: Destination cannot be immediate for '%s'.\n",
                           line_number, instr->name);
                } else {
                    printf("Error on line %d: Operand types not allowed for '%s'.\n",
                           line_number, instr->name);
                }
            } else {
                printf("Error on line %d: Operand types not allowed for '%s'.\n",
                       line_number, instr->name);
            }
            return false;
        }
        return true;
    }

    return false;
}

/* ===== בדיקת שורת פקודה שלמה ===== */
bool validate_command_line(const char *line, int line_number) {
    char command[32] = "";
    char rest[256]   = "";
    char op1[64]     = "", op2[64] = "";

    if (sscanf(line, " %31s %255[^\n]", command, rest) < 1) {
        printf("Error on line %d: Empty or invalid line format.\n", line_number);
        return false;
    }

    if (!is_lowercase_only(command)) {
        printf("Error on line %d: Command name must contain only lowercase letters: '%s'\n",
               line_number, command);
        return false;
    }

    InstructionDef *instr = find_instruction(command);
    if (!instr) {
        printf("Error on line %d: Unknown instruction '%s'\n", line_number, command);
        return false;
    }

    int count = parse_operands(rest, op1, op2);

    if (count != instr->operand_count) {
        printf("Error on line %d: Instruction '%s' expects %d operands, got %d.\n",
               line_number, instr->name, instr->operand_count, count);
        return false;
    }
    if (!validate_operands_count(instr, op1, op2, line_number)) return false;

    if (!validate_operands_types(instr, op1, op2, line_number)) return false;

    return true;
}
