/*
 * statement.h - the model of one parsed line of the expanded source file.
 *
 * A Statement is what the parser of the syntax module makes of one line of
 * the .am file: the optional label, the kind of the statement, the
 * operation and its operands with their addressing modes and values, or the
 * directive with its data words or its symbol. Both passes work on the
 * Statement; no module but the parser looks at the text of a line.
 *
 * The structure is private to statement.c, for the parser too. The parser
 * fills a new statement through the building functions of the first half
 * of this header; the passes read it through the access functions of the
 * second half. The Statement also answers how many machine words it takes
 * and where the words of each operand lie, so that the length of an
 * instruction is computed in exactly one place (ADR-14).
 *
 * What a statement holds:
 *   - A new statement is valid, has the kind STATEMENT_UNKNOWN and holds
 *     nothing else.
 *   - A valid statement holds everything that its kind needs, and every
 *     function of this header may be asked about it.
 *   - A statement that is not valid (statement_mark_invalid) may be filled
 *     only in part. Only statement_is_valid, statement_kind,
 *     statement_stores_data and statement_label are meaningful for it.
 *   - The label is a legal label name or is absent, also in a statement
 *     that is not valid: the first pass defines the label of a faulty line
 *     (LBL-23).
 */
#ifndef STATEMENT_H
#define STATEMENT_H

#include "common.h"
#include "instruction_table.h"

/* One parsed line. The structure is private. */
typedef struct Statement Statement;

/* The kinds of statement (SYN-3, DIR-4). */
typedef enum
{
    STATEMENT_EMPTY,       /* Only white space (SYN-4). */
    STATEMENT_COMMENT,     /* The first character is ';' (SYN-5). */
    STATEMENT_INSTRUCTION, /* An operation with its operands. */
    STATEMENT_DATA,        /* A .data directive. */
    STATEMENT_STRING,      /* A .string directive. */
    STATEMENT_MAT,         /* A .mat directive. */
    STATEMENT_ENTRY,       /* A .entry directive. */
    STATEMENT_EXTERN,      /* A .extern directive. */
    STATEMENT_UNKNOWN      /* A faulty line whose kind was not recognised. */
} StatementKind;

/* The two index registers of a matrix operand (ADR-6). */
typedef enum
{
    MATRIX_ROW,   /* The register in the first pair of brackets. */
    MATRIX_COLUMN /* The register in the second pair of brackets. */
} MatrixIndex;

/*
 * Creation, destruction and the building functions, for the parser.
 *
 * The functions that receive a name take a string on the heap and make the
 * statement its owner: the statement frees it, and the caller must not.
 */

/*
 * Creates a statement with nothing in it: valid, of the kind
 * STATEMENT_UNKNOWN, without a label, operands, data words or a symbol.
 * Returns the statement, which the caller frees with statement_destroy, or
 * NULL when memory is exhausted.
 */
Statement *statement_create(void);

/*
 * Frees a statement and everything it owns.
 *   statement  the statement to free; NULL is allowed and does nothing
 */
void statement_destroy(Statement *statement);

/*
 * Marks a statement as not valid, for a line that has a syntax error. What
 * the statement holds already is kept.
 *   statement  the statement to mark
 */
void statement_mark_invalid(Statement *statement);

/*
 * Sets the kind of a statement that is not an instruction.
 *   statement  the statement to change
 *   kind       the kind; for STATEMENT_INSTRUCTION use
 *              statement_set_instruction
 */
void statement_set_kind(Statement *statement, StatementKind kind);

/*
 * Gives a statement its label. Only a legal label name may be given, so
 * that a faulty line never defines an illegal label (LBL-23).
 *   statement  a statement without a label
 *   name       the legal name without its colon, a string on the heap that
 *              the statement now owns
 */
void statement_set_label(Statement *statement, char *name);

/*
 * Notes that the line has a label in front of .entry or .extern, which is
 * ignored (DIR-35). Such a statement gets no label.
 *   statement  the statement to change
 */
void statement_ignore_label(Statement *statement);

