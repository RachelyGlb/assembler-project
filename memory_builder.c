/* memory_builder.c - building the memory image */
#include "memory_builder.h"
#include "line_analysis.h"
#include "instruction_table.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* initialize memory image */
void init_memory_image(MemoryImage *memory)
{
    int i;
    if (!memory)
        return;

    for (i = 0; i < MEMORY_SIZE; i++)
    {
        memory->instruction_memory[i] = 0;
        memory->data_memory[i] = 0;
        memory->data_image[i].bits = 0;
    }

    memory->instruction_count = 0;
    memory->data_count = 0;
    memory->ICF = 0;
    memory->DCF = 0;
}

Boolean add_data_word(MemoryImage *memory, int address, MachineWord word)
{
    int idx;
    if (!memory)
        return FALSE;
    if (memory->data_count >= MEMORY_SIZE)
    {
        printf("Error: data image overflow at address %d\n", address);
        return FALSE;
    }
    idx = memory->data_count++;
    memory->data_image[idx] = word;
    return TRUE;
}

Boolean add_instruction_word(MemoryImage *memory, int address, MachineWord word)
{
    int idx;
    if (!memory)
        return FALSE;
    if (memory->instruction_count >= MEMORY_SIZE)
    {
        printf("Error: instruction image overflow at address %d\n", address);
        return FALSE;
    }
    idx = memory->instruction_count++;
    memory->instruction_memory[idx] = word.bits;
    return TRUE;
}

Boolean update_instruction_word(MemoryImage *memory, int address, MachineWord word)
{
    int idx;
    if (!memory)
        return FALSE;
    idx = address - BASE_ADDRESS;
    if (idx < 0 || idx >= memory->instruction_count)
    {
        printf("Error: update address %d out of range (valid: %d..%d)\n",
               address,
               BASE_ADDRESS,
               BASE_ADDRESS + memory->instruction_count - 1);
        return FALSE;
    }
    memory->instruction_memory[idx] = (word.bits & 0x3FF);
    return TRUE;
}

/* build memory image from .am file */
Boolean build_memory_image(const char *am_filename, MemoryImage *memory)
{
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    int current_ic = BASE_ADDRESS;
    int current_dc = 0;
    Boolean has_errors = FALSE;

    /* initialize memory image */
    init_memory_image(memory);

    file = fopen(am_filename, "r");
    if (!file)
    {
        printf("Error: Cannot open file %s for memory building\n", am_filename);
        return FALSE;
    }

    printf("Building memory image from: %s\n", am_filename);

    /* iterate over the file */
    while (fgets(line, sizeof(line), file))
    {
        line_number++;
        line[strcspn(line, "\n")] = '\0';

        if (is_comment_or_empty(line))
        {
            continue;
        }

        /* skip .entry and .extern */
        if (strstr(line, ".entry") || strstr(line, ".extern"))
        {
            continue;
        }

        /* process data directives */
        if (strstr(line, ".data"))
        {
            encode_data_directive(line, memory, &current_dc, line_number);
            continue;
        }

        if (strstr(line, ".string"))
        {
            encode_string_directive(line, memory, &current_dc, line_number);
            continue;
        }

        if (strstr(line, ".mat"))
        {
            encode_matrix_directive(line, memory, &current_dc, line_number);
            continue;
        }

        /* process instruction lines */
        if (is_command(line))
        {
            encode_instruction_first_pass(line, memory, &current_ic, line_number);
        }
    }

    memory->ICF = BASE_ADDRESS + memory->instruction_count;
    memory->DCF = memory->data_count;

    fclose(file);

    if (has_errors)
    {
        printf("Errors occurred while building memory image\n");
        return FALSE;
    }

    printf("Memory image built successfully. IC=%d, DC=%d\n", memory->ICF, memory->DCF);
    return TRUE;
}

/* processing the .data directive */
void encode_data_directive(const char *line, MemoryImage *memory, int *current_dc, int line_number)
{
    char line_copy[MAX_LINE_LENGTH];
    char *token;
    int value;
    MachineWord data_word;

    strcpy(line_copy, line);

    /* skip label if present */
    token = strtok(line_copy, " \t");
    if (token && strchr(token, ':'))
    {
        token = strtok(NULL, " \t");
    }

    /* skip the ".data" token */
    if (token && strcmp(token, ".data") == 0)
    {
        token = strtok(NULL, " \t");
    }

    /* process comma-separated values */
    if (token)
    {
        token = strtok(token, ",");
        while (token && memory->data_count < MEMORY_SIZE)
        {
            while (isspace(*token))
                token++;
            value = atoi(token);

            /* handle negative numbers - two's complement to 10 bits */
            if (value < 0)
            {
                value = (1 << 10) + value;
            }

            data_word.bits = value & 0x3FF; /* mask to 10 bits */
            add_data_word(memory, BASE_ADDRESS + memory->instruction_count + *current_dc, data_word);
            (*current_dc)++;

            token = strtok(NULL, ",");
        }
    }
}

