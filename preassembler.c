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
#include "preassembler.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_table.h"
#include "util.h"

/* The character that a macro name may hold besides letters and digits. */
#define MACRO_NAME_EXTRA_CHARACTER '_'

/* The byte that a source line must not contain (SYN-26). */
#define NUL_BYTE '\0'

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
    putc(NEWLINE_CHARACTER, scan->output);
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
 * Tells whether a "mcro" line whose name has legal syntax is faulty, and
 * reports the fault: a reserved word (ERR-2), a name defined before (ERR-8)
 * or text after the name (ERR-3), in this order.
 *   scan   the scan
 *   name   the macro name, as a string
 *   after  the text that follows the name on the line
 *   quiet  TRUE when the line has already had its message
 * Returns TRUE when the line is faulty.
 */
static Boolean definition_is_faulty(const Scan *scan, const char *name,
                                    const char *after, Boolean quiet)
{
    Boolean reserved = is_reserved_word(name);
    Boolean defined = (macro_table_find(scan->macros, name) != NULL) ? TRUE
                                                                     : FALSE;
    Boolean followed = util_is_blank(after) ? FALSE : TRUE;

    if (quiet)
    {
        /* The line keeps the one message that it already has (ERR-R16). */
    }
    else if (reserved)
    {
        errors_report(scan->errors, scan->line_number,
                      "'%s' is a reserved word and cannot be a macro name",
                      name);
    }
    else if (defined)
    {
        errors_report(scan->errors, scan->line_number,
                      "the macro '%s' is already defined", name);
    }
    else if (followed)
    {
        errors_report(scan->errors, scan->line_number,
                      "unexpected text after the macro name '%s' on the '"
                      MACRO_START_KEYWORD "' line",
                      name);
    }
    return (reserved || defined || followed) ? TRUE : FALSE;
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
    if (definition_is_faulty(scan, name, after, quiet))
    {
        return TRUE;
    }
    /* The name is new: definition_is_faulty has looked it up in the table. */
    scan->current = macro_table_add(scan->macros, name);
    if (scan->current == NULL)
    {
        return FALSE;
    }
    scan->state = STATE_RECORDING;
    return TRUE;
}

/*
 * Tells whether the name field of a "mcro" line is faulty, and reports the
 * fault: no name at all (ERR-5) or a name with illegal syntax (ERR-6).
 *   scan   the scan
 *   name   the field after the word "mcro"; empty when there is none
 *   quiet  TRUE when the line has already had its message
 * Returns TRUE when the field is faulty.
 */
static Boolean name_is_faulty(const Scan *scan, const Field *name,
                              Boolean quiet)
{
    Boolean missing = (name->length == 0) ? TRUE : FALSE;
    Boolean malformed = (!missing && !is_valid_macro_name(name)) ? TRUE
                                                                 : FALSE;

    if (missing && !quiet)
    {
        errors_report(scan->errors, scan->line_number,
                      "the macro name is missing after '" MACRO_START_KEYWORD
                      "'");
    }
    else if (malformed && !quiet)
    {
        errors_report(scan->errors, scan->line_number,
                      "'%.*s' is not a legal macro name; a macro name is a "
                      "letter followed by letters, digits and '%c', at most "
                      "%d characters in all",
                      (int)name->length, name->start,
                      MACRO_NAME_EXTRA_CHARACTER, MAX_NAME_LENGTH);
    }
    return (missing || malformed) ? TRUE : FALSE;
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
    char *name_text;
    Boolean enough_memory;

    scan->state = STATE_DISCARDING;
    if (name_is_faulty(scan, &name, quiet))
    {
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
 * Reports a label in front of "mcro", in front of "mcroend" or in front of
 * a macro call (ERR-9), with the text of the one that the line has.
 *   scan  the scan
 *   info  the classification of the line; its labelled member is TRUE
 */
static void report_labelled_line(const Scan *scan, const LineInfo *info)
{
    switch (info->kind)
    {
    case LINE_MACRO_START:
        errors_report(scan->errors, scan->line_number,
                      "a label is not allowed in front of '"
                      MACRO_START_KEYWORD "'");
        break;
    case LINE_MACRO_END:
        errors_report(scan->errors, scan->line_number,
                      "a label is not allowed in front of '"
                      MACRO_END_KEYWORD "'");
        break;
    default:
        /* Only a macro call is left: a plain line is never labelled. */
        errors_report(scan->errors, scan->line_number,
                      "a label is not allowed in front of the call of the "
                      "macro '%s'",
                      macro_name(info->macro));
        break;
    }
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
        report_labelled_line(scan, &info);
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
                          "unexpected text after the call of the macro '%s'",
                          macro_name(info.macro));
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
        report_labelled_line(scan, &info);
    }
    else if (!util_is_blank(info.rest))
    {
        errors_report(scan->errors, scan->line_number,
                      "unexpected text after '" MACRO_END_KEYWORD "'");
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
        errors_report(scan->errors, NO_LINE, "cannot read the '%s' file",
                      SOURCE_EXTENSION);
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
        errors_report(scan->errors, NO_LINE, "cannot write the '%s' file",
                      EXPANDED_EXTENSION);
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
        errors_report(errors, NO_LINE, "cannot create the '%s' file",
                      EXPANDED_EXTENSION);
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
