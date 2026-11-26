/*
 * base4.c - the "unique base 4" notation of the output files.
 *
 * See base4.h for the contracts.
 */
#include "base4.h"

/* The base, and the mask that selects the value of one digit. */
#define BASE4_RADIX (1u << BASE4_BITS_PER_DIGIT)
#define BASE4_DIGIT_MASK (BASE4_RADIX - 1u)

/* The fewest digits that a number is written with. */
#define BASE4_MIN_DIGITS 1

/*
 * Returns the character of one digit (ENC-13).
 *   digit  the value of the digit, 0 to 3
 */
static char digit_character(unsigned int digit)
{
    /* The four digits, in the order of their values: a=0, b=1, c=2, d=3. */
    static const char digits[] = "abcd";

    return digits[digit & BASE4_DIGIT_MASK];
}

/* Writes a number with a fixed number of digits; see base4.h. */
void base4_format(unsigned int value, int digits, char *buffer)
{
    int position;

    for (position = digits - 1; position >= 0; position--)
    {
        buffer[position] = digit_character(value & BASE4_DIGIT_MASK);
        value >>= BASE4_BITS_PER_DIGIT;
    }
    buffer[digits] = '\0';
}

/* Writes a number with as many digits as it needs; see base4.h. */
void base4_format_shortest(unsigned int value, char *buffer)
{
    unsigned int rest = value >> BASE4_BITS_PER_DIGIT;
    int digits = BASE4_MIN_DIGITS;

    while (rest != 0)
    {
        digits++;
        rest >>= BASE4_BITS_PER_DIGIT;
    }
    base4_format(value, digits, buffer);
}