/* processing the .string directive */
void encode_string_directive(const char *line, MemoryImage *memory, int *current_dc, int line_number)
{
    char *start, *end;
    int i;
    MachineWord char_word;

    /* find string content between quotes */
    start = strchr(line, '"');
    if (!start)
        return;
    start++; /* skip opening quote */

    end = strrchr(line, '"');
    if (!end || end <= start)
        return;

    /* encode each character */
    for (i = 0; start + i < end && memory->data_count < MEMORY_SIZE; i++)
    {
        char_word.bits = (unsigned char)start[i];
        add_data_word(memory, BASE_ADDRESS + memory->instruction_count + *current_dc, char_word);
        (*current_dc)++;
    }

    /* append string terminator */
    if (memory->data_count < MEMORY_SIZE)
    {
        char_word.bits = 0;
        add_data_word(memory, BASE_ADDRESS + memory->instruction_count + *current_dc, char_word);
        (*current_dc)++;
    }
}

/* processing the .mat directive */
void encode_matrix_directive(const char *line, MemoryImage *memory, int *current_dc, int line_number)
{
    char line_copy[MAX_LINE_LENGTH];
    char *token;
    int value;
    MachineWord data_word;

    strcpy(line_copy, line);

    /* skip label if present */
    token = strtok(line_copy, " \t");
    if (token && strchr(token, ':'))
    {
        token = strtok(NULL, " \t");
    }

    /* skip the ".mat" token and dimensions */
    token = strtok(NULL, " \t");
    while (token && strchr(token, '['))
    {
        token = strtok(NULL, " \t");
    }

    /* process comma-separated values */
    if (token)
    {
        token = strtok(token, ",");
        while (token && memory->data_count < MEMORY_SIZE)
        {
            while (isspace(*token))
                token++;
            value = atoi(token);

            if (value < 0)
            {
                value = (1 << 10) + value;
            }

            data_word.bits = value & 0x3FF;
            add_data_word(memory, BASE_ADDRESS + memory->instruction_count + *current_dc, data_word);
            (*current_dc)++;

            token = strtok(NULL, ",");
        }
    }
}

/* encode instruction for the first pass */
/* encode instruction for the first pass */
void encode_instruction_first_pass(const char *line, MemoryImage *memory, int *current_ic, int line_number)
{
    char line_copy[MAX_LINE_LENGTH];
    char *token;
    char *instruction_name;
    char operand1[MAX_LINE_LENGTH] = "";
    char operand2[MAX_LINE_LENGTH] = "";
    char *end;
    int operand_count = 0;
    InstructionDef *instr;
    int opcode;
    int src_method = 0, dest_method = 0;
    MachineWord first_word, w;

    (void)line_number; /* unused here; keep for interface compatibility */

    strcpy(line_copy, line);

    /* skip label if present */
    token = strtok(line_copy, " \t");
    if (token && strchr(token, ':'))
    {
        token = strtok(NULL, " \t");
    }

    instruction_name = token;
    if (!instruction_name)
        return;

    /* locate instruction */
    instr = find_instruction(instruction_name);
    if (!instr)
        return;

    opcode = get_opcode_value(instruction_name);

    /* parse operands (trim leading and trailing whitespace) */
    token = strtok(NULL, ",");
    if (token)
    {
        while (isspace((unsigned char)*token))
            token++;
        strcpy(operand1, token);
        end = operand1 + strlen(operand1) - 1;
        while (end >= operand1 && isspace((unsigned char)*end))
        {
            *end-- = '\0';
        }
        operand_count++;

        token = strtok(NULL, ",");
        if (token)
        {
            while (isspace((unsigned char)*token))
                token++;
            strcpy(operand2, token);
            end = operand2 + strlen(operand2) - 1;
            while (end >= operand2 && isspace((unsigned char)*end))
            {
                *end-- = '\0';
            }
            operand_count++;
        }
    }

    /* addressing methods for first word */
    if (operand_count >= 1)
    {
        src_method = get_addressing_method(operand1);
    }
    if (operand_count >= 2)
    {
        dest_method = get_addressing_method(operand2);
    }

    /* emit first (opcode) word */
    first_word = encode_first_word(opcode, src_method, dest_method);
    add_instruction_word(memory, *current_ic, first_word);
    (*current_ic)++;

    /* --- emit additional operand words --- */

    /* special case: two registers share one word */
    if (operand_count == 2 && is_register(operand1) && is_register(operand2))
    {
        int src_reg = atoi(operand1 + 1); /* 'rX' -> X */
        int dest_reg = atoi(operand2 + 1);
        w = encode_register_operand(src_reg, dest_reg);
        add_instruction_word(memory, *current_ic, w);
        (*current_ic)++;
        return;
    }

    /* source operand (if exists) */
    if (operand_count >= 1)
    {
        if (operand1[0] == '#')
        {
            /* immediate */
            w = encode_immediate_operand(atoi(operand1 + 1));
            add_instruction_word(memory, *current_ic, w);
            (*current_ic)++;
        }
        else if (is_register(operand1))
        {
            /* single register (source-only field) */
            int reg = atoi(operand1 + 1);
            w = encode_register_operand(reg, 0);
            add_instruction_word(memory, *current_ic, w);
            (*current_ic)++;
        }
        else
        {
            /* direct/matrix symbol -> placeholder to be patched in second pass */
            w.bits = 0;
            add_instruction_word(memory, *current_ic, w);
            (*current_ic)++;
        }
    }

    /* destination operand (if exists) */
    if (operand_count >= 2)
    {
        if (operand2[0] == '#')
        {
            /* immediate */
            w = encode_immediate_operand(atoi(operand2 + 1));
            add_instruction_word(memory, *current_ic, w);
            (*current_ic)++;
        }
        else if (is_register(operand2))
        {
            /* single register (dest-only field) */
            int reg = atoi(operand2 + 1);
            w = encode_register_operand(0, reg);
            add_instruction_word(memory, *current_ic, w);
            (*current_ic)++;
        }
        else
        {
            /* direct/matrix symbol -> placeholder to be patched in second pass */
            w.bits = 0;
            add_instruction_word(memory, *current_ic, w);
            (*current_ic)++;
        }
    }
}

