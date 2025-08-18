#include "line_analysis.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Symbol *symbol_table_head = NULL;

int extract_label(const char *line, char *label)
{
    const char *colon = strchr(line, ':');
    if (colon)
    {
        int len = colon - line;
        if (len < MAX_LABEL_LENGTH)
        {
            strncpy(label, line, len);
            label[len] = '\0';
            return 1;
        }
    }
    return 0;
}

Boolean is_valid_label(const char *label)
{
    int i;
    if (!label || strlen(label) == 0 || strlen(label) >= MAX_LABEL_LENGTH)
        return FALSE;
    if (!isalpha(label[0]))
        return FALSE;
    for (i = 1; label[i]; i++)
    {
        if (!isalnum(label[i]))
            return FALSE;
    }
    return TRUE;
}

Boolean label_exists(const char *label)
{
    return find_symbol(label) ? TRUE : FALSE;
}

Boolean is_comment_or_empty(const char *line)
{
    while (*line == ' ' || *line == '\t')
    {
        line++;
    }
    return (*line == ';' || *line == '\0' || *line == '\n') ? TRUE : FALSE;
}

Boolean is_label(const char *line)
{
    const char *p;
    const char *colon;

    while (*line == ' ' || *line == '\t')
    {
        line++;
    }

    colon = strchr(line, ':');
    if (!colon)
        return FALSE;

    for (p = line; p < colon; p++)
    {
        if (!(('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') || ('0' <= *p && *p <= '9')))
            return FALSE;
    }
    return TRUE;
}

Boolean is_data_or_string(const char *line)
{
    return (strstr(line, ".data") != NULL ||
            strstr(line, ".string") != NULL ||
            strstr(line, ".mat") != NULL)
               ? TRUE
               : FALSE;
}

Boolean is_command(const char *line)
{
    char op[16] = {0};
    const char *q;
    int i;
    const char *colon;
    static const char *ops[] = {
        "mov", "cmp", "add", "sub", "lea",
        "clr", "not", "inc", "dec",
        "jmp", "bne", "jsr",
        "red", "prn",
        "rts", "stop",
        NULL};

    const char *p = line;
    while (*p && isspace((unsigned char)*p))
        p++;

    colon = strchr(p, ':');
    if (colon)
    {
        int looks_like_label = 1;
        for (q = p; q < colon; q++)
        {
            if (!isalnum((unsigned char)*q))
            {
                looks_like_label = 0;
                break;
            }
        }
        if (looks_like_label)
        {
            p = colon + 1;
            while (*p && isspace((unsigned char)*p))
                p++;
        }
    }

    sscanf(p, "%15s", op);
    if (op[0] == '\0')
        return FALSE;

    for (i = 0; ops[i] != NULL; i++)
    {
        if (strcmp(op, ops[i]) == 0)
            return TRUE;
    }

    return FALSE;
}

int count_data_items(const char *line)
{
    int count = 0;
    char *token;
    char buffer[256];
    const char *data_start = strstr(line, ".data");
    if (!data_start)
        return 0;

    data_start += strlen(".data");
    strcpy(buffer, data_start);

    token = strtok(buffer, ",");
    while (token != NULL)
    {
        while (isspace(*token))
            token++;
        if (*token != '\0' && *token != '\n')
        {
            count++;
        }
        token = strtok(NULL, ",");
    }

    return count;
}

int count_matrix_items(const char *line)
{
    int rows = 0, cols = 0;
    const char *start = strchr(line, '[');
    if (start)
    {
        sscanf(start, "[%d][%d]", &rows, &cols);
        return rows * cols;
    }
    return 0;
}

Boolean is_register(const char *operand)
{
    return (operand != NULL && strlen(operand) == 2 &&
            operand[0] == 'r' && operand[1] >= '0' && operand[1] <= '7')
               ? TRUE
               : FALSE;
}

/* function to parse operands */
int parse_operands(const char *rest, char *op1, char *op2)
{
    char *comma;
    int count = 0;

    op1[0] = '\0';
    op2[0] = '\0';

    if (!rest || !*rest)
        return 0;

    comma = strchr(rest, ',');

    if (!comma)
    {
        sscanf(rest, " %63s", op1);
        if (op1[0] != '\0')
            count = 1;
    }
    else
    {
        char temp[256];
        strncpy(temp, rest, (size_t)(comma - rest));
        temp[comma - rest] = '\0';
        sscanf(temp, " %63s", op1);
        sscanf(comma + 1, " %63s", op2);

        if (op1[0] != '\0')
            count++;
        if (op2[0] != '\0')
            count++;
    }

    return count;
}

Boolean is_matrix(const char *operand)
{
    return (operand != NULL && strchr(operand, '[') && strchr(operand, ']')) ? TRUE : FALSE;
}

Boolean is_immediate(const char *operand)
{
    return (operand != NULL && operand[0] == '#') ? TRUE : FALSE;
}

Boolean is_label_operand(const char *operand)
{
    return (operand != NULL &&
            !is_register(operand) &&
            !is_immediate(operand) &&
            !is_matrix(operand))
               ? TRUE
               : FALSE;
}

int count_string_length(const char *line)
{
    const char *p = strchr(line, '"');
    int count = 0;

    if (!p)
        return 0;
    p++;
    while (*p && *p != '"')
    {
        count++;
        p++;
    }

    return count + 1;
}

int count_command_words(const char *line)
{
    char command[MAX_LINE_LENGTH] = {0};
    char operands_part[MAX_LINE_LENGTH] = {0};
    char operand1[MAX_LINE_LENGTH] = {0}, operand2[MAX_LINE_LENGTH] = {0};
    int words = 1;
    int num_operands;

    if (sscanf(line, "%s %[^\n]", command, operands_part) < 1)
    {
        return 0;
    }

    if (strcmp(command, "stop") == 0 || strcmp(command, "rts") == 0)
    {
        return 1;
    }

    num_operands = parse_operands(operands_part, operand1, operand2);

    if (num_operands == 1)
    {
        if (is_register(operand1) || is_immediate(operand1) || is_label_operand(operand1))
        {
            words += 1;
        }
        else if (is_matrix(operand1))
        {
            words += 2;
        }
        else
        {
            words += 1;
        }
    }
    else if (num_operands == 2)
    {
        Boolean reg1 = is_register(operand1);
        Boolean reg2 = is_register(operand2);

        if (reg1 && reg2)
        {
            words += 1;
        }
        else
        {
            if (is_register(operand1) || is_immediate(operand1) || is_label_operand(operand1))
            {
                words += 1;
            }
            else if (is_matrix(operand1))
            {
                words += 2;
            }

            if (is_register(operand2) || is_immediate(operand2) || is_label_operand(operand2))
            {
                words += 1;
            }
            else if (is_matrix(operand2))
            {
                words += 2;
            }
        }
    }

    return words;
}

Boolean is_empty_line(const char *line)
{
    int i;
    if (!line)
        return TRUE;

    for (i = 0; line[i]; i++)
    {
        if (!isspace(line[i]))
        {
            return FALSE;
        }
    }
    return TRUE;
}

Boolean is_comment_line(const char *line)
{
    int i;
    if (!line)
        return FALSE;

    for (i = 0; line[i] && isspace(line[i]); i++)
        ;

    return (line[i] == ';') ? TRUE : FALSE;
}

char *trim_whitespace(char *str)
{
    char *end;

    if (!str)
        return str;

    while (isspace(*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;

    *(end + 1) = '\0';

    return str;
}
