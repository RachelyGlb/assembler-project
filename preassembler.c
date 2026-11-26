/*
 * preassembler.c - the pre-assembler: macro expansion.
 *
 * The file is read line by line by a small state machine with three states:
 * outside a macro definition, inside a definition that is being recorded,
 * and inside a definition that is being discarded because its "mcro" line
 * was faulty (MAC-38). Every line is first classified by its first field,
 * or by its second field when the first one is a label (MAC-36), and then
 * handled by the function of the current state.
 *
 * A line gets at most one message (ERR-R16). After that message the line is
 * handled "quietly": it still opens or closes a definition, so that the
 * lines after it are read in the right state, but it reports nothing more,
 * is not copied and is not expanded (MAC-37).
 *
 * The .am file is written while the file is read and is removed at the end
 * if any error was found (MAC-23). See preassembler.h for the contract.
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_table.h"
#include "preassembler.h"
#include "util.h"

/* The character that a macro name may hold besides letters and digits. */
#define MACRO_NAME_EXTRA_CHARACTER '_'

/* Turns the value of a constant into a string literal, for messages. */
#define STRINGIFY_TOKEN(token) #token
#define STRINGIFY(constant) STRINGIFY_TOKEN(constant)

/* The byte that a source line must not contain (SYN-26). */
#define NUL_BYTE '\0'

/* The mode in which the .am file is opened. */
#define WRITE_MODE "w"

/* The state of the scan with respect to macro definitions. */
typedef enum
{
    STATE_OUTSIDE,   /* Not inside a macro definition. */
    STATE_RECORDING, /* Inside a definition; body lines are stored. */
    STATE_DISCARDING /* Inside a faulty definition; lines are dropped. */
} MacroState;

/* What a line is for the pre-assembler. */
typedef enum
{
    LINE_PLAIN,       /* Any other line, including comments and blanks. */
    LINE_MACRO_START, /* A "mcro" line. */
    LINE_MACRO_END,   /* A "mcroend" line. */
    LINE_MACRO_CALL   /* A line whose key field is a macro name. */
} LineKind;

/* One white-space-delimited field of a line; it is not zero-terminated. */
typedef struct
{
    const char *start; /* The first character of the field. */
    size_t length;     /* The number of characters; 0 when there is none. */
} Field;

/* The classification of one line. */
typedef struct
{
    LineKind kind;      /* What the line is. */
    Boolean labelled;   /* TRUE when a label stands in front (MAC-36). */
    const char *rest;   /* The text after the key field. */
    const Macro *macro; /* The macro called, for LINE_MACRO_CALL. */
} LineInfo;

/* Everything the scan of one source file needs. */
typedef struct
{
    FILE *output;         /* The .am file being written. */
    MacroTable *macros;   /* The macro table being filled. */
    LineMap *lines;       /* The line map being filled. */
    ErrorContext *errors; /* The diagnostics context of the file. */
    MacroState state;     /* The state of the scan. */
    Macro *current;       /* The macro being recorded, if any. */
    int line_number;      /* The number of the line being handled. */
} Scan;

/*
 * Finds the next field of a text.
 *   text   the text to look in
 *   field  receives the field; its length is 0 when only white space is
 *          left
 * Returns the position just after the field.
 */
static const char *read_field(const char *text, Field *field)
{
    const char *end;

    field->start = util_skip_spaces(text);
    end = util_skip_field(field->start);
    field->length = (size_t)(end - field->start);
    return end;
}

/*
 * Tells whether a field is exactly a given word.
 *   field  the field to compare
 *   word   the word, a string
 * Returns TRUE when they are equal, character for character.
 */
static Boolean field_is(const Field *field, const char *word)
{
    return (strlen(word) == field->length &&
            strncmp(field->start, word, field->length) == 0)
               ? TRUE
               : FALSE;
}

/*
 * Tells whether a field has the form of a label definition for MAC-36: it
 * ends with a colon. The text before the colon is not validated.
 *   field  the field to test
 * Returns TRUE when the last character of the field is a colon.
 */
static Boolean field_is_label(const Field *field)
{
    return (field->length > 0 &&
            field->start[field->length - 1] == LABEL_END_CHARACTER)
               ? TRUE
               : FALSE;
}

/*
 * Decides what a key field makes of its line (MAC-16 steps 2 and 3).
 *   macros  the macros defined so far
 *   field   the first field of the line, or the second after a label
 *   macro   receives the macro when the field is a macro name
 * Returns the kind of the line.
 */