/* get opcode value */
int get_opcode_value(const char *instruction_name)
{
    if (strcmp(instruction_name, "mov") == 0)
        return 0;
    if (strcmp(instruction_name, "cmp") == 0)
        return 1;
    if (strcmp(instruction_name, "add") == 0)
        return 2;
    if (strcmp(instruction_name, "sub") == 0)
        return 3;
    if (strcmp(instruction_name, "not") == 0)
        return 4;
    if (strcmp(instruction_name, "clr") == 0)
        return 5;
    if (strcmp(instruction_name, "lea") == 0)
        return 6;
    if (strcmp(instruction_name, "inc") == 0)
        return 7;
    if (strcmp(instruction_name, "dec") == 0)
        return 8;
    if (strcmp(instruction_name, "jmp") == 0)
        return 9;
    if (strcmp(instruction_name, "bne") == 0)
        return 10;
    if (strcmp(instruction_name, "red") == 0)
        return 11;
    if (strcmp(instruction_name, "prn") == 0)
        return 12;
    if (strcmp(instruction_name, "jsr") == 0)
        return 13;
    if (strcmp(instruction_name, "rts") == 0)
        return 14;
    if (strcmp(instruction_name, "stop") == 0)
        return 15;
    return -1; /* unknown instruction */
}

/* get addressing method */
int get_addressing_method(const char *operand)
{
    if (!operand)
        return -1;

    if (operand[0] == '#')
    {
        return IMMEDIATE_ADDR; /* 0 - immediate */
    }

    if (operand[0] == 'r' && isdigit(operand[1]))
    {
        return REGISTER_ADDR; /* 3 - register */
    }

    if (operand[0] == '[' && operand[strlen(operand) - 1] == ']')
    {
        return MATRIX_ADDR; /* 2 - matrix */
    }

    return DIRECT_ADDR; /* 1 - direct */
}

/* encode first word */
MachineWord encode_first_word(int opcode, int src_method, int dest_method)
{
    MachineWord word;
    word.bits = 0;

    /* bits 6-9: opcode */
    word.bits |= (opcode & 0xF) << 6;

    /* bits 4-5: source addressing method */
    word.bits |= (src_method & 0x3) << 4;

    /* bits 2-3: destination addressing method */
    word.bits |= (dest_method & 0x3) << 2;

    /* bits 0-1: A,R,E - default value 0 */

    return word;
}
/* encode immediate operand */
MachineWord encode_immediate_operand(int value)
{
    MachineWord word;

    /* handle negative numbers */
    if (value < 0)
    {
        value = (1 << 10) + value; /* two's complement to 10 bits */
    }

    word.bits = value & 0x3FF; /* 10-bit mask */
    return word;
}

/* encode register operand */
MachineWord encode_register_operand(int src_reg, int dest_reg)
{
    MachineWord word;
    word.bits = 0;

    /* source register number in bits 6-9 */
    word.bits |= (src_reg & 0xF) << 6;

    /* destination register number in bits 2-5 */
    word.bits |= (dest_reg & 0xF) << 2;

    return word;
}

void free_memory_image(MemoryImage *memory)
{
    if (!memory)
        return;
    memory->instruction_count = 0;
    memory->data_count = 0;
    memory->ICF = BASE_ADDRESS;
    memory->DCF = 0;
}
