/*
 * encoder.h - the layout of the machine words.
 *
 * This module is the only place that knows where the fields of a 10-bit
 * word are (SPEC sections 2.2 and 2.3). Its functions are pure: each builds
 * one word from the values of its fields and touches nothing else. A word is
 * returned in the low WORD_BITS bits of an unsigned int; the bits above them
 * are always zero.
 */
#ifndef ENCODER_H
#define ENCODER_H

#include "common.h"

/* The values of the A,R,E field, bits 1-0 of an instruction word (ENC-5). */
typedef enum
{
    ARE_ABSOLUTE = 0,   /* The word does not depend on the load address. */
    ARE_EXTERNAL = 1,   /* The word depends on an external symbol. */
    ARE_RELOCATABLE = 2 /* The word holds an address inside the file. */
} AreKind;

/*
 * The content of a mode field of the first word of an instruction that has
 * no operand for the field (ENC-9, ENC-10).
 */
#define MODE_FIELD_UNUSED 0

/*
 * Builds the first word of an instruction (ENC-3, ENC-4): the opcode in
 * bits 9-6, the source mode in bits 5-4, the destination mode in bits 3-2
 * and the A,R,E field 00.
 *   opcode            the opcode, 0 to 15
 *   source_mode       the number of the addressing mode of the source
 *                     operand, an AddressingMode, or MODE_FIELD_UNUSED when
 *                     the instruction has no source operand
 *   destination_mode  the number of the addressing mode of the destination
 *                     operand, or MODE_FIELD_UNUSED when the instruction
 *                     has no operand
 * Returns the word.
 */
unsigned int encode_first_word(int opcode, int source_mode,
                               int destination_mode);

/*
 * Builds the additional word of an immediate operand (ADR-3): the value in
 * bits 9-2, in two's complement cut to eight bits (ENC-15), and A,R,E 00.
 *   value  the operand, -128 to 127
 * Returns the word.
 */
unsigned int encode_immediate(int value);

/*
 * Builds a register word (ADR-9 to ADR-12): the source register in bits
 * 9-6, the destination register in bits 5-2 and A,R,E 00. Two register
 * operands share this one word (ADR-11); a single register operand passes
 * zero for the half it does not use (ENC-12).
 *   source_register       the number of the source register, 0 to 7
 *   destination_register  the number of the destination register, 0 to 7
 * Returns the word.
 */
unsigned int encode_registers(int source_register, int destination_register);

/*
 * Builds the second additional word of a matrix operand (ADR-7): the row
 * register in bits 9-6, the column register in bits 5-2 and A,R,E 00.
 *   row_register     the number of the register that holds the row
 *   column_register  the number of the register that holds the column
 * Returns the word.
 */
unsigned int encode_matrix_indices(int row_register, int column_register);

/*
 * Builds a word that holds the address of a symbol (ADR-5): the address in
 * bits 9-2 and the A,R,E field in bits 1-0.
 *   address  the address; zero for an external symbol (ENC-8)
 *   are      ARE_RELOCATABLE for a symbol of the file, ARE_EXTERNAL for an
 *            external symbol
 * Returns the word.
 */
unsigned int encode_address(int address, AreKind are);

/*
 * Builds a data word (DIR-3): the value fills all ten bits, in two's
 * complement (ENC-15), and there is no A,R,E field.
 *   value  the value, -512 to 511
 * Returns the word.
 */
unsigned int encode_data(int value);

#endif /* ENCODER_H */
