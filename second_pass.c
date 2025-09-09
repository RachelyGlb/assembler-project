/**
 * @file second_pass.c
 * @brief Second pass of the assembler - code generation and address resolution
 * 
 * This module implements the second pass of the two-pass assembler.
 * It generates the final machine code, resolves symbol addresses,
 * and creates the necessary output files.
 */

#include "second_pass.h"
#include "line_analysis.h"
#include "instruction_table.h"
#include "output_writer.h"
#include "memory_builder.h"

/**
 * @brief Main second pass function - generates machine code and resolves addresses
 * @param filename Name of the .am file to process
 * @param memory Pointer to the memory image structure
 * @return TRUE if second pass completed successfully, FALSE on error
 * 
 * The second pass performs the following operations:
 * 1. Reads the preprocessed .am file line by line
 * 2. Processes .entry directives for symbol exports
 * 3. Generates machine code for instructions
 * 4. Resolves symbol addresses and external references
 * 5. Updates the memory image with final code
 * 6. Prepares data for output file generation
 */
/* Main second pass function */
Boolean second_pass(const char *filename, MemoryImage *memory)
{
    FILE *file;
    char line[MAX_LINE_LENGTH];
    SecondPassContext ctx;
    int line_number = 0;

    /* Initialize context */
    ctx.memory = memory;
    ctx.ext_list = NULL;
    ctx.entry_list = NULL;
    ctx.has_errors = FALSE;
    ctx.current_ic = BASE_ADDRESS;

    /* Open .am file */
    file = fopen(filename, "r");
    if (!file)
    {
        printf("Error: Cannot open file %s for second pass\n", filename);
        return FALSE;
    }

    printf("Starting second pass for file: %s\n", filename);

    /* Process each line */
    while (fgets(line, sizeof(line), file))
    {
        line_number++;

        /* Remove newline */
        line[strcspn(line, "\n")] = '\0';

        /* Skip empty lines and comments */
        if (is_comment_or_empty(line))
        {
            continue;
        }

        /* Check if line starts with .entry */
        if (strstr(line, ".entry"))
        {
            process_entry_directive(line, &ctx, line_number);
            continue;
        }

        /* Skip other directives (.data, .string, .mat, .extern) */
        if (line[0] == '.' || strstr(line, ".data") || strstr(line, ".string") ||
            strstr(line, ".mat") || strstr(line, ".extern"))
        {
            continue;
        }

        /* Process instruction lines */
        /* Process instruction lines */
if (is_command(line))
{
    process_instruction_second_pass(line, &ctx, line_number);
}
else
{
    printf("SECOND_PASS DEBUG: Skipping non-instruction line: '%s'\n", line);
}
    }

    fclose(file);

    if (ctx.has_errors)
    {
        printf("Errors found in second pass. Output files will not be generated.\n");
        free_ext_list(ctx.ext_list);
        free_entry_list(ctx.entry_list);
        return FALSE;
    }

    /* Generate output files */
    generate_output_files(filename, memory, ctx.ext_list, ctx.entry_list);

    /* Cleanup */
    free_ext_list(ctx.ext_list);
    free_entry_list(ctx.entry_list);

    printf("Second pass completed successfully.\n");
    return TRUE;
}

