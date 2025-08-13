#include <stddef.h>

#include "instruction_table.h"

/* מספר האופרנדים לכל פקודה לפי המפרט */
InstructionDef instruction_table[] = {
    {"mov",  GROUP_ONE,   2},
    {"cmp",  GROUP_ONE,   2},
    {"add",  GROUP_ONE,   2},
    {"sub",  GROUP_ONE,   2},
    {"lea",  GROUP_ONE,   2},

    {"clr",  GROUP_TWO,   1},
    {"not",  GROUP_TWO,   1},
    {"inc",  GROUP_TWO,   1},
    {"dec",  GROUP_TWO,   1},
    {"jmp",  GROUP_TWO,   1},
    {"bne",  GROUP_TWO,   1},
    {"jsr",  GROUP_TWO,   1},
    {"red",  GROUP_TWO,   1},
    {"prn",  GROUP_TWO,   1},

    {"rts",  GROUP_THREE, 0},
    {"stop", GROUP_THREE, 0},

    {NULL,   0,           0}
};
