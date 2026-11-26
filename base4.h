/*
 * base4.h - the "unique base 4" notation of the output files.
 *
 * A number is written with the four digits a, b, c and d, which stand for
 * 0, 1, 2 and 3 (ENC-13). The functions write into a buffer of the caller
 * and allocate nothing.
 */
#ifndef BASE4_H
#define BASE4_H

#include <limits.h>

#include "common.h"

/* The number of bits that one base-4 digit stands for. */
#define BASE4_BITS_PER_DIGIT 2

/* The number of digits of a machine word and of an address (OUT-5). */
#define BASE4_WORD_DIGITS (WORD_BITS / BASE4_BITS_PER_DIGIT)
#define BASE4_ADDRESS_DIGITS (OPERAND_FIELD_BITS / BASE4_BITS_PER_DIGIT)

/* The most digits that any unsigned int can need. */
#define BASE4_MAX_DIGITS \
    ((int)(sizeof(unsigned int) * CHAR_BIT / BASE4_BITS_PER_DIGIT))

/* The size of a buffer that can hold any result, with its terminator. */
#define BASE4_BUFFER_SIZE (BASE4_MAX_DIGITS + 1)

/*
 * Writes a number with a fixed number of digits, padded on the left with
 * the digit a (OUT-13). Digits of the number that do not fit are dropped
 * from the left.
 *   value   the number to write
 *   digits  the number of digits to write, 1 to BASE4_MAX_DIGITS
 *   buffer  receives the digits and a terminating zero; it must have room
 *           for digits + 1 characters
 */
void base4_format(unsigned int value, int digits, char *buffer);

/*
 * Writes a number with as many digits as it needs and no leading a; zero is
 * written as the single digit a (OUT-13).
 *   value   the number to write
 *   buffer  receives the digits and a terminating zero; it must have room
 *           for BASE4_BUFFER_SIZE characters
 */
void base4_format_shortest(unsigned int value, char *buffer);

#endif /* BASE4_H */