static LineKind field_kind(const MacroTable *macros, const Field *field,
                           const Macro **macro)
{
    if (field_is(field, MACRO_START_KEYWORD))
    {
        return LINE_MACRO_START;
    }
    if (field_is(field, MACRO_END_KEYWORD))
    {
        return LINE_MACRO_END;
    }
    *macro = macro_table_find_text(macros, field->start, field->length);
    return (*macro != NULL) ? LINE_MACRO_CALL : LINE_PLAIN;
}

/*
 * Classifies one line. A comment line is always plain (SYN-5, SYN-9). A
 * line is "labelled" only when its first field ends with a colon and its
 * second field is "mcro", "mcroend" or a macro name (MAC-36).
 *   macros  the macros defined so far
 *   line    the line, without its terminator
 * Returns the classification.
 */
static LineInfo classify_line(const MacroTable *macros, const char *line)
{
    LineInfo info;
    Field field;

    info.kind = LINE_PLAIN;
    info.labelled = FALSE;
    info.rest = line;
    info.macro = NULL;
    if (line[0] == COMMENT_CHARACTER)
    {
        return info;
    }
    info.rest = read_field(line, &field);
    info.kind = field_kind(macros, &field, &info.macro);
    if (info.kind == LINE_PLAIN && field_is_label(&field))
    {
        info.rest = read_field(info.rest, &field);
        info.kind = field_kind(macros, &field, &info.macro);
        info.labelled = (info.kind != LINE_PLAIN) ? TRUE : FALSE;
    }
    return info;
}

/*
 * Writes one line to the .am file, always ending it with one newline
 * (MAC-29), and records its origin in the line map (MAC-35).
 *   scan         the scan
 *   text         the line, without a terminator
 *   source_line  the .as line number the text comes from
 * Returns FALSE when memory is exhausted.
 */
static Boolean write_line(Scan *scan, const char *text, int source_line)
{
    fputs(text, scan->output);
    putc('\n', scan->output);
    return line_map_append(scan->lines, source_line);
}

/*
 * Replaces a macro call by the body lines of the macro, each exactly as it
 * was written in the definition and mapped to its own line there (MAC-29,
 * MAC-35). The body is not examined, so nothing in it is expanded (MAC-32).
 *   scan   the scan
 *   macro  the macro that is called
 * Returns FALSE when memory is exhausted.
 */