/*
 * Makes a statement an instruction: sets its operation and the kind
 * STATEMENT_INSTRUCTION.
 *   statement    the statement to change
 *   instruction  the row of the instruction table, not NULL
 */
void statement_set_instruction(Statement *statement,
                               const Instruction *instruction);

/*
 * Adds an immediate operand after the operands that an instruction has
 * (ADR-2).
 *   statement  a statement with fewer than MAX_OPERAND_COUNT operands
 *   value      the value of the operand
 */
void statement_add_immediate_operand(Statement *statement, int value);

/*
 * Adds a direct operand after the operands that an instruction has (ADR-4).
 *   statement  a statement with fewer than MAX_OPERAND_COUNT operands
 *   symbol     the label that the operand names, a string on the heap that
 *              the statement now owns
 */
void statement_add_direct_operand(Statement *statement, char *symbol);

/*
 * Adds a matrix operand after the operands that an instruction has (ADR-6).
 *   statement        a statement with fewer than MAX_OPERAND_COUNT operands
 *   symbol           the label of the matrix, a string on the heap that the
 *                    statement now owns
 *   row_register     the number of the register of the first index, 0 to 7
 *   column_register  the number of the register of the second index, 0 to 7
 */
void statement_add_matrix_operand(Statement *statement, char *symbol,
                                  int row_register, int column_register);

/*
 * Adds a register operand after the operands that an instruction has
 * (ADR-8).
 *   statement  a statement with fewer than MAX_OPERAND_COUNT operands
 *   number     the number of the register, 0 to 7
 */
void statement_add_register_operand(Statement *statement, int number);

/*
 * Appends one data word with a listed value to a .data, .string or .mat
 * statement. The data count grows by one.
 *   statement  the statement to change
 *   value      the value of the word
 * Returns FALSE when memory is exhausted; the statement is then unchanged.
 */
Boolean statement_add_data_value(Statement *statement, int value);

/*
 * Extends the data words of a statement to a given count with words that
 * have no listed value and are zero: the cells of a matrix beyond its value
 * list (DIR-13, DIR-24). This needs no memory.
 *   statement  the statement to change
 *   count      the new data count, not less than the current one
 */
void statement_extend_data(Statement *statement, int count);

/*
 * Gives a .entry or .extern statement the symbol that it names.
 *   statement  a statement without such a symbol
 *   name       the name, a string on the heap that the statement now owns
 */
void statement_set_directive_symbol(Statement *statement, char *name);

/*
 * The access functions, for the passes and for the parser.
 */

/*
 * Tells whether the line was free of syntax errors.
 *   statement  the statement to ask
 * Returns FALSE when the parser reported an error for the line. Only
 * statement_kind, statement_stores_data and statement_label are meaningful
 * for such a statement.
 */
Boolean statement_is_valid(const Statement *statement);

/*
 * Returns the kind of a statement. For a statement that is not valid it is
 * the kind that was recognised before the error was found, or
 * STATEMENT_UNKNOWN.
 *   statement  the statement to ask
 */
StatementKind statement_kind(const Statement *statement);

/*
 * Tells whether a statement stores words in the data image, so that its
 * label is a data label (DIR-8, DIR-11, DIR-16). The answer depends on the
 * kind alone, so it is meaningful for a statement that is not valid too.
 *   statement  the statement to ask
 * Returns TRUE for a .data, .string or .mat directive.
 */
Boolean statement_stores_data(const Statement *statement);

/*
 * Returns the label defined at the start of the line.
 *   statement  the statement to ask
 * Returns the name without its colon, or NULL when the line defines no
 * label, when the name is not a legal label name, or when the label stands
 * in front of .entry or .extern and is ignored (DIR-35).
 */
const char *statement_label(const Statement *statement);

/*
 * Tells whether the line has a label in front of .entry or .extern, which
 * is ignored and earns the warning WRN-1 (DIR-35).
 *   statement  the statement to ask
 * Returns TRUE for such a line.
 */
Boolean statement_has_ignored_label(const Statement *statement);

/*
 * Returns the operation of an instruction statement.
 *   statement  a statement of the kind STATEMENT_INSTRUCTION
 * Returns the row of the instruction table, or NULL for another kind.
 */
