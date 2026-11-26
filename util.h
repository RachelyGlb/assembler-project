/*
 * util.h - small helpers for strings, white space, file names and lines.
 *
 * Nothing here knows about the assembly language. The functions that
 * allocate return NULL (or READ_NO_MEMORY) when memory is exhausted; the
 * caller owns what they return and frees it with free().
 */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>

#include "common.h"

/* The result of reading one line with util_read_line. */
typedef enum
{
    READ_LINE,     /* A line was read into the buffer. */
    READ_END,      /* There is no further line. */
    READ_NO_MEMORY /* The buffer could not be enlarged. */
} ReadStatus;

/*
 * Tells whether a character is white space. White space is whatever the C
 * library function isspace accepts (SYN-8); this is the only place that
 * calls it.
 *   character  the character to test, as a char value
 * Returns TRUE for a white-space character.
 */
Boolean util_is_space(char character);

/*
 * Skips white space.
 *   text  a string
 * Returns the first character of text that is not white space (possibly the
 * terminating zero).
 */
const char *util_skip_spaces(const char *text);

/*
 * Skips one field, that is a run of characters that are not white space.
 *   text  a string
 * Returns the first white-space character of text, or its terminating zero.
 */
const char *util_skip_field(const char *text);

/*
 * Tells whether a string holds nothing but white space.
 *   text  a string
 * Returns TRUE when text is empty or all white space.
 */
Boolean util_is_blank(const char *text);

/*
 * Copies a string to the heap.
 *   text  the string to copy
 * Returns the copy, or NULL when memory is exhausted.
 */
char *util_copy_string(const char *text);

/*
 * Copies the first characters of a text to the heap, as a string.
 *   text    the text to copy from; it need not be zero-terminated
 *   length  the number of characters to copy
 * Returns the zero-terminated copy, or NULL when memory is exhausted.
 */
char *util_copy_text(const char *text, size_t length);

/*
 * Builds a file name on the heap.
 *   base       the name without the extension
 *   extension  the extension, with its dot
 * Returns base followed by extension, or NULL when memory is exhausted.
 */
char *util_make_file_name(const char *base, const char *extension);

/*
 * Reads the next line of a text file, of any length, into a heap buffer
 * that grows as needed. A last line without a newline is a complete line
 * (SYN-24). The newline is not stored.
 *   file                 the file to read from
 *   buffer               the address of the buffer pointer; *buffer must be
 *                        NULL before the first call and is reused by the
 *                        following calls. The caller frees it at the end,
 *                        also after READ_END and READ_NO_MEMORY.
 *   capacity             the address of the buffer size; *capacity must be
 *                        0 before the first call
 *   drop_carriage_return TRUE to remove a carriage return that stands
 *                        immediately before the newline (SYN-25); this is
 *                        how a source file is read. FALSE keeps it.
 * Returns READ_LINE when *buffer holds the line as a string, READ_END at the
 * end of the file, READ_NO_MEMORY when the buffer could not grow.
 */
ReadStatus util_read_line(FILE *file, char **buffer, size_t *capacity,
                          Boolean drop_carriage_return);

#endif /* UTIL_H */
