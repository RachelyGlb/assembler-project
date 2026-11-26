/*
 * first_pass.c - the first pass over the expanded source file.
 *
 * The am_reader module walks over the file and parses every line into a
 * statement, which is handled here by the function of its kind. A line
 * that has any error adds no word to the memory image; its label, when it
 * is legal and new, is still defined (LBL-23). See first_pass.h for the
 * contract.
 */
#include "first_pass.h"

#include "am_reader.h"
#include "encoder.h"
#include "instruction_table.h"
#include "syntax.h"

/* The value of an external symbol in the symbol table (DIR-21). */
#define EXTERNAL_SYMBOL_VALUE 0

/* The half of a register word that a single register does not use. */
#define UNUSED_REGISTER_FIELD 0

/* A code word before any of its bits is known. */
#define EMPTY_WORD 0u

/*
 * The position of the index word among the two additional words of a
 * matrix operand (ADR-7).
 */
#define MATRIX_INDEX_WORD 1

/* Everything the first pass of one source file needs. */
typedef struct
{
    const MacroTable *macros;  /* The macros of the file. */
    SymbolTable *symbols;      /* The symbol table being filled. */
    MemoryImage *image;        /* The memory image being filled. */
    ErrorContext *errors;      /* The diagnostics context of the file. */
    int line_number;           /* The .as line of the line being handled. */
    Boolean overflow_reported; /* TRUE once ERR-41 was reported (ALG-17). */
    Boolean no_memory;         /* TRUE once an allocation failed. */
} FirstPass;

/*
 * Tells whether a statement stores data, so that its label is a data label
 * (DIR-8, DIR-11, DIR-16).
 *   statement  the statement to ask
 * Returns TRUE for .data, .string and .mat.
 */
static Boolean is_data_statement(const Statement *statement)
{
    StatementKind kind = statement_kind(statement);

    return (kind == STATEMENT_DATA || kind == STATEMENT_STRING ||
            kind == STATEMENT_MAT)
               ? TRUE
               : FALSE;
}

/*
 * Adds a symbol to the symbol table and notes an allocation failure.
 *   pass     the state of the pass
 *   name     the name of the symbol
 *   address  the value of the symbol
 *   kind     the kind of the symbol
 * Returns FALSE when memory is exhausted.
 */
static Boolean add_symbol(FirstPass *pass, const char *name, int address,
                          SymbolKind kind)
{
    if (!symbol_table_add(pass->symbols, name, address, kind))
    {
        pass->no_memory = TRUE;
        return FALSE;
    }
    return TRUE;
}

/*
 * Tells why a name cannot become a new label and, if asked, reports it:
 * the name of a macro (LBL-21, ERR-14), a symbol declared .extern (DIR-36,
 * ERR-53) or a label defined before (LBL-5, ERR-15).
 *   pass    the state of the pass
 *   label   the name of the label
 *   report  TRUE to report the reason; FALSE on a line that already has
 *           its one message (ERR-R16)
 * Returns TRUE when the name is free.
 */
static Boolean label_is_free(FirstPass *pass, const char *label,
                             Boolean report)
{
    const Symbol *symbol = symbol_table_find(pass->symbols, label);
    const char *reason = NULL;

    if (macro_table_find(pass->macros, label) != NULL)
    {
        reason = "the label '%s' has the name of a macro";
    }
    else if (symbol != NULL && symbol_kind(symbol) == SYMBOL_EXTERNAL)
    {
        reason = "the label '%s' is declared .extern in this file";
    }
    else if (symbol != NULL)
    {
        reason = "the label '%s' is already defined";
    }
    if (reason != NULL && report)
    {
        errors_report(pass->errors, pass->line_number, reason, label);
    }
    return (reason == NULL) ? TRUE : FALSE;
}

/*
 * Defines the label of a statement, if it has one: a data label gets DC, a
 * code label gets IC (LBL-9).
 *   pass       the state of the pass
 *   statement  the statement
 *   report     TRUE to report a label that cannot be defined
 * Returns FALSE when the label could not be defined.
 */
static Boolean define_label(FirstPass *pass, const Statement *statement,
                            Boolean report)
{
    const char *label = statement_label(statement);

    if (label == NULL)
    {
        return TRUE;
    }
    if (!label_is_free(pass, label, report))
    {
        return FALSE;
    }
    if (is_data_statement(statement))
    {
        return add_symbol(pass, label,
                          memory_image_data_counter(pass->image),
                          SYMBOL_DATA);
    }
    return add_symbol(pass, label,
                      memory_image_instruction_counter(pass->image),
                      SYMBOL_CODE);
}

