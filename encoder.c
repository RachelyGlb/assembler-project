/*
 * encoder.c - the layout of the machine words.
 *
 * Every bit position and every field width of SPEC sections 2.2 and 2.3 is
 * named here, once. See encoder.h for the contracts.
 */
#include "encoder.h"

/* The position of the lowest bit of each field of a first word (ENC-3). */
#define OPCODE_SHIFT 6
#define SOURCE_MODE_SHIFT 4
#define DESTINATION_MODE_SHIFT 2

/* The widths, in bits, of the fields of a first word (ENC-3). */
#define OPCODE_BITS 4
#define MODE_BITS 2

/* The width of the A,R,E field, bits 1-0 (ENC-3). */
#define ARE_BITS 2

/* The position of the value of an operand word, bits 9-2 (ADR-3, ADR-5). */
#define OPERAND_FIELD_SHIFT ARE_BITS

/* The positions and the width of the two halves of a register word. */
#define SOURCE_REGISTER_SHIFT 6
#define DESTINATION_REGISTER_SHIFT 2
#define REGISTER_BITS 4

/* The mask that keeps the lowest bits of a value. */
#define LOW_BITS(count) ((1u << (count)) - 1u)

/*
 * Places a value in a field of a word.
 *   value  the value; a negative value is taken in two's complement
 *   bits   the width of the field
 *   shift  the position of the lowest bit of the field
 * Returns the field, with every other bit zero.
 */
static unsigned int field(int value, int bits, int shift)
{
    /* The conversion to unsigned is the two's complement of ENC-15. */
    return ((unsigned int)value & LOW_BITS(bits)) << shift;
}

/* Builds the first word of an instruction; see encoder.h. */
unsigned int encode_first_word(int opcode, AddressingMode source,
                               AddressingMode destination)
{
    return field(opcode, OPCODE_BITS, OPCODE_SHIFT) |
           field((int)source, MODE_BITS, SOURCE_MODE_SHIFT) |
           field((int)destination, MODE_BITS, DESTINATION_MODE_SHIFT) |
           field((int)ARE_ABSOLUTE, ARE_BITS, 0);
}

/* Builds the word of an immediate operand; see encoder.h. */
unsigned int encode_immediate(int value)
{
    return field(value, OPERAND_FIELD_BITS, OPERAND_FIELD_SHIFT) |
           field((int)ARE_ABSOLUTE, ARE_BITS, 0);
}

/* Builds a register word; see encoder.h. */
unsigned int encode_registers(int source_register, int destination_register)
{
    return field(source_register, REGISTER_BITS, SOURCE_REGISTER_SHIFT) |
           field(destination_register, REGISTER_BITS,
                 DESTINATION_REGISTER_SHIFT) |
           field((int)ARE_ABSOLUTE, ARE_BITS, 0);
}

/* Builds the index word of a matrix operand; see encoder.h. */
unsigned int encode_matrix_indices(int row_register, int column_register)
{
    /* The row and the column lie where a register pair lies (ADR-7). */
    return encode_registers(row_register, column_register);
}

/* Builds a word that holds the address of a symbol; see encoder.h. */
unsigned int encode_address(int address, AreKind are)
{
    return field(address, OPERAND_FIELD_BITS, OPERAND_FIELD_SHIFT) |
           field((int)are, ARE_BITS, 0);
}

/* Builds a data word; see encoder.h. */
unsigned int encode_data(int value)
{
    return field(value, WORD_BITS, 0);
}