/* Process instruction line in second pass */
void process_instruction_second_pass(const char *line, SecondPassContext *ctx, int line_number)
{
    char line_copy[MAX_LINE_LENGTH];
    char *token,*end;
    char *instruction_name;
    char operand1[MAX_LINE_LENGTH] = "";
    char operand2[MAX_LINE_LENGTH] = "";
    int operand_count = 0;
    InstructionDef *instr;
    MachineWord word;
    int are_bits;
    int addr1, addr2;
    int operand1_words = 0;
    int instruction_length;
    
    strcpy(line_copy, line);

    /* Skip label if exists */
    token = strtok(line_copy, " \t");
    if (token && strchr(token, ':'))
    {
        token = strtok(NULL, " \t");
    }

    instruction_name = token;
    if (!instruction_name)
    {
        printf("Error line %d: Missing instruction name\n", line_number);
        ctx->has_errors = TRUE;
        return;
    }

    /* Find instruction in table */
    instr = find_instruction(instruction_name);
    if (!instr)
    {
        printf("Error line %d: Unknown instruction '%s'\n", line_number, instruction_name);
        ctx->has_errors = TRUE;
        return;
    }

    /* Parse operands FIRST */
    token = strtok(NULL, ",");
    if (token)
    {
        /* Remove leading/trailing whitespace */
        while (isspace(*token))
            token++;
        strcpy(operand1, token);
        /* Remove trailing whitespace */
        end = operand1 + strlen(operand1) - 1;
        while (end > operand1 && isspace(*end))
            end--;
        *(end + 1) = '\0';
        operand_count++;

        token = strtok(NULL, ",");
        if (token)
        {
            while (isspace(*token))
                token++;
            strcpy(operand2, token);
            end = operand2 + strlen(operand2) - 1;
            while (end > operand2 && isspace(*end))
                end--;
            *(end + 1) = '\0';
            operand_count++;
        }
    }

    
    /* NOW calculate addresses AFTER parsing - handle matrix properly */
if (operand_count >= 1)
{
    if (operand1[0] == '#')
        operand1_words = 1;          /* immediate */
    else if (is_register(operand1))
        operand1_words = 0;          /* register goes in shared word */
    else if (is_matrix(operand1))
        operand1_words = 2;          /* matrix needs 2 words */
    else
        operand1_words = 1;          /* direct addressing */
}

addr1 = ctx->current_ic + 1;
addr2 = ctx->current_ic + 1 + operand1_words;
    /* Process operands that reference symbols */
    /* operand 1 */
    /* operand 1 */
if (operand_count >= 1 && operand1[0] != '#' && !is_register(operand1))
{
    if (encode_operand(operand1, &word, &are_bits, ctx, line_number, addr1))
    {
    
        update_instruction_word(ctx->memory, addr1, word);
    }
    else
    {
        ctx->has_errors = TRUE;
    }
}

    /* operand 2 */
    if (operand_count >= 2 && operand2[0] != '#' && !is_register(operand2))
{
    if (encode_operand(operand2, &word, &are_bits, ctx, line_number, addr2))
    {
        update_instruction_word(ctx->memory, addr2, word);
    }
    else
    {
        ctx->has_errors = TRUE;
    }
}
/* Update IC by instruction length */
    instruction_length = get_instruction_length(instruction_name, operand1, operand2);
    ctx->current_ic += instruction_length;

}
/* Process .entry directive */
void process_entry_directive(const char *line, SecondPassContext *ctx, int line_number)
{
    char line_copy[MAX_LINE_LENGTH];
    char *token;
    Symbol *symbol;

    strcpy(line_copy, line);

    /* Skip ".entry" */
    token = strtok(line_copy, " \t");
    token = strtok(NULL, " \t");

    if (!token)
    {
        printf("Error line %d: Missing symbol name in .entry directive\n", line_number);
        ctx->has_errors = TRUE;
        return;
    }

    /* Find symbol in symbol table */
    symbol = find_symbol(token);
    if (!symbol)
    {
        printf("Error line %d: Symbol '%s' not defined for .entry\n", line_number, token);
        ctx->has_errors = TRUE;
        return;
    }

    if (symbol->type == EXTERN_SYM)
    {
        printf("Error line %d: Cannot declare external symbol '%s' as entry\n", line_number, token);
        ctx->has_errors = TRUE;
        return;
    }

    /* Mark as entry and add to entry list */
    symbol->is_entry = 1;
    add_entry_point(ctx, token, symbol->address);
}

/* Encode operand that references a symbol */
Boolean encode_operand(const char *operand, MachineWord *word, int *are_bits,
                       SecondPassContext *ctx, int line_number, int target_address)
{
    Symbol *symbol;
    

    
    char symbol_name[MAX_LABEL_LENGTH];
    /* Extract symbol name (handle matrix addressing) */
    if (strchr(operand, '['))
    {
        /* Matrix addressing - extract base name */
        int i = 0;
        while (operand[i] && operand[i] != '[' && i < MAX_LABEL_LENGTH - 1)
        {
            symbol_name[i] = operand[i];
            i++;
        }
        symbol_name[i] = '\0';
    }
    else
    {
        /* Direct addressing */
        strncpy(symbol_name, operand, MAX_LABEL_LENGTH - 1);
        symbol_name[MAX_LABEL_LENGTH - 1] = '\0';
    }

    /* Find symbol in table */
    symbol = find_symbol(symbol_name);
    if (!symbol)
    {
        printf("Error line %d: Undefined symbol '%s'\n", line_number, symbol_name);
        ctx->has_errors = TRUE;
        return FALSE;
    }

    /* Set ARE bits and address based on symbol type - רק פעם אחת! */
    if (symbol->type == EXTERN_SYM)
    {
        *are_bits = ARE_EXTERNAL;
        word->bits = ARE_EXTERNAL;  
        add_external_reference(ctx, symbol_name, target_address);
    }
    else
    {
        *are_bits = ARE_RELOCATABLE;
        word->bits = ((symbol->address & 0xFF) << 2) | ARE_RELOCATABLE;
    }
    
    return TRUE;
}

