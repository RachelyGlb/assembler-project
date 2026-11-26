/*
 * statement.c - the model of one parsed line of the expanded source file.
 *
 * The Statement structure and the Operand structure are defined here and
 * nowhere else. The second half of the file holds the layout of an
 * instruction in machine words: how many additional words an operand takes
 * and at which offset they lie (ADR-14).
 *
 * See statement.h for the contracts of the public functions.
 */
#include "statement.h"

#include <stdlib.h>

#include "util.h"

/* The numbers of words of the parts of an instruction (ADR-14). */
#define FIRST_WORD_COUNT 1
#define SIMPLE_OPERAND_WORDS 1
#define MATRIX_OPERAND_WORDS 2

/*
 * The position of the index word among the two additional words of a
 * matrix operand (ADR-7).
 */
#define MATRIX_INDEX_WORD 1

/* The first size of the array of data values. */
#define VALUES_INITIAL_CAPACITY 8

/* One operand of an instruction. */
typedef struct
{
    AddressingMode mode; /* The addressing mode. */
    int value;           /* The immediate value or the register number. */
    char *symbol;        /* The label of a direct or matrix operand; owned. */
    int row_register;    /* The row register of a matrix operand. */
    int column_register; /* The column register of a matrix operand. */
} Operand;

/* One parsed line. */
struct Statement
{
    StatementKind kind;             /* The kind of the statement. */
    Boolean valid;                  /* FALSE once an error was reported. */
    char *label;                    /* The legal label, or NULL; owned. */
    Boolean label_ignored;          /* A label before .entry or .extern. */
    const Instruction *instruction; /* The operation, or NULL. */
    int operand_count;              /* The number of operands added. */
    /*
     * The operands, as written. Only the first operand_count elements hold
     * an operand; an absent operand has no content at all, and nothing
     * reads it.
     */
    Operand operands[MAX_OPERAND_COUNT];
    char *directive_symbol;         /* The operand of .entry/.extern; owned. */
    int *values;                    /* The listed data values; owned. */
    int value_count;                /* The number of listed values. */
    size_t value_capacity;          /* The number of elements allocated. */
    int data_count;                 /* The number of data words taken. */
};

/* Creates a statement with nothing in it; see statement.h. */
Statement *statement_create(void)
{
    Statement *statement = (Statement *)malloc(sizeof(Statement));

    if (statement == NULL)
    {
        return NULL;
    }
    statement->kind = STATEMENT_UNKNOWN;
    statement->valid = TRUE;
    statement->label = NULL;
    statement->label_ignored = FALSE;
    statement->instruction = NULL;
    statement->operand_count = 0;
    statement->directive_symbol = NULL;
    statement->values = NULL;
    statement->value_count = 0;
    statement->value_capacity = 0;
    statement->data_count = 0;
    return statement;
}

/* Frees a statement; see statement.h. */
void statement_destroy(Statement *statement)
{
    int index;

    if (statement == NULL)
    {
        return;
    }
    for (index = 0; index < statement->operand_count; index++)
    {
        free(statement->operands[index].symbol);
    }
    free(statement->label);
    free(statement->directive_symbol);
    free(statement->values);
    free(statement);
}

/* Marks a statement as not valid; see statement.h. */
void statement_mark_invalid(Statement *statement)
{
    statement->valid = FALSE;
}

/* Sets the kind of a statement; see statement.h. */
void statement_set_kind(Statement *statement, StatementKind kind)
{
    statement->kind = kind;
}

/* Gives a statement its label; see statement.h. */
void statement_set_label(Statement *statement, char *name)
{
    statement->label = name;
}

/* Notes an ignored label; see statement.h. */
void statement_ignore_label(Statement *statement)
{
    statement->label_ignored = TRUE;
}

/* Makes a statement an instruction; see statement.h. */
void statement_set_instruction(Statement *statement,
                               const Instruction *instruction)
{
    statement->instruction = instruction;
    statement->kind = STATEMENT_INSTRUCTION;
}

/*
 * Takes the next free operand of a statement into use, as an operand that
 * carries no number, no symbol and no index registers.
 *   statement  a statement with fewer than MAX_OPERAND_COUNT operands
 *   mode       the addressing mode of the operand
 * Returns the operand, for the caller to give it what its mode carries.
 */
static Operand *add_operand(Statement *statement, AddressingMode mode)
{
    Operand *operand = &statement->operands[statement->operand_count++];

    operand->mode = mode;
    operand->value = 0;
    operand->symbol = NULL;
    operand->row_register = 0;
    operand->column_register = 0;
    return operand;
}

/* Adds an immediate operand; see statement.h. */
void statement_add_immediate_operand(Statement *statement, int value)
{
    add_operand(statement, ADDRESSING_IMMEDIATE)->value = value;
}

/* Adds a direct operand; see statement.h. */
void statement_add_direct_operand(Statement *statement, char *symbol)
{
    add_operand(statement, ADDRESSING_DIRECT)->symbol = symbol;
}

/* Adds a matrix operand; see statement.h. */
void statement_add_matrix_operand(Statement *statement, char *symbol,
                                  int row_register, int column_register)
{
    Operand *operand = add_operand(statement, ADDRESSING_MATRIX);

    operand->symbol = symbol;
    operand->row_register = row_register;
    operand->column_register = column_register;
}

/* Adds a register operand; see statement.h. */
void statement_add_register_operand(Statement *statement, int number)
{
    add_operand(statement, ADDRESSING_REGISTER)->value = number;
}

