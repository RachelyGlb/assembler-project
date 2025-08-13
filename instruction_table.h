#ifndef INSTRUCTION_TABLE_H
#define INSTRUCTION_TABLE_H

#include <stdbool.h>

#define MAX_LABEL_LENGTH 31

/* קבוצות לפי המפרט שלך */
typedef enum {
    GROUP_THREE,   /* stop, rts*/
    GROUP_TWO,     /* clr, not, inc, dec, jmp, bne, red, prn, jsr*/
    GROUP_ONE     /* mov, cmp, add, sub, lea*/
} InstructionGroup;

typedef struct {
    const char *name;
    InstructionGroup group;
    int operand_count;
} InstructionDef;

/* הטבלה מוגדרת ב-instruction_table.c */
extern InstructionDef instruction_table[];

/* הצהרות (ימומשו בקובץ הוולידציה) */
bool is_lowercase_only(const char *word);
InstructionDef *find_instruction(const char *name);

#endif /* INSTRUCTION_TABLE_H */
