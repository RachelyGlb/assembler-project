/*
 * output_writer.c - the writer of the output files.
 *
 * Every line of the three files has the same shape, two fields and one TAB
 * (OUT-13), and is written by the one function write_pair. A file is opened
 * and closed by open_output and close_output, so the handling of a file
 * that cannot be written is in one place too. The list of the three files
 * exists once, in remove_output_files. See output_writer.h for the
 * contract.
 */
#include "output_writer.h"

#include <stdio.h>
#include <stdlib.h>

#include "base4.h"
#include "util.h"

/* The layout of every output line: two fields, one TAB, one newline. */
#define LINE_FORMAT "%s\t%s\n"

/*
 * Creates one output file.
 *   base_name  the name of the source file without the extension
 *   extension  the extension of the output file
 *   errors     the diagnostics context of the file
 *   file       receives the open file, or NULL
 * Returns STAGE_OK; STAGE_ERRORS, after a report, when the file cannot be
 * created; STAGE_FATAL when memory is exhausted.
 */
static StageResult open_output(const char *base_name, const char *extension,
                               ErrorContext *errors, FILE **file)
{
    char *file_name = util_make_file_name(base_name, extension);

    *file = NULL;
    if (file_name == NULL)
    {
        return STAGE_FATAL;
    }
    *file = fopen(file_name, WRITE_MODE);
    free(file_name);
    if (*file == NULL)
    {
        errors_report(errors, NO_LINE, "cannot create the '%s' file",
                      extension);
        return STAGE_ERRORS;
    }
    return STAGE_OK;
}

/*
 * Closes an output file and checks that everything was written (ERR-R11).
 *   file       the file to close
 *   extension  the extension of the file, for the message
 *   errors     the diagnostics context of the file
 * Returns STAGE_OK, or STAGE_ERRORS after a report when a write failed.
 */
static StageResult close_output(FILE *file, const char *extension,
                                ErrorContext *errors)
{
    Boolean failed = ferror(file) ? TRUE : FALSE;

    if (fclose(file) != 0)
    {
        failed = TRUE;
    }
    if (failed)
    {
        errors_report(errors, NO_LINE, "cannot write the '%s' file",
                      extension);
        return STAGE_ERRORS;
    }
    return STAGE_OK;
}

/*
 * Writes one line of an output file: two fields separated by one TAB
 * (OUT-13).
 *   file   the file to write to
 *   left   the first field
 *   right  the second field
 */
static void write_pair(FILE *file, const char *left, const char *right)
{
    fprintf(file, LINE_FORMAT, left, right);
}

/*
 * Writes one line that holds a name and an address of four base-4 digits,
 * the line of the .ent and .ext files (OUT-8, OUT-9).
 *   file     the file to write to
 *   name     the name of the symbol
 *   address  the address
 */
static void write_symbol_line(FILE *file, const char *name, int address)
{
    char digits[BASE4_BUFFER_SIZE];

    base4_format((unsigned int)address, BASE4_ADDRESS_DIGITS, digits);
    write_pair(file, name, digits);
}

/*
 * Writes one line of the memory image: the address in four base-4 digits
 * and the word in five (OUT-4, OUT-5).
 *   file     the file to write to
 *   address  the address of the word
 *   word     the machine word
 */
static void write_word_line(FILE *file, int address, unsigned int word)
{
    char address_digits[BASE4_BUFFER_SIZE];
    char word_digits[BASE4_BUFFER_SIZE];

    base4_format((unsigned int)address, BASE4_ADDRESS_DIGITS, address_digits);
    base4_format(word, BASE4_WORD_DIGITS, word_digits);
    write_pair(file, address_digits, word_digits);
}

/*
 * Writes the object file: the two lengths without leading digits, then the
 * code image and right after it the data image, in ascending address order
 * from the load address (OUT-3, OUT-4, OUT-7, OUT-13, ALG-14).
 *   base_name  the name of the source file without the extension
 *   image      the memory image
 *   errors     the diagnostics context of the file
 * Returns the result of the stage, as write_output_files does.
 */
static StageResult write_object(const char *base_name,
                                const MemoryImage *image,
                                ErrorContext *errors)
{
    int code_count = memory_image_code_count(image);
    int data_count = memory_image_data_counter(image);
    char code_length[BASE4_BUFFER_SIZE];
    char data_length[BASE4_BUFFER_SIZE];
    FILE *file;
    int index;
    StageResult result =
        open_output(base_name, OBJECT_EXTENSION, errors, &file);

    if (result != STAGE_OK)
    {
        return result;
    }
    base4_format_shortest((unsigned int)code_count, code_length);
    base4_format_shortest((unsigned int)data_count, data_length);
    write_pair(file, code_length, data_length);
    for (index = 0; index < code_count; index++)
    {
        write_word_line(file, FIRST_LOAD_ADDRESS + index,
                        memory_image_code_word(image, index));
    }
    for (index = 0; index < data_count; index++)
    {
        write_word_line(file, FIRST_LOAD_ADDRESS + code_count + index,
                        memory_image_data_word(image, index));
    }
    return close_output(file, OBJECT_EXTENSION, errors);
}