/*
 * Checks that the words of a line still fit in the memory of the machine
 * and reports the overflow the first time they do not (ALG-15, ALG-17,
 * ERR-41).
 *   pass   the state of the pass
 *   words  the number of words that the line adds
 * Returns FALSE when the words do not fit; nothing must then be stored.
 */
static Boolean has_room(FirstPass *pass, int words)
{
    if (words <= memory_image_free_words(pass->image))
    {
        return TRUE;
    }
    if (!pass->overflow_reported)
    {
        errors_report(pass->errors, pass->line_number,
                      "the program does not fit in the %d words of memory "
                      "that follow address %d",
                      MAX_PROGRAM_WORDS, FIRST_LOAD_ADDRESS);
        pass->overflow_reported = TRUE;
    }
    return FALSE;
}

/*
 * Stores the words of a .data, .string or .mat directive in the data image
 * (DIR-7, DIR-10, DIR-13).
 *   pass       the state of the pass
 *   statement  a valid statement of one of these kinds
 * Returns FALSE when the words do not fit in memory.
 */
static Boolean store_data(FirstPass *pass, const Statement *statement)
{
    int count = statement_data_count(statement);
    int index;

    if (!has_room(pass, count))
    {
        return FALSE;
    }
    for (index = 0; index < count; index++)
    {
        memory_image_add_data(
            pass->image, encode_data(statement_data_value(statement, index)));
    }
    return TRUE;
}

/*
 * Builds the first word of an instruction from its operation and the
 * addressing modes of its operands (ENC-3). A mode field without an operand
 * is zero (ENC-9, ENC-10).
 *   statement  a valid instruction statement
 * Returns the word.
 */
static unsigned int first_word(const Statement *statement)
{
    AddressingMode source = ADDRESSING_IMMEDIATE;
    AddressingMode destination = ADDRESSING_IMMEDIATE;
    int index;

    for (index = 0; index < statement_operand_count(statement); index++)
    {
        if (statement_operand_is_source(statement, index))
        {
            source = statement_operand_mode(statement, index);
        }
        else
        {
            destination = statement_operand_mode(statement, index);
        }
    }
    return encode_first_word(
        instruction_opcode(statement_instruction(statement)), source,
        destination);
}

/*
 * Stores the bits of the additional words of one operand that do not depend
 * on a symbol: an immediate value, a register, the index registers of a
 * matrix (ALG-7). The word of a label stays zero for the second pass.
 *   pass       the state of the pass
 *   statement  a valid instruction statement
 *   index      the position of the operand
 *   base       the address of the first word of the instruction
 */
static void store_operand(FirstPass *pass, const Statement *statement,
                          int index, int base)
{
    int address = base + statement_operand_word_offset(statement, index);
    int value = statement_operand_value(statement, index);
    AddressingMode mode = statement_operand_mode(statement, index);
    unsigned int word = EMPTY_WORD;

    if (mode == ADDRESSING_IMMEDIATE)
    {
        word = encode_immediate(value);
    }
    else if (mode == ADDRESSING_REGISTER)
    {
        /* Two registers merge into the word they share (ADR-11). */
        word = statement_operand_is_source(statement, index)
                   ? encode_registers(value, UNUSED_REGISTER_FIELD)
                   : encode_registers(UNUSED_REGISTER_FIELD, value);
    }
    else if (mode == ADDRESSING_MATRIX)
    {
        address += MATRIX_INDEX_WORD;
        word = encode_matrix_indices(
            statement_operand_matrix_register(statement, index, MATRIX_ROW),
            statement_operand_matrix_register(statement, index,
                                              MATRIX_COLUMN));
    }
    memory_image_merge_code(pass->image, address, word);
}

/*
 * Stores an instruction in the code image: as many words as the statement
 * takes, with the first word and every operand bit that is known (ALG-9
 * steps 13 to 16).
 *   pass       the state of the pass
 *   statement  a valid instruction statement
 * Returns FALSE when the words do not fit in memory.
 */
static Boolean store_instruction(FirstPass *pass, const Statement *statement)
{
    int base = memory_image_instruction_counter(pass->image);
    int words = statement_word_count(statement);
    int index;

    if (!has_room(pass, words))
    {
        return FALSE;
    }
    for (index = 0; index < words; index++)
    {
        memory_image_add_code(pass->image, EMPTY_WORD);
    }
    memory_image_merge_code(pass->image, base, first_word(statement));
    for (index = 0; index < statement_operand_count(statement); index++)
    {
        store_operand(pass, statement, index, base);
    }
    return TRUE;
}

