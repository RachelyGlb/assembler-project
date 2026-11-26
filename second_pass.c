/*
 * second_pass.c - the second pass over the expanded source file.
 *
 * The pass parses every line again with the syntax module, which reports
 * nothing now because the first pass found the file free of syntax errors,
 * and follows the instruction counter with the same word counts that the
 * first pass used. See second_pass.h for the contract.
 */
#include "second_pass.h"

#include <stdlib.h>

#include "encoder.h"
#include "syntax.h"
#include "util.h"

/* The address bits of a word that refers to an external symbol (ENC-8). */
#define EXTERNAL_ADDRESS 0

/* Everything the second pass of one source file needs. */
typedef struct
{
    SymbolTable *symbols;     /* The symbols of the file. */
    MemoryImage *image;       /* The memory image being completed. */
    ReferenceList *externals; /* The uses of external symbols. */
    ErrorContext *errors;     /* The diagnostics context of the file. */
    int line_number;          /* The .as line of the line being handled. */
    int address;              /* The address of the current instruction. */
    Boolean no_memory;        /* TRUE once an allocation failed. */
} SecondPass;

/*
 * Handles .entry: gives the symbol the attribute "entry" (DIR-19). The
 * symbol must be defined in the file (ERR-51) and must not be external
 * (DIR-23, ERR-52).
 *   pass       the state of the pass
 *   statement  a valid .entry statement
 */
static void mark_entry(SecondPass *pass, const Statement *statement)
{
    const char *name = statement_directive_symbol(statement);
    const Symbol *symbol = symbol_table_find(pass->symbols, name);

    if (symbol == NULL)
    {
        errors_report(pass->errors, pass->line_number,
                      ".entry names the symbol '%s', which is not defined "
                      "in this file",
                      name);
        return;
    }
    if (symbol_kind(symbol) == SYMBOL_EXTERNAL)
    {
        errors_report(pass->errors, pass->line_number,
                      "the symbol '%s' is declared both .entry and .extern",
                      name);
        return;
    }
    symbol_table_mark_entry(pass->symbols, name);
}

/*
 * Completes the word that holds the address of a symbol (ADR-5): a symbol
 * of the file gives its address and R, an external symbol gives zero and E
 * and is recorded as a reference (ENC-8, OUT-9).
 *   pass     the state of the pass
 *   name     the name of the symbol
 *   address  the address of the word to complete
 * Returns FALSE when the symbol is not in the symbol table (LBL-13,
 * ERR-50), which is reported, or when memory is exhausted.
 */
static Boolean resolve_symbol(SecondPass *pass, const char *name, int address)
{
    const Symbol *symbol = symbol_table_find(pass->symbols, name);

    if (symbol == NULL)
    {
        errors_report(pass->errors, pass->line_number,
                      "the symbol '%s' is neither defined nor declared "
                      ".extern",
                      name);
        return FALSE;
    }
    if (symbol_kind(symbol) != SYMBOL_EXTERNAL)
    {
        memory_image_merge_code(
            pass->image, address,
            encode_address(symbol_address(symbol), ARE_RELOCATABLE));
        return TRUE;
    }
    memory_image_merge_code(pass->image, address,
                            encode_address(EXTERNAL_ADDRESS, ARE_EXTERNAL));
    if (!reference_list_append(pass->externals, name, address))
    {
        pass->no_memory = TRUE;
        return FALSE;
    }
    return TRUE;
}

/*
 * Completes the symbol words of one instruction (ALG-10 step 6). The first
 * symbol that is not found ends the work on the line (ERR-R16).
 *   pass       the state of the pass; its address is that of the first
 *              word of the instruction
 *   statement  a valid instruction statement
 */
static void resolve_instruction(SecondPass *pass, const Statement *statement)
{
    const char *name;
    int index;

    for (index = 0; index < statement_operand_count(statement); index++)
    {
        /* Only a direct operand and a matrix operand name a symbol. */
        name = statement_operand_symbol(statement, index);
        if (name != NULL &&
            !resolve_symbol(pass, name,
                            pass->address + statement_operand_word_offset(
                                                statement, index)))
        {
            return;
        }
    }
}

/*
 * Handles one line of the .am file.
 *   pass  the state of the pass
 *   line  the text of the line
 */
static void handle_line(SecondPass *pass, const char *line)
{
    Statement *statement =
        statement_parse(line, pass->line_number, pass->errors);

    if (statement == NULL)
    {
        pass->no_memory = TRUE;
        return;
    }
    if (statement_is_valid(statement))
    {
        if (statement_kind(statement) == STATEMENT_ENTRY)
        {
            mark_entry(pass, statement);
        }
        else if (statement_kind(statement) == STATEMENT_INSTRUCTION)
        {
            resolve_instruction(pass, statement);
            pass->address += statement_word_count(statement);
        }
    }
    statement_destroy(statement);
}

/*
 * Reports that the .am file could not be read back (CLI-12, ERR-47). The
 * message has no line number, like the one for a file that does not open.
 *   errors  the diagnostics context of the file
 */
static void report_unreadable(ErrorContext *errors)
{
    errors_report(errors, NO_LINE, "cannot read the %s file",
                  EXPANDED_EXTENSION);
}

/* Runs the second pass on one expanded source file; see second_pass.h. */
StageResult second_pass(FILE *am_file, const LineMap *lines,
                        SymbolTable *symbols, MemoryImage *image,
                        ReferenceList *externals, ErrorContext *errors)
{
    SecondPass pass;
    char *line = NULL;
    size_t capacity = 0;
    int am_line = FIRST_LINE_NUMBER;
    int errors_before = errors_count(errors);
    ReadStatus status;

    pass.symbols = symbols;
    pass.image = image;
    pass.externals = externals;
    pass.errors = errors;
    pass.address = FIRST_LOAD_ADDRESS;
    pass.no_memory = FALSE;
    status = util_read_line(am_file, &line, &capacity, NULL, FALSE);
    while (status == READ_LINE && !pass.no_memory)
    {
        pass.line_number = line_map_source_line(lines, am_line++);
        handle_line(&pass, line);
        status = util_read_line(am_file, &line, &capacity, NULL, FALSE);
    }
    free(line);
    if (status == READ_NO_MEMORY || pass.no_memory)
    {
        return STAGE_FATAL;
    }
    if (status == READ_ERROR)
    {
        report_unreadable(errors);
    }
    return (errors_count(errors) > errors_before) ? STAGE_ERRORS : STAGE_OK;
}