/*
 * Counts one symbol when it is an entry; a SymbolVisitor.
 *   symbol    the symbol visited
 *   argument  the count of entry symbols so far, an int
 */
static void count_entry(const Symbol *symbol, void *argument)
{
    int *count = (int *)argument;

    if (symbol_is_entry(symbol))
    {
        (*count)++;
    }
}

/*
 * Writes the line of one symbol when it is an entry; a SymbolVisitor. The
 * symbol table holds every symbol once, so an entry symbol is written once
 * (DIR-34, OUT-8).
 *   symbol    the symbol visited
 *   argument  the open entries file, a FILE
 */
static void write_entry(const Symbol *symbol, void *argument)
{
    FILE *file = (FILE *)argument;

    if (symbol_is_entry(symbol))
    {
        write_symbol_line(file, symbol_name(symbol), symbol_address(symbol));
    }
}

/*
 * Writes the entries file, unless no symbol is an entry (OUT-2, OUT-8).
 *   base_name  the name of the source file without the extension
 *   symbols    the symbol table
 *   errors     the diagnostics context of the file
 * Returns the result of the stage, as write_output_files does.
 */
static StageResult write_entries(const char *base_name,
                                 const SymbolTable *symbols,
                                 ErrorContext *errors)
{
    int count = 0;
    FILE *file;
    StageResult result;

    /* The entries are counted first, so that an empty file is never made. */
    symbol_table_for_each(symbols, count_entry, &count);
    if (count == 0)
    {
        return STAGE_OK;
    }
    result = open_output(base_name, ENTRIES_EXTENSION, errors, &file);
    if (result != STAGE_OK)
    {
        return result;
    }
    symbol_table_for_each(symbols, write_entry, file);
    return close_output(file, ENTRIES_EXTENSION, errors);
}

/*
 * Writes the externals file, one line per use of an external symbol, unless
 * no external symbol is used (OUT-2, OUT-9, OUT-13).
 *   base_name  the name of the source file without the extension
 *   externals  the uses of external symbols
 *   errors     the diagnostics context of the file
 * Returns the result of the stage, as write_output_files does.
 */
static StageResult write_externals(const char *base_name,
                                   const ReferenceList *externals,
                                   ErrorContext *errors)
{
    int count = reference_list_count(externals);
    FILE *file;
    int index;
    StageResult result;

    if (count == 0)
    {
        return STAGE_OK;
    }
    result = open_output(base_name, EXTERNALS_EXTENSION, errors, &file);
    if (result != STAGE_OK)
    {
        return result;
    }
    for (index = 0; index < count; index++)
    {
        write_symbol_line(file, reference_list_name(externals, index),
                          reference_list_address(externals, index));
    }
    return close_output(file, EXTERNALS_EXTENSION, errors);
}

/* Removes the output files of a source file; see output_writer.h. */
Boolean remove_output_files(const char *base_name)
{
    static const char *const extensions[] = {
        OBJECT_EXTENSION, ENTRIES_EXTENSION, EXTERNALS_EXTENSION};
    Boolean memory_left = TRUE;
    char *file_name;
    size_t index;

    for (index = 0; index < ARRAY_LENGTH(extensions); index++)
    {
        file_name = util_make_file_name(base_name, extensions[index]);
        if (file_name == NULL)
        {
            /* The other files are still removed. */
            memory_left = FALSE;
            continue;
        }
        remove(file_name);
        free(file_name);
    }
    return memory_left;
}

/* Writes the output files of one source file; see output_writer.h. */
StageResult write_output_files(const char *base_name,
                               const MemoryImage *image,
                               const SymbolTable *symbols,
                               const ReferenceList *externals,
                               ErrorContext *errors)
{
    StageResult result = write_object(base_name, image, errors);

    if (result == STAGE_OK)
    {
        result = write_entries(base_name, symbols, errors);
    }
    if (result == STAGE_OK)
    {
        result = write_externals(base_name, externals, errors);
    }
    if (result != STAGE_OK)
    {
        /*
         * No incomplete output is left. The result is already a failure,
         * so a file name that could not be built does not change it.
         */
        remove_output_files(base_name);
    }
    return result;
}