/*
 * Handles .extern: enters the symbol with the value 0 (DIR-21). A second
 * .extern of the same symbol is accepted silently (DIR-34); a symbol that
 * is a label of the file is an error (DIR-36, ERR-53), and so is the name
 * of a macro (LBL-8).
 *   pass       the state of the pass
 *   statement  a valid .extern statement
 * Returns FALSE when the line has an error or memory is exhausted.
 */
static Boolean declare_external(FirstPass *pass, const Statement *statement)
{
    const char *name = statement_directive_symbol(statement);
    const Symbol *symbol = symbol_table_find(pass->symbols, name);

    if (macro_table_find(pass->macros, name) != NULL)
    {
        errors_report(pass->errors, pass->line_number,
                      "the symbol '%s' has the name of a macro", name);
        return FALSE;
    }
    if (symbol == NULL)
    {
        return add_symbol(pass, name, EXTERNAL_SYMBOL_VALUE,
                          SYMBOL_EXTERNAL);
    }
    if (symbol_kind(symbol) != SYMBOL_EXTERNAL)
    {
        errors_report(pass->errors, pass->line_number,
                      "the symbol '%s' is defined as a label in this file",
                      name);
        return FALSE;
    }
    return TRUE;
}

/*
 * Handles a statement that has no syntax error, according to its kind
 * (ALG-9 steps 5 to 16). .entry is left to the second pass (DIR-19).
 *   pass       the state of the pass
 *   statement  the valid statement
 * Returns FALSE when the line turned out to have an error.
 */
static Boolean handle_statement(FirstPass *pass, const Statement *statement)
{
    switch (statement_kind(statement))
    {
    case STATEMENT_INSTRUCTION:
        return (define_label(pass, statement, TRUE) &&
                store_instruction(pass, statement))
                   ? TRUE
                   : FALSE;
    case STATEMENT_DATA:
    case STATEMENT_STRING:
    case STATEMENT_MAT:
        return (define_label(pass, statement, TRUE) &&
                store_data(pass, statement))
                   ? TRUE
                   : FALSE;
    case STATEMENT_EXTERN:
        return declare_external(pass, statement);
    default:
        return TRUE;
    }
}

/*
 * Handles one line of the .am file; the StatementHandler of the first pass.
 *   statement    the parsed line, valid or not
 *   line_number  the number of the line in the .as file
 *   argument     the state of the pass, a FirstPass
 * Returns FALSE when memory is exhausted.
 */
static Boolean handle_line(const Statement *statement, int line_number,
                           void *argument)
{
    FirstPass *pass = argument;

    pass->line_number = line_number;
    if (!statement_is_valid(statement))
    {
        /* The line has its message; its label is still defined (LBL-23). */
        define_label(pass, statement, FALSE);
    }
    else if (handle_statement(pass, statement) &&
             statement_has_ignored_label(statement))
    {
        /* Only a line without an error gets the warning (ERR-R16). */
        errors_warn(pass->errors, pass->line_number,
                    "the label in front of .entry or .extern is ignored");
    }
    return pass->no_memory ? FALSE : TRUE;
}

/*
 * Prepares the state of the first pass of one source file.
 *   pass     the state to fill
 *   macros   the macros of the file
 *   symbols  the symbol table to fill
 *   image    the memory image to fill
 *   errors   the diagnostics context of the file
 */
static void init_pass(FirstPass *pass, const MacroTable *macros,
                      SymbolTable *symbols, MemoryImage *image,
                      ErrorContext *errors)
{
    pass->macros = macros;
    pass->symbols = symbols;
    pass->image = image;
    pass->errors = errors;
    pass->line_number = NO_LINE;
    pass->overflow_reported = FALSE;
    pass->no_memory = FALSE;
}

/* Runs the first pass on one expanded source file; see first_pass.h. */
StageResult first_pass(FILE *am_file, const LineMap *lines,
                       const MacroTable *macros, SymbolTable *symbols,
                       MemoryImage *image, ErrorContext *errors)
{
    FirstPass pass;
    StageResult result;

    init_pass(&pass, macros, symbols, image, errors);
    result = am_reader_for_each(am_file, lines, errors, handle_line, &pass);
    if (result == STAGE_OK)
    {
        /* The data image follows the code image (ALG-6). */
        symbol_table_relocate_data(symbols,
                                   memory_image_instruction_counter(image));
    }
    return result;
}