/* Appends one data word with a listed value; see statement.h. */
Boolean statement_add_data_value(Statement *statement, int value)
{
    int *values = (int *)util_reserve(statement->values,
                                      &statement->value_capacity,
                                      (size_t)statement->value_count + 1,
                                      VALUES_INITIAL_CAPACITY, sizeof(int));

    if (values == NULL)
    {
        return FALSE;
    }
    statement->values = values;
    statement->values[statement->value_count++] = value;
    statement->data_count = statement->value_count;
    return TRUE;
}

/* Extends the data words with zero words; see statement.h. */
void statement_extend_data(Statement *statement, int count)
{
    /* The words are not stored: statement_data_value answers zero. */
    statement->data_count = count;
}

/* Gives a statement the symbol of .entry or .extern; see statement.h. */
void statement_set_directive_symbol(Statement *statement, char *name)
{
    statement->directive_symbol = name;
}

/* Tells whether the line was free of syntax errors; see statement.h. */
Boolean statement_is_valid(const Statement *statement)
{
    return statement->valid;
}

/* Returns the kind of a statement; see statement.h. */
StatementKind statement_kind(const Statement *statement)
{
    return statement->kind;
}

/* Tells whether a statement stores data words; see statement.h. */
Boolean statement_stores_data(const Statement *statement)
{
    switch (statement->kind)
    {
    case STATEMENT_DATA:
    case STATEMENT_STRING:
    case STATEMENT_MAT:
        return TRUE;
    default:
        return FALSE;
    }
}

/* Returns the label defined by the line; see statement.h. */
const char *statement_label(const Statement *statement)
{
    return statement->label;
}

/* Tells whether the line has an ignored label; see statement.h. */
Boolean statement_has_ignored_label(const Statement *statement)
{
    return statement->label_ignored;
}

/* Returns the operation of an instruction; see statement.h. */
const Instruction *statement_instruction(const Statement *statement)
{
    return statement->instruction;
}

/* Returns the number of operands; see statement.h. */
int statement_operand_count(const Statement *statement)
{
    return statement->operand_count;
}

/* Tells whether an operand is the source operand; see statement.h. */
Boolean statement_operand_is_source(const Statement *statement, int index)
{
    /* Only the first of two operands is a source (ENC-1, ENC-9). */
    return (index == 0 && statement->operand_count == MAX_OPERAND_COUNT)
               ? TRUE
               : FALSE;
}

/* Returns the addressing mode of an operand; see statement.h. */
AddressingMode statement_operand_mode(const Statement *statement, int index)
{
    return statement->operands[index].mode;
}

/* Returns the number that an operand carries; see statement.h. */
int statement_operand_value(const Statement *statement, int index)
{
    return statement->operands[index].value;
}

/* Returns the symbol that an operand names; see statement.h. */
const char *statement_operand_symbol(const Statement *statement, int index)
{
    return statement->operands[index].symbol;
}

/* Returns one index register of a matrix operand; see statement.h. */
int statement_operand_matrix_register(const Statement *statement, int index,
                                      MatrixIndex which)
{
    return (which == MATRIX_ROW) ? statement->operands[index].row_register
                                 : statement->operands[index].column_register;
}

/*
 * Returns the number of additional words that one operand takes when it
 * does not share a word (ADR-14).
 *   operand  the operand
 */
static int operand_words(const Operand *operand)
{
    return (operand->mode == ADDRESSING_MATRIX) ? MATRIX_OPERAND_WORDS
                                                : SIMPLE_OPERAND_WORDS;
}

/* Returns where the words of an operand lie; see statement.h. */
int statement_operand_word_offset(const Statement *statement, int index)
{
    const Operand *first = &statement->operands[0];
    const Operand *operand = &statement->operands[index];

    /* The first operand follows the first word of the instruction. */
    if (index == 0)
    {
        return FIRST_WORD_COUNT;
    }
    /* Two registers share one word (ADR-11); nothing else does (ADR-13). */
    if (first->mode == ADDRESSING_REGISTER &&
        operand->mode == ADDRESSING_REGISTER)
    {
        return FIRST_WORD_COUNT;
    }
    return FIRST_WORD_COUNT + operand_words(first);
}

/* Returns where the index word of a matrix operand lies; see statement.h. */
int statement_operand_index_word_offset(const Statement *statement,
                                        int index)
{
    return statement_operand_word_offset(statement, index) +
           MATRIX_INDEX_WORD;
}

/*
 * Returns the length of an instruction in words (ADR-14): the offset of its
 * last operand plus the words of that operand, or one word when it has no
 * operand.
 *   statement  a valid instruction statement
 */
static int instruction_words(const Statement *statement)
{
    int last = statement->operand_count - 1;

    if (last < 0)
    {
        return FIRST_WORD_COUNT;
    }
    return statement_operand_word_offset(statement, last) +
           operand_words(&statement->operands[last]);
}

/* Returns the number of machine words of a statement; see statement.h. */
int statement_word_count(const Statement *statement)
{
    if (statement->kind == STATEMENT_INSTRUCTION)
    {
        return instruction_words(statement);
    }
    return statement_data_count(statement);
}

/* Returns the number of data words of a directive; see statement.h. */
int statement_data_count(const Statement *statement)
{
    return statement->data_count;
}

/* Returns one data word of a directive, as a number; see statement.h. */
int statement_data_value(const Statement *statement, int index)
{
    /* A cell without a listed value is zero (DIR-13, DIR-24). */
    if (index < 0 || index >= statement->value_count)
    {
        return 0;
    }
    return statement->values[index];
}

/* Returns the symbol of .entry or .extern; see statement.h. */
const char *statement_directive_symbol(const Statement *statement)
{
    return statement->directive_symbol;
}