const Instruction *statement_instruction(const Statement *statement);

/*
 * Returns the number of operands of an instruction statement, 0 to 2. While
 * a statement is being built, it is the number of operands added so far.
 *   statement  the statement to ask
 */
int statement_operand_count(const Statement *statement);

/*
 * Tells whether an operand is the source operand. With two operands the
 * first is the source and the second the destination (ENC-1); a single
 * operand is the destination (ENC-9).
 *   statement  a valid instruction statement
 *   index      the position of the operand as written, from 0
 * Returns TRUE for the source operand.
 */
Boolean statement_operand_is_source(const Statement *statement, int index);

/*
 * Returns the addressing mode of an operand (ADR-15).
 *   statement  a valid instruction statement
 *   index      the position of the operand as written, from 0
 */
AddressingMode statement_operand_mode(const Statement *statement, int index);

/*
 * Returns the number that an operand carries.
 *   statement  a valid instruction statement
 *   index      the position of the operand as written, from 0
 * Returns the value of an immediate operand or the number of a register
 * operand; zero for the other modes.
 */
int statement_operand_value(const Statement *statement, int index);

/*
 * Returns the symbol that an operand names.
 *   statement  a valid instruction statement
 *   index      the position of the operand as written, from 0
 * Returns the label of a direct operand or of a matrix operand, or NULL for
 * the other modes.
 */
const char *statement_operand_symbol(const Statement *statement, int index);

/*
 * Returns one index register of a matrix operand.
 *   statement  a valid instruction statement
 *   index      the position of the operand as written, from 0
 *   which      MATRIX_ROW or MATRIX_COLUMN
 * Returns the register number, 0 to 7.
 */
int statement_operand_matrix_register(const Statement *statement, int index,
                                      MatrixIndex which);

/*
 * Returns where the additional words of an operand lie inside the
 * instruction (ENC-11, ADR-11, ADR-13).
 *   statement  a valid instruction statement
 *   index      the position of the operand as written, from 0
 * Returns the offset of the first additional word of the operand from the
 * first word of the instruction. Two register operands get the same offset,
 * because they share one word. The second word of a matrix operand is
 * answered by statement_operand_index_word_offset.
 */
int statement_operand_word_offset(const Statement *statement, int index);

/*
 * Returns where the index word of a matrix operand lies inside the
 * instruction: the second of its two additional words, which holds the two
 * index registers (ADR-7).
 *   statement  a valid instruction statement
 *   index      the position of a matrix operand as written, from 0
 * Returns the offset of the index word from the first word of the
 * instruction.
 */
int statement_operand_index_word_offset(const Statement *statement,
                                        int index);

/*
 * Returns the number of machine words that a statement takes. This is the
 * one place where the length of an instruction is computed (ADR-14).
 *   statement  a valid statement
 * Returns the length of an instruction, 1 to 5; the number of data words of
 * a .data, .string or .mat directive; zero for every other kind.
 */
int statement_word_count(const Statement *statement);

/*
 * Returns the number of data words of a .data, .string or .mat directive:
 * the values of .data (DIR-7), the characters of .string and the
 * terminating zero (DIR-10), the rows times the columns of .mat (DIR-13).
 * While a statement is being built, it is the number of words added so far.
 *   statement  a valid statement
 * Returns the count; zero for every other kind.
 */
int statement_data_count(const Statement *statement);

/*
 * Returns one data word of a .data, .string or .mat directive, as a number.
 *   statement  a valid statement of one of these kinds
 *   index      the position of the word, from 0 to the data count - 1
 * Returns the value. A cell of a matrix that has no listed value is zero
 * (DIR-13, DIR-24).
 */
int statement_data_value(const Statement *statement, int index);

/*
 * Returns the symbol named by a .entry or .extern directive.
 *   statement  a valid statement of the kind STATEMENT_ENTRY or
 *              STATEMENT_EXTERN
 * Returns the name, or NULL for another kind.
 */
const char *statement_directive_symbol(const Statement *statement);

#endif /* STATEMENT_H */
