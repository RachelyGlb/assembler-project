/*
 * util.c - small helpers for strings, white space, file names and lines.
 *
 * See util.h for the contracts.
 */
#include "util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* The first size of a line buffer; it doubles whenever it is full. */
#define LINE_BUFFER_INITIAL_SIZE 128
#define LINE_BUFFER_GROWTH_FACTOR 2

/* The character that precedes the newline of a CR LF line ending (SYN-25). */
#define CARRIAGE_RETURN '\r'

/* Tells whether a character is white space; see util.h. */
Boolean util_is_space(char character)
{
    /* isspace is defined only for the values of unsigned char and EOF. */
    return isspace((unsigned char)character) ? TRUE : FALSE;
}

/* Skips white space; see util.h. */
const char *util_skip_spaces(const char *text)
{
    while (*text != '\0' && util_is_space(*text))
    {
        text++;
    }
    return text;
}

/* Skips a run of characters that are not white space; see util.h. */
const char *util_skip_field(const char *text)
{
    while (*text != '\0' && !util_is_space(*text))
    {
        text++;
    }
    return text;
}

/* Tells whether a string holds only white space; see util.h. */
Boolean util_is_blank(const char *text)
{
    return *util_skip_spaces(text) == '\0' ? TRUE : FALSE;
}

/* Copies a string to the heap; see util.h. */
char *util_copy_string(const char *text)
{
    return util_copy_text(text, strlen(text));
}

/* Copies the first characters of a text to the heap; see util.h. */
char *util_copy_text(const char *text, size_t length)
{
    char *copy = (char *)malloc(length + 1);

    if (copy != NULL)
    {
        memcpy(copy, text, length);
        copy[length] = '\0';
    }
    return copy;
}

/* Builds a file name on the heap; see util.h. */
char *util_make_file_name(const char *base, const char *extension)
{
    size_t base_length = strlen(base);
    char *name = (char *)malloc(base_length + strlen(extension) + 1);

    if (name != NULL)
    {
        strcpy(name, base);
        strcpy(name + base_length, extension);
    }
    return name;
}

/*
 * Makes sure that a line buffer has room for one more character and the
 * terminating zero.
 *   buffer    the address of the buffer pointer
 *   capacity  the address of the buffer size
 *   length    the number of characters stored so far
 * Returns FALSE when the buffer had to grow and could not; the old buffer
 * then still belongs to the caller.
 */
static Boolean reserve_character(char **buffer, size_t *capacity,
                                 size_t length)
{
    size_t new_capacity;
    char *larger;

    if (length + 1 < *capacity)
    {
        return TRUE;
    }
    new_capacity = (*capacity == 0) ? LINE_BUFFER_INITIAL_SIZE
                                    : *capacity * LINE_BUFFER_GROWTH_FACTOR;
    larger = (char *)realloc(*buffer, new_capacity);
    if (larger == NULL)
    {
        return FALSE;
    }
    *buffer = larger;
    *capacity = new_capacity;
    return TRUE;
}

/*
 * Hands the length of a line to a caller that asked for it.
 *   length  where to store the length, or NULL when the caller has no use
 *           for it
 *   count   the number of bytes of the line
 */
static void store_length(size_t *length, size_t count)
{
    if (length != NULL)
    {
        *length = count;
    }
}

/* Reads the next line of a text file into a growing buffer; see util.h. */
ReadStatus util_read_line(FILE *file, char **buffer, size_t *capacity,
                          size_t *length, Boolean drop_carriage_return)
{
    size_t count = 0;
    int character = getc(file);

    if (character == EOF)
    {
        /* getc gives EOF for a failed read as well (CLI-12). */
        return ferror(file) ? READ_ERROR : READ_END;
    }
    while (character != EOF && character != NEWLINE_CHARACTER)
    {
        if (!reserve_character(buffer, capacity, count))
        {
            return READ_NO_MEMORY;
        }
        (*buffer)[count++] = (char)character;
        character = getc(file);
    }
    /* A line that a failed read cut short is not a line (CLI-12). */
    if (character == EOF && ferror(file))
    {
        return READ_ERROR;
    }
    /* An empty line stores no character, so the buffer may still be absent. */
    if (!reserve_character(buffer, capacity, count))
    {
        return READ_NO_MEMORY;
    }
    /* Only a carriage return that a newline follows is a line ending. */
    if (drop_carriage_return && character == NEWLINE_CHARACTER && count > 0 &&
        (*buffer)[count - 1] == CARRIAGE_RETURN)
    {
        count--;
    }
    (*buffer)[count] = '\0';
    store_length(length, count);
    return READ_LINE;
}
