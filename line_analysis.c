#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


#include "symbol_table.h"
#include "line_analysis.h"

#define MAX_LABEL_LENGTH 31
#define MAX_LINE_LENGTH 80


Symbol *symbol_table_head = NULL;  

bool extract_label(const char *line, char *label) {
 
    const char *colon = strchr(line, ':');
    if (colon) {
        int len = colon - line;
        if (len < MAX_LABEL_LENGTH) {
            strncpy(label, line, len);
            label[len] = '\0';
            return true;
        }
    }
    return false;
}

bool is_valid_label(const char *label) {
    if (!label || strlen(label) == 0 || strlen(label) >= MAX_LABEL_LENGTH)
        return false;
    if (!isalpha(label[0]))
        return false;
    for (int i = 1; label[i]; i++) {
        if (!isalnum(label[i]))
            return false;
    }
    return true;
}

bool label_exists(const char *label) {
    return find_symbol(label) != NULL;
}

bool is_comment_or_empty(const char *line) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return (*line == ';' || *line == '\0' || *line == '\n');
}

bool is_label(const char *line) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    const char *colon = strchr(line, ':');
    if (!colon) return false;

  
    for (const char *p = line; p < colon; p++) {
        if (!(('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') || ('0' <= *p && *p <= '9')))
            return false;
    }
    return true;
}

bool is_data_or_string(const char *line) {
    return (strstr(line, ".data") != NULL || strstr(line, ".string") != NULL || strstr(line, ".mat") != NULL);
}

bool is_command(const char *line) {
    static const char *ops[] = {
        "mov","cmp","add","sub","lea",
        "clr","not","inc","dec",
        "jmp","bne","jsr",
        "red","prn",
        "rts","stop",
        NULL
    };

    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;

    const char *colon = strchr(p, ':');
    if (colon) {
        bool looks_like_label = true;
        for (const char *q = p; q < colon; q++) {
            if (!isalnum((unsigned char)*q)) { looks_like_label = false; break; }
        }
        if (looks_like_label) {
            p = colon + 1;
            while (*p && isspace((unsigned char)*p)) p++;
        }
    }

    char op[16] = {0};
    sscanf(p, "%15s", op);
    if (op[0] == '\0') return false;

    for (int i = 0; ops[i] != NULL; i++) {
        if (strcmp(op, ops[i]) == 0) return true;
    }
    return false;
}

int count_data_items(const char *line) {
    const char *data_start = strstr(line, ".data");
    if (!data_start) return 0;

    data_start += strlen(".data");

    int count = 0;
    char *token;
    char buffer[256];
    strcpy(buffer, data_start);

    token = strtok(buffer, ",");
    while (token != NULL) {
        while (isspace(*token)) token++;
        if (*token != '\0' && *token != '\n') {
            count++;
        }
        token = strtok(NULL, ",");
    }

    return count;
}

int count_matrix_items(const char *line) {
    int rows = 0, cols = 0;
    const char *start = strchr(line, '[');
    if (start) {
        sscanf(start, "[%d][%d]", &rows, &cols);
        return rows * cols;
    }
    return 0;
}



/* עוזרת: בודקת אם האופרטנד הוא אוגר */
bool is_register(const char *operand) {
    return operand != NULL && strlen(operand) == 2 &&
           operand[0] == 'r' && operand[1] >= '0' && operand[1] <= '7';
}

/* עוזרת: בודקת אם האופרטנד הוא immediate value (מתחיל ב-#) */
bool is_immediate(const char *operand) {
    return operand != NULL && operand[0] == '#';
}

bool is_label_operand(const char *operand) {
    return operand != NULL &&
           !is_register(operand) &&
           !is_immediate(operand) &&
           !is_matrix(operand);
}


/* עוזרת: סופרת מספר האופרטנדים (0, 1 או 2) וממלאת אותם */
int parse_operands(const char *line, char *operand1, char *operand2) {
    const char *comma = strchr(line, ',');
    if (!comma) {
        sscanf(line, "%s", operand1);
        return strlen(operand1) > 0 ? 1 : 0;
    } else {
        sscanf(line, "%[^,], %s", operand1, operand2);
        return 2;
    }
}

bool is_matrix(const char *operand) {
    return operand != NULL && strchr(operand, '[') && strchr(operand, ']');
}

int count_string_length(const char *line) {
    /* סופרת תווי המחרוזת שבין מירכאות, ומוסיפה 1 עבור '\0' */
    const char *p = strchr(line, '"');
    int count = 0;

    if (!p) return 0;      /* אין פתיחת מירכאות */
    p++;                   /* אחרי המירכאות הראשונה */

    while (*p && *p != '"') {
        count++;
        p++;
    }

    return count + 1;      /* +1 עבור תו סיום המחרוזת */
}

/* הפונקציה הראשית לספירת מילים עבור פקודת אסמבלי */
int count_command_words(const char *line) {
    char command[MAX_LINE_LENGTH] = {0};
    char operands_part[MAX_LINE_LENGTH] = {0};
    char operand1[MAX_LINE_LENGTH] = {0}, operand2[MAX_LINE_LENGTH] = {0};
    int words = 1;  /* מילה עבור הopcode עצמו */
    int num_operands;

    if (sscanf(line, "%s %[^\n]", command, operands_part) < 1) {
        return 0;  /* שורה ריקה או לא תקינה */
    }

    /* פקודות בלי אופרנדים */
    if (strcmp(command, "stop") == 0 || strcmp(command, "rts") == 0) {
        return 1;
    }

    /* כמה אופרנדים יש ומה הם */
    num_operands = parse_operands(operands_part, operand1, operand2);

    if (num_operands == 1) {
        if (is_register(operand1) || is_immediate(operand1) || is_label_operand(operand1)) {
            words += 1;
        } else if (is_matrix(operand1)) {
            words += 2;
        } else {
            words += 1; /* ברירת מחדל */
        }
    } else if (num_operands == 2) {
        bool reg1 = is_register(operand1);
        bool reg2 = is_register(operand2);

        if (reg1 && reg2) {
            words += 1; /* שני רגיסטרים – מילה אחת */
        } else {
            if (is_register(operand1) || is_immediate(operand1) || is_label_operand(operand1)) {
                words += 1;
            } else if (is_matrix(operand1)) {
                words += 2;
            }

            if (is_register(operand2) || is_immediate(operand2) || is_label_operand(operand2)) {
                words += 1;
            } else if (is_matrix(operand2)) {
                words += 2;
            }
        }
    }

    return words;
}
