/*
 * instruction_table.h - the words of the assembly language.
 *
 * This module owns the single table of the sixteen operations (name, opcode,
 * number of operands, legal source modes, legal destination modes; SPEC
 * section 3), the register names, the directive names and the list of
 * reserved words (LBL-20). The tables are private to the module (IMP-1);
 * every other module asks through the functions below.
 */
#ifndef INSTRUCTION_TABLE_H
#define INSTRUCTION_TABLE_H

#include <stddef.h>

#include "common.h"

/* One row of the instruction table. The structure is private. */
typedef struct Instruction Instruction;

/* The five directives (DIR-4), and the answer for "not a directive". */
typedef enum
{
    DIRECTIVE_NONE,
    DIRECTIVE_DATA,
    DIRECTIVE_STRING,
    DIRECTIVE_MAT,
    DIRECTIVE_ENTRY,
    DIRECTIVE_EXTERN
} DirectiveKind;

/* What register_number returns for a name that is not a register. */
#define NOT_A_REGISTER (-1)

/*
 * Looks up an operation by its name. The comparison is case-sensitive.
 *   name  the operation name, for example "mov"
 * Returns the row of the operation, or NULL when there is no such operation.
 */
const Instruction *instruction_find(const char *name);

/*
 * Looks up an operation by a name that is given as a piece of a longer
 * text. The comparison is case-sensitive.
 *   text    the first character of the name; no terminating zero is needed
 *   length  the number of characters of the name
 * Returns the row of the operation, or NULL when there is no such operation.
 */
const Instruction *instruction_find_text(const char *text, size_t length);

/*
 * Returns the opcode of an operation, 0 to 15 (INS-1).
 *   instruction  a row returned by instruction_find
 */
int instruction_opcode(const Instruction *instruction);

/*
 * Returns the number of operands of an operation, 0 to 2 (INS-3).
 *   instruction  a row returned by instruction_find
 */
int instruction_operand_count(const Instruction *instruction);

/*
 * Tells whether an addressing mode is legal for the source operand (INS-4).
 *   instruction  a row returned by instruction_find
 *   mode         the addressing mode of the source operand
 * Returns TRUE when the mode is legal; always FALSE for an operation that
 * has no source operand.
 */
Boolean instruction_allows_source(const Instruction *instruction,
                                  AddressingMode mode);

/*
 * Tells whether an addressing mode is legal for the destination operand
 * (INS-4).
 *   instruction  a row returned by instruction_find
 *   mode         the addressing mode of the destination operand
 * Returns TRUE when the mode is legal; always FALSE for an operation that
 * has no operand.
 */
Boolean instruction_allows_destination(const Instruction *instruction,
                                       AddressingMode mode);

/*
 * Looks up a register by its name (M-2). The comparison is case-sensitive,
 * and a name such as "r8" is not a register (LBL-16).
 *   name  the candidate name
 * Returns the register number, 0 to 7, or NOT_A_REGISTER.
 */
int register_number(const char *name);

/*
 * Looks up a register by a name that is given as a piece of a longer text.
 *   text    the first character of the name; no terminating zero is needed
 *   length  the number of characters of the name
 * Returns the register number, 0 to 7, or NOT_A_REGISTER.
 */
int register_number_text(const char *text, size_t length);

/*
 * Looks up a directive by its name.
 *   name  the candidate name, with its leading dot, for example ".data"
 * Returns the directive, or DIRECTIVE_NONE when the name is not a directive.
 */
DirectiveKind directive_find(const char *name);

/*
 * Looks up a directive by a name that is given as a piece of a longer text.
 *   text    the first character of the name, its leading dot; no
 *           terminating zero is needed
 *   length  the number of characters of the name, the dot included
 * Returns the directive, or DIRECTIVE_NONE when the name is not a directive.
 */
DirectiveKind directive_find_text(const char *text, size_t length);

/*
 * The one test for a reserved word (LBL-20): an operation name, a directive
 * name written without the dot, a register name r0 to r7, "mcro" or
 * "mcroend". The comparison is case-sensitive.
 *   word  the candidate name
 * Returns TRUE when word may be neither a label name nor a macro name.
 */
Boolean is_reserved_word(const char *word);

#endif /* INSTRUCTION_TABLE_H */
