/*
 * syntax.h - the parser of one line of the expanded source file.
 *
 * statement_parse turns one line of the .am file into a Statement: the
 * optional label, the kind of the statement, the operation and its operands
 * with their addressing modes and values, or the directive with its data
 * words or its symbol. It reports every syntax error that can be decided
 * from the line alone (SPEC sections 5.3, ERR-11 to ERR-13, ERR-16 to
 * ERR-40, ERR-42 to ERR-45), one message per line (ERR-R16). Errors that
 * need the symbol table, the macro table or the memory counters are left to
 * the passes.
 *
 * Both passes work on the Statement; no other module looks at the text of a
 * line. The Statement also answers how many machine words it takes and
 * where the words of each operand lie, so that the length of an instruction
 * is computed in exactly one place (ADR-14).
 */
#ifndef SYNTAX_H
#define SYNTAX_H

#include "common.h"
#include "errors.h"
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
 * Parses one line. When the line has a syntax error, the first error found
 * is reported and the statement is returned all the same, marked as not
 * valid: it still carries its label, if the label itself is legal, so that
 * the first pass can define it (LBL-23).
 *   line         the text of the line, without its line terminator
 *   line_number  the number of the line in the .as file, for the message
 *                (ERR-R12)
 *   errors       the diagnostics context of the file
 * Returns the statement, which the caller frees with statement_destroy, or
 * NULL when memory is exhausted (nothing is reported about that here).
 */
Statement *statement_parse(const char *line, int line_number,
                           ErrorContext *errors);

/*
 * Frees a statement.
 *   statement  the statement to free; NULL is allowed and does nothing
 */
void statement_destroy(Statement *statement);

/*
 * Tells whether the line was free of syntax errors.
 *   statement  the statement to ask
 * Returns FALSE when statement_parse reported an error for the line. Only
 * statement_kind and statement_label are meaningful for such a statement.
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
 * Returns the number of operands of an instruction statement, 0 to 2.
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
 * because they share one word. The second word of a matrix operand lies at
 * the next offset.
 */
int statement_operand_word_offset(const Statement *statement, int index);

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

/*
 * The one rule for a label name: a letter followed by letters and digits
 * (LBL-2), at most MAX_NAME_LENGTH characters (LBL-3), and not a reserved
 * word (LBL-20).
 *   name  the candidate name
 * Returns TRUE when name may be the name of a label.
 */
Boolean syntax_is_valid_label(const char *name);

#endif /* SYNTAX_H */