/* Get instruction length in words */
int get_instruction_length(const char *instruction, const char *operand1, const char *operand2)
{
    int length = 1; /* always the first opcode word */
    int has_op1 = (operand1 && operand1[0] != '\0');
    int has_op2 = (operand2 && operand2[0] != '\0');

    int op1_is_reg = has_op1 && is_register(operand1);
    int op2_is_reg = has_op2 && is_register(operand2);
    int op1_is_matrix = has_op1 && is_matrix(operand1);
    int op2_is_matrix = has_op2 && is_matrix(operand2);

    /* add words for NON-register operands */
    if (has_op1 && !op1_is_reg)
    {
        if (operand1[0] == '#')
            length++;  /* immediate = 1 word */
        else if (op1_is_matrix)
            length += 2;  /* matrix = 2 words */
        else
            length++;  /* direct = 1 word */
    }

    if (has_op2 && !op2_is_reg) 
    {
        if (operand2[0] == '#')
            length++;  /* immediate = 1 word */
        else if (op2_is_matrix)
            length += 2;  /* matrix = 2 words */
        else
            length++;  /* direct = 1 word */
    }

    /* registers: add ONE word if there are ANY register operands */
    if (op1_is_reg || op2_is_reg)
    {
        length += 1;
    }

    return length;
}
/* Add external reference to list */
void add_external_reference(SecondPassContext *ctx, const char *symbol_name, int address)
{
    ExtRef *new_ref = malloc(sizeof(ExtRef));
    if (!new_ref)
    {
        printf("Error: Memory allocation failed for external reference\n");
        ctx->has_errors = TRUE;
        return;
    }

    strncpy(new_ref->symbol_name, symbol_name, MAX_LABEL_LENGTH - 1);
    new_ref->symbol_name[MAX_LABEL_LENGTH - 1] = '\0';
    new_ref->address = address;
    new_ref->next = ctx->ext_list;
    ctx->ext_list = new_ref;
}

/* Add entry point to list */
void add_entry_point(SecondPassContext *ctx, const char *symbol_name, int address)
{
    EntryPoint *new_entry = malloc(sizeof(EntryPoint));
    if (!new_entry)
    {
        printf("Error: Memory allocation failed for entry point\n");
        ctx->has_errors = TRUE;
        return;
    }

    strncpy(new_entry->symbol_name, symbol_name, MAX_LABEL_LENGTH - 1);
    new_entry->symbol_name[MAX_LABEL_LENGTH - 1] = '\0';
    new_entry->address = address;
    new_entry->next = ctx->entry_list;
    ctx->entry_list = new_entry;
}

/* Convert decimal to base 4 unique representation */
char *decimal_to_base4(int decimal, int digits)
{
    char *result = malloc(digits + 1);
    int i;
    char base4_chars[] = {'a', 'b', 'c', 'd'};
    unsigned int value;

    if (!result)
        return NULL;
    result[digits] = '\0';

    /* Handle negative numbers using two's complement within 10 bits */
    if (decimal < 0)
    {
        value = (1 << 10) + decimal; /* Convert to positive using 10-bit two's complement */
    }
    else
    {
        value = decimal;
    }

    /* Ensure value fits in 10 bits (0-1023) */
    value &= 0x3FF;
    /* Convert to base 4 from right to left */
    for (i = digits - 1; i >= 0; i--)
    {
        result[i] = base4_chars[value % 4];
        value /= 4;
    }
    return result;
}

/* Free external references list */
void free_ext_list(ExtRef *head)
{
    ExtRef *current = head;
    while (current)
    {
        ExtRef *next = current->next;
        free(current);
        current = next;
    }
}

/* Free entry points list */
void free_entry_list(EntryPoint *head)
{
    EntryPoint *current = head;
    while (current)
    {
        EntryPoint *next = current->next;
        free(current);
        current = next;
    }
}