static Boolean expand_macro(Scan *scan, const Macro *macro)
{
    int count = macro_line_count(macro);
    int index;

    for (index = 0; index < count; index++)
    {
        if (!write_line(scan, macro_line(macro, index),
                        macro_line_source(macro, index)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * Checks the syntax of a macro name (MAC-33): a letter, then letters,
 * digits and '_', at most MAX_NAME_LENGTH characters in all.
 *   name  the field that should be a macro name; not empty
 * Returns TRUE when the syntax is legal. Reserved words are not checked
 * here.
 */
static Boolean is_valid_macro_name(const Field *name)
{
    size_t index;
    unsigned char character;

    if (name->length > MAX_NAME_LENGTH ||
        !isalpha((unsigned char)name->start[0]))
    {
        return FALSE;
    }
    for (index = 1; index < name->length; index++)
    {
        character = (unsigned char)name->start[index];
        if (!isalnum(character) && character != MACRO_NAME_EXTRA_CHARACTER)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * Finds what is wrong with a "mcro" line whose name has legal syntax.
 *   scan   the scan
 *   name   the macro name, as a string
 *   after  the text that follows the name on the line
 * Returns the text of the error (ERR-2, ERR-8 or ERR-3), or NULL when the
 * line is correct.
 */
static const char *definition_fault(const Scan *scan, const char *name,
                                    const char *after)
{
    if (is_reserved_word(name))
    {
        return "the macro name is a reserved word of the language";
    }
    if (macro_table_find(scan->macros, name) != NULL)
    {
        return "a macro with this name is already defined";
    }
    if (!util_is_blank(after))
    {
        return "extra text after the macro name on the mcro line";
    }
    return NULL;
}

/*
 * Opens the definition of a macro whose name has legal syntax: enters the
 * macro into the table and starts recording, or reports the fault of the
 * line and leaves the definition to be discarded (MAC-38).
 *   scan   the scan; its state is STATE_DISCARDING on entry
 *   name   the macro name, as a string
 *   after  the text that follows the name on the line
 *   quiet  TRUE when the line has already had its message
 * Returns FALSE when memory is exhausted.
 */
static Boolean open_definition(Scan *scan, const char *name,
                               const char *after, Boolean quiet)
{
    const char *fault = definition_fault(scan, name, after);

    if (fault != NULL)
    {
        if (!quiet)
        {
            errors_report(scan->errors, scan->line_number, "%s", fault);
        }
        return TRUE;
    }
    scan->current = macro_table_add(scan->macros, name);
    if (scan->current == NULL)
    {
        return FALSE;
    }
    scan->state = STATE_RECORDING;
    return TRUE;
}

/*
 * Handles a "mcro" line. Whatever is wrong with the line, a definition is
 * open afterwards: a correct line starts recording, a faulty one starts
 * discarding up to the matching "mcroend" (MAC-38).
 *   scan   the scan
 *   rest   the text after the word "mcro"
 *   quiet  TRUE when the line has already had its message (MAC-37)
 * Returns FALSE when memory is exhausted.
 */
static Boolean start_definition(Scan *scan, const char *rest, Boolean quiet)
{
    Field name;
    const char *after = read_field(rest, &name);
    const char *fault = NULL;
    char *name_text;
    Boolean enough_memory;

    scan->state = STATE_DISCARDING;
    if (name.length == 0)
    {
        fault = "the macro name is missing after mcro";
    }
    else if (!is_valid_macro_name(&name))
    {
        fault = "invalid macro name: it must start with a letter, hold only "
                "letters, digits and '_', and have at most "
                STRINGIFY(MAX_NAME_LENGTH) " characters";
    }
    if (fault != NULL)
    {
        if (!quiet)
        {
            errors_report(scan->errors, scan->line_number, "%s", fault);
        }
        return TRUE;
    }
    name_text = util_copy_text(name.start, name.length);
    if (name_text == NULL)
    {
        return FALSE;
    }
    enough_memory = open_definition(scan, name_text, after, quiet);
    free(name_text);
    return enough_memory;
}

/*
 * Handles a line outside a macro definition.
 *   scan   the scan
 *   line   the line, without its terminator
 *   quiet  TRUE when the line has already had its message
 * Returns FALSE when memory is exhausted.
 */
static Boolean handle_outside(Scan *scan, const char *line, Boolean quiet)
{
    LineInfo info = classify_line(scan->macros, line);

    if (info.labelled && !quiet)
    {
        errors_report(scan->errors, scan->line_number,
                      "a label is not allowed in front of mcro, mcroend or "
                      "a macro call");
        quiet = TRUE;
    }
    if (info.kind == LINE_MACRO_START)
    {
        return start_definition(scan, info.rest, quiet);
    }
    if (quiet)
    {
        /* The line had its message: not copied and not expanded (MAC-37). */
        return TRUE;
    }
    if (info.kind == LINE_MACRO_CALL)
    {
        if (!util_is_blank(info.rest))
        {
            errors_report(scan->errors, scan->line_number,
                          "extra text after the macro name '%s' on a macro "
                          "call line", macro_name(info.macro));
            return TRUE;
        }
        return expand_macro(scan, info.macro);
    }
    /* A plain line, or a "mcroend" that closes nothing, is copied as is. */
    return write_line(scan, line, scan->line_number);
}

/*
 * Handles a line inside a definition that is being recorded: the "mcroend"
 * line closes the definition (also when it is faulty, MAC-31, MAC-37);
 * every other line is stored as a body line without being examined
 * (MAC-24).
 *   scan   the scan
 *   line   the line, without its terminator
 *   quiet  TRUE when the line has already had its message
 * Returns FALSE when memory is exhausted.
 */
static Boolean handle_recording(Scan *scan, const char *line, Boolean quiet)
{
    LineInfo info = classify_line(scan->macros, line);

    if (info.kind != LINE_MACRO_END)
    {
        return macro_add_line(scan->current, line, scan->line_number);
    }
    scan->state = STATE_OUTSIDE;
    scan->current = NULL;
    if (quiet)
    {
        return TRUE;
    }
    if (info.labelled)
    {
        errors_report(scan->errors, scan->line_number,
                      "a label is not allowed in front of mcroend");
    }
    else if (!util_is_blank(info.rest))
    {
        errors_report(scan->errors, scan->line_number,
                      "extra text after mcroend");
    }
    return TRUE;
}

/*
 * Handles a line inside a definition that is being discarded after a faulty
 * "mcro" line: nothing is stored and nothing is reported; the "mcroend"
 * line, with or without a label, ends the discarding (MAC-38).
 *   scan  the scan
 *   line  the line, without its terminator
 */
static void handle_discarding(Scan *scan, const char *line)
{
    LineInfo info = classify_line(scan->macros, line);

    if (info.kind == LINE_MACRO_END)
    {
        scan->state = STATE_OUTSIDE;
    }
}

/*
 * Handles one line of the source file according to the state of the scan.
 * The length of the line is checked first (ERR-10) and then its bytes
 * (ERR-46), except for the lines that are discarded (MAC-38). The length is
 * the true one, NUL bytes included, and a line that is too long gets only
 * the message about its length (SYN-26, ERR-R16). A line with a NUL byte is
 * then handled quietly like a line that is too long; its fields are those
 * of the text in front of the first NUL byte.
 *   scan    the scan
 *   line    the line, without its terminator
 *   length  the number of bytes of the line, NUL bytes included
 * Returns FALSE when memory is exhausted.
 */
static Boolean handle_line(Scan *scan, const char *line, size_t length)
{
    Boolean quiet = FALSE;

    if (scan->state == STATE_DISCARDING)
    {
        handle_discarding(scan, line);
        return TRUE;
    }
    if (length > MAX_SOURCE_LINE_LENGTH)
    {
        errors_report(scan->errors, scan->line_number,
                      "the line is longer than %d characters",
                      MAX_SOURCE_LINE_LENGTH);
        quiet = TRUE;
    }
    else if (memchr(line, NUL_BYTE, length) != NULL)
    {
        errors_report(scan->errors, scan->line_number,
                      "the line contains a NUL byte");
        quiet = TRUE;
    }
    if (scan->state == STATE_RECORDING)
    {
        return handle_recording(scan, line, quiet);
    }
    return handle_outside(scan, line, quiet);
}

/*
 * Reads the whole source file and handles every line. A source file that
 * cannot be read is reported here, without a line number (CLI-12, ERR-47).
 *   scan    the scan
 *   source  the .as file
 * Returns FALSE when memory is exhausted.
 */
static Boolean scan_source(Scan *scan, FILE *source)
{
    char *line = NULL;
    size_t capacity = 0;
    size_t length;
    Boolean enough_memory = TRUE;
    ReadStatus status = util_read_line(source, &line, &capacity, &length,
                                       TRUE);

    while (status == READ_LINE && enough_memory)
    {
        enough_memory = handle_line(scan, line, length);
        scan->line_number++;
        if (enough_memory)
        {
            status = util_read_line(source, &line, &capacity, &length,
                                    TRUE);
        }
    }
    free(line);
    if (status == READ_ERROR)
    {
        errors_report(scan->errors, NO_LINE, "cannot read the source file");
    }
    return (enough_memory && status != READ_NO_MEMORY) ? TRUE : FALSE;
}

/*
 * Closes the .am file and decides the result of the stage. The file is
 * removed unless the scan was completed without any error (MAC-23).
 *   scan          the scan
 *   am_file_name  the name of the .am file
 *   completed     FALSE when the scan stopped because memory ran out
 * Returns the result of the pre-assembler.
 */
static StageResult finish_output(Scan *scan, const char *am_file_name,
                                 Boolean completed)
{
    Boolean write_failed = ferror(scan->output) ? TRUE : FALSE;

    if (fclose(scan->output) != 0)
    {
        write_failed = TRUE;
    }
    if (completed && write_failed)
    {
        errors_report(scan->errors, NO_LINE, "cannot write the file %s",
                      am_file_name);
    }
    if (completed && errors_count(scan->errors) == 0)
    {
        return STAGE_OK;
    }
    remove(am_file_name);
    return completed ? STAGE_ERRORS : STAGE_FATAL;
}

/* Runs the pre-assembler on one source file; see preassembler.h. */
StageResult preassemble(FILE *source, const char *am_file_name,
                        MacroTable *macros, LineMap *lines,
                        ErrorContext *errors)
{
    Scan scan;
    Boolean completed;

    scan.output = fopen(am_file_name, WRITE_MODE);
    if (scan.output == NULL)
    {
        errors_report(errors, NO_LINE, "cannot create the file %s",
                      am_file_name);
        return STAGE_ERRORS;
    }
    scan.macros = macros;
    scan.lines = lines;
    scan.errors = errors;
    scan.state = STATE_OUTSIDE;
    scan.current = NULL;
    scan.line_number = FIRST_LINE_NUMBER;
    completed = scan_source(&scan, source);
    return finish_output(&scan, am_file_name, completed);
}
