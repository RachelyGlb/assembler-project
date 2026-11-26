/*
 * syntax.c - the parser of one line of the expanded source file.
 *
 * The parser is a set of small recursive-descent functions over the text of
 * the line. Nothing is split in place and nothing is tokenised twice: every
 * function receives the position at which it starts and returns the
 * position at which it stopped, or NULL (or FALSE) after it reported an
 * error. A Parser carries what all of them need. The first error ends the
 * parse (ERR-R16); the statement is then marked as not valid.
 *
 * The statement is filled and asked through the functions of statement.h
 * only; its structure is not known here.
 *
 * See syntax.h for the contract of the public function.
 */
#include "syntax.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "instruction_table.h"
#include "util.h"

/* The characters of the assembly language that the parser looks for. */
#define DIRECTIVE_CHARACTER '.'
#define IMMEDIATE_CHARACTER '#'
#define OPERAND_SEPARATOR ','
#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'
#define STRING_QUOTE '"'
#define PLUS_SIGN '+'
#define MINUS_SIGN '-'

/* The base of every number of the language (SYN-20). */
#define DECIMAL_RADIX 10

/*
 * The magnitude at which a number being read stops growing. It lies above
 * every legal value, so a longer number is still out of range, and far
 * below the limit of a long, so no input can overflow.
 */
#define NUMBER_CLAMP 100000L

/* The signed range of an immediate operand, eight bits (SYN-21). */
#define IMMEDIATE_MAX ((1L << (OPERAND_FIELD_BITS - 1)) - 1)
#define IMMEDIATE_MIN (-(1L << (OPERAND_FIELD_BITS - 1)))

/* The signed range of a data word, ten bits (SYN-21). */
#define DATA_VALUE_MAX ((1L << (WORD_BITS - 1)) - 1)
#define DATA_VALUE_MIN (-(1L << (WORD_BITS - 1)))

/*
 * The value at which a matrix dimension is capped. A matrix with a larger
 * dimension cannot fit in memory anyway (ALG-15), and the cap keeps the
 * product of two dimensions inside every int.
 */
#define DIMENSION_CLAMP (MAX_PROGRAM_WORDS + 1)

/* The first and the last character that a string may hold (SYN-22). */
#define FIRST_STRING_CHARACTER 32
#define LAST_STRING_CHARACTER 126

/* The word that ends the characters of a string in memory (DIR-10). */
#define STRING_TERMINATOR 0

/* What a name is on its line, for the messages about names. */
#define ROLE_LABEL "label"
#define ROLE_OPERAND "operand"

/* The numbers of the first and of the last register, for a message (M-2). */
#define FIRST_REGISTER_NUMBER 0
#define LAST_REGISTER_NUMBER (REGISTER_COUNT - 1)

/* What is wrong with a candidate label name, if anything. */
typedef enum
{
    NAME_LEGAL,      /* The name may be a label. */
    NAME_MALFORMED,  /* Not a letter followed by letters and digits. */
    NAME_TOO_LONG,   /* More than MAX_NAME_LENGTH characters. */
    NAME_RESERVED    /* A reserved word (LBL-20). */
} NameStatus;

/*
 * What an operand of each addressing mode is, in the words of a message;
 * the index is the AddressingMode (ADR-1).
 */
static const char *const mode_descriptions[ADDRESSING_MODE_COUNT] = {
    "an immediate value", "a label", "a matrix element", "a register"};

/* What every parsing function needs. */
typedef struct
{
    Statement *statement; /* The statement being filled. */
    ErrorContext *errors; /* The diagnostics context of the file. */
    int line_number;      /* The .as line number, for the message. */
    Boolean no_memory;    /* TRUE once an allocation failed. */
} Parser;

/*
 * The type of a function that parses the operands of a directive.
 *   parser  the parser of the line
 *   text    the text after the directive name
 */
typedef void (*DirectiveParser)(Parser *parser, const char *text);

/* Everything the parser knows about one directive. */
typedef struct
{
    DirectiveKind directive; /* The directive. */
    StatementKind kind;      /* The kind of statement it makes. */
    DirectiveParser parse;   /* The function that parses its operands. */
    Boolean ignores_label;   /* TRUE when a label in front is ignored. */
} DirectiveRule;

/*
 * Reports the syntax error of the line and marks the statement as not
 * valid. A piece of the line is quoted with "%.*s", its length (an int)
 * and its first character.
 *   parser  the parser of the line
 *   format  a printf format for the message, in the style of errors.h,
 *           followed by its arguments
 * Returns FALSE, so that a caller can write "return fail(...)".
 */
static Boolean fail(Parser *parser, const char *format, ...)
    ERRORS_PRINTF_FORMAT(2, 3);
static Boolean fail(Parser *parser, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    errors_report_list(parser->errors, parser->line_number, format,
                       arguments);
    va_end(arguments);
    statement_mark_invalid(parser->statement);
    return FALSE;
}

/*
 * Copies a piece of the line to the heap, as a string.
 *   parser  the parser of the line; it notes an allocation failure
 *   start   the first character of the piece
 *   end     the position just after the piece
 * Returns the copy, or NULL when memory is exhausted.
 */
static char *copy_piece(Parser *parser, const char *start, const char *end)
{
    char *copy = util_copy_text(start, (size_t)(end - start));

    if (copy == NULL)
    {
        parser->no_memory = TRUE;
    }
    return copy;
}

/*
 * Tells whether a character ends a token of an operand list: the end of the
 * line, white space or a comma.
 *   character  the character to test
 * Returns TRUE for such a character.
 */
static Boolean ends_token(char character)
{
    return (character == '\0' || util_is_space(character) ||
            character == OPERAND_SEPARATOR)
               ? TRUE
               : FALSE;
}

/*
 * Finds the end of a token of an operand list.
 *   text  the first character of the token
 * Returns the first position at which ends_token holds.
 */
static const char *token_end(const char *text)
{
    while (!ends_token(*text))
    {
        text++;
    }
    return text;
}

/*
 * Examines a candidate label name (LBL-2, LBL-3, LBL-20).
 *   name  the candidate, a string
 * Returns NAME_LEGAL or what is wrong with the name.
 */
static NameStatus check_name(const char *name)
{
    size_t index;

    if (!isalpha((unsigned char)name[0]))
    {
        return NAME_MALFORMED;
    }
    for (index = 1; name[index] != '\0'; index++)
    {
        if (!isalnum((unsigned char)name[index]))
        {
            return NAME_MALFORMED;
        }
    }
    if (index > MAX_NAME_LENGTH)
    {
        return NAME_TOO_LONG;
    }
    return is_reserved_word(name) ? NAME_RESERVED : NAME_LEGAL;
}

/*
 * Checks that a name is a legal label name and reports it when it is not
 * (ERR-11, ERR-12, ERR-13, ERR-34).
 *   parser  the parser of the line
 *   name    the name, a string
 *   role    what the name is on the line, "label" or "operand", for the
 *           message
 * Returns TRUE when the name is legal.
 */
static Boolean require_name(Parser *parser, const char *name,
                            const char *role)
{
    switch (check_name(name))
    {
    case NAME_LEGAL:
        return TRUE;
    case NAME_TOO_LONG:
        return fail(parser,
                    "the %s '%s' is longer than %d characters, the limit of "
                    "a label name",
                    role, name, MAX_NAME_LENGTH);
    case NAME_RESERVED:
        return fail(parser,
                    "the %s '%s' is a reserved word and cannot be a label "
                    "name",
                    role, name);
    default:
        return fail(parser,
                    "the %s '%s' is not a legal label name; a label name is "
                    "a letter followed by letters and digits",
                    role, name);
    }
}

/*
 * Reads a decimal integer with an optional sign that fills a piece of the
 * line completely (SYN-20). A magnitude beyond NUMBER_CLAMP is not read
 * exactly, but stays beyond every legal range.
 *   start  the first character of the piece
 *   end    the position just after the piece
 *   value  receives the number
 * Returns FALSE when the piece is not a legal integer.
 */
static Boolean read_integer(const char *start, const char *end, long *value)
{
    const char *position = start;
    Boolean negative = FALSE;
    long magnitude = 0;

    if (position < end && (*position == PLUS_SIGN || *position == MINUS_SIGN))
    {
        negative = (*position == MINUS_SIGN) ? TRUE : FALSE;
        position++;
    }
    if (position == end)
    {
        return FALSE;
    }
    for (; position < end; position++)
    {
        if (!isdigit((unsigned char)*position))
        {
            return FALSE;
        }
        if (magnitude < NUMBER_CLAMP)
        {
            magnitude = magnitude * DECIMAL_RADIX + (*position - '0');
        }
    }
    *value = negative ? -magnitude : magnitude;
    return TRUE;
}

/*
 * Appends one value to the data words of the statement.
 *   parser  the parser of the line; it notes an allocation failure
 *   value   the value to append
 * Returns FALSE when memory is exhausted.
 */
static Boolean append_value(Parser *parser, int value)
{
    if (!statement_add_data_value(parser->statement, value))
    {
        parser->no_memory = TRUE;
        return FALSE;
    }
    return TRUE;
}

/*
 * Parses one value of a number list: a legal integer in the range of a data
 * word (ERR-29, ERR-39), where a value is expected (ERR-30).
 *   parser  the parser of the line
 *   text    the position at which the value is expected
 * Returns the position just after the value, or NULL after an error.
 */
static const char *parse_list_value(Parser *parser, const char *text)
{
    const char *start = util_skip_spaces(text);
    const char *end = token_end(start);
    long value;

    if (*start == OPERAND_SEPARATOR)
    {
        fail(parser, "unexpected comma where a number is expected");
        return NULL;
    }
    if (*start == '\0')
    {
        fail(parser, "a number is missing after the last comma");
        return NULL;
    }
    if (!read_integer(start, end, &value))
    {
        fail(parser, "'%.*s' is not a legal integer", (int)(end - start),
             start);
        return NULL;
    }
    if (value < DATA_VALUE_MIN || value > DATA_VALUE_MAX)
    {
        fail(parser,
             "the value '%.*s' is out of range; a data word holds %ld to "
             "%ld",
             (int)(end - start), start, DATA_VALUE_MIN, DATA_VALUE_MAX);
        return NULL;
    }
    return append_value(parser, (int)value) ? end : NULL;
}

/*
 * Parses a list of integers separated by single commas (DIR-5, DIR-6) and
 * stores the values in the statement.
 *   parser       the parser of the line
 *   text         the position at which the list starts
 *   allow_empty  TRUE when the list may be absent (.mat, DIR-13)
 * Returns FALSE after an error.
 */
static Boolean parse_number_list(Parser *parser, const char *text,
                                 Boolean allow_empty)
{
    const char *position = util_skip_spaces(text);

    if (*position == '\0')
    {
        return allow_empty ? TRUE
                           : fail(parser, "the list of numbers is missing "
                                          "after the directive");
    }
    for (;;)
    {
        position = parse_list_value(parser, position);
        if (position == NULL)
        {
            return FALSE;
        }
        position = util_skip_spaces(position);
        if (*position == '\0')
        {
            return TRUE;
        }
        if (*position != OPERAND_SEPARATOR)
        {
            return fail(parser, "a comma is missing between two numbers");
        }
        position++;
    }
}

/*
 * Parses the operands of .data: every value of the list is one data word
 * (DIR-5 to DIR-7, ERR-28 to ERR-30).
 *   parser  the parser of the line
 *   text    the text after the directive name
 */
static void parse_data(Parser *parser, const char *text)
{
    parse_number_list(parser, text, FALSE);
}

/*
 * Stores the characters of a string and the terminating zero as data words
 * (DIR-10) and checks that every character is printable ASCII (ERR-32).
 *   parser  the parser of the line
 *   start   the first character of the string, after the opening quote
 *   end     the position of the closing quote
 * Returns FALSE after an error.
 */
static Boolean store_string(Parser *parser, const char *start,
                            const char *end)
{
    const char *position;
    unsigned char character;

    for (position = start; position < end; position++)
    {
        character = (unsigned char)*position;
        if (character < FIRST_STRING_CHARACTER ||
            character > LAST_STRING_CHARACTER)
        {
            return fail(parser,
                        "the string contains a character with code %d, "
                        "which is not printable ASCII (codes %d to %d)",
                        (int)character, FIRST_STRING_CHARACTER,
                        LAST_STRING_CHARACTER);
        }
        if (!append_value(parser, (int)character))
        {
            return FALSE;
        }
    }
    return append_value(parser, STRING_TERMINATOR);
}

/*
 * Parses the operand of .string (DIR-9, SYN-22, ERR-31). The string starts
 * at the first quotation mark and ends at the last one of the line; every
 * character between them belongs to it.
 *   parser  the parser of the line
 *   text    the text after the directive name
 */
static void parse_string(Parser *parser, const char *text)
{
    const char *open = util_skip_spaces(text);
    const char *close = strrchr(open, STRING_QUOTE);

    if (*open != STRING_QUOTE)
    {
        fail(parser, "the opening quotation mark of the string is missing");
        return;
    }
    if (close == open)
    {
        fail(parser, "the closing quotation mark of the string is missing");
        return;
    }
    if (!util_is_blank(close + 1))
    {
        fail(parser, "unexpected text after the closing quotation mark of "
                     "the string");
        return;
    }
    store_string(parser, open + 1, close);
}

/*
 * Parses one dimension of .mat, "[" digits "]", with optional white space
 * inside the brackets (DIR-27, DIR-33, ERR-33, ERR-44).
 *   parser     the parser of the line
 *   text       the position at which the opening bracket is expected
 *   dimension  receives the value, 1 to DIMENSION_CLAMP
 * Returns the position just after the closing bracket, or NULL after an
 * error.
 */
static const char *parse_dimension(Parser *parser, const char *text,
                                   int *dimension)
{
    const char *start;
    const char *end;
    long value;

    if (*text != OPEN_BRACKET)
    {
        fail(parser, "a matrix dimension in square brackets is missing; two "
                     "are expected, the second right after the first");
        return NULL;
    }
    start = util_skip_spaces(text + 1);
    if (*start == PLUS_SIGN || *start == MINUS_SIGN)
    {
        fail(parser, "a matrix dimension has a sign; a positive number "
                     "without a sign is expected");
        return NULL;
    }
    for (end = start; isdigit((unsigned char)*end); end++)
    {
    }
    if (!read_integer(start, end, &value) ||
        *util_skip_spaces(end) != CLOSE_BRACKET)
    {
        fail(parser, "a matrix dimension is not a number enclosed in square "
                     "brackets");
        return NULL;
    }
    if (value == 0)
    {
        fail(parser, "a matrix dimension is zero; a positive number is "
                     "expected");
        return NULL;
    }
    *dimension = (value > DIMENSION_CLAMP) ? DIMENSION_CLAMP : (int)value;
    return util_skip_spaces(end) + 1;
}

/*
 * Parses the operands of .mat: two dimensions and an optional value list
 * (DIR-12, DIR-24, DIR-25, ERR-40).
 *   parser  the parser of the line
 *   text    the text after the directive name
 */
static void parse_mat(Parser *parser, const char *text)
{
    Statement *statement = parser->statement;
    const char *position = util_skip_spaces(text);
    int rows;
    int columns;
    int listed;

    position = parse_dimension(parser, position, &rows);
    if (position == NULL)
    {
        return;
    }
    /* The second bracket must touch the first (DIR-27). */
    position = parse_dimension(parser, position, &columns);
    if (position == NULL || !parse_number_list(parser, position, TRUE))
    {
        return;
    }
    listed = statement_data_count(statement);
    if (listed > rows * columns)
    {
        fail(parser, "the matrix has %d values but room for only %d", listed,
             rows * columns);
        return;
    }
    /* The cells without a listed value are zero (DIR-13, DIR-24). */
    statement_extend_data(statement, rows * columns);
}

/*
 * Parses the operand of .entry or .extern: exactly one legal label name
 * (DIR-17, DIR-20, ERR-34).
 *   parser  the parser of the line
 *   text    the text after the directive name
 */
static void parse_symbol_operand(Parser *parser, const char *text)
{
    const char *start = util_skip_spaces(text);
    const char *end = util_skip_field(start);
    char *name;

    if (start == end)
    {
        fail(parser, "the operand of the directive is missing; one label "
                     "name is expected");
        return;
    }
    name = copy_piece(parser, start, end);
    if (name == NULL)
    {
        return;
    }
    /* The statement owns the name from here on and frees it. */
    statement_set_directive_symbol(parser->statement, name);
    if (!require_name(parser, name, ROLE_OPERAND))
    {
        return;
    }
    if (!util_is_blank(end))
    {
        fail(parser, "unexpected text after the operand '%s'; the directive "
                     "takes one label name",
             name);
    }
}

/*
 * Tells whether a piece of the line holds a character that is not a letter.
 *   start  the first character of the piece
 *   end    the position just after the piece
 * Returns TRUE when it does; FALSE for an empty piece.
 */
static Boolean has_non_letter(const char *start, const char *end)
{
    const char *position;

    for (position = start; position < end; position++)
    {
        if (!isalpha((unsigned char)*position))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Finds the longest directive name with which a longer word begins.
 *   start  the first character of the word, the dot
 *   end    the position just after the word
 * Returns the length of that name, the dot included, or 0 when the word
 * begins with no directive name.
 */
static size_t directive_prefix_length(const char *start, const char *end)
{
    size_t length = (size_t)(end - start);

    while (length > 0)
    {
        length--;
        if (directive_find_text(start, length) != DIRECTIVE_NONE)
        {
            return length;
        }
    }
    return 0;
}

/*
 * Reports a first field that starts with a dot and is not a directive: a
 * directive glued to its operand (SYN-17, ERR-36) or an unknown directive
 * (ERR-18). The rule: the field is a glued directive when it begins with a
 * directive name, the longest one that fits, and either what follows the
 * name in the field holds a character that is not a letter, or the field is
 * the last of the line, so that the operand can only be the part stuck to
 * the name. So ".data5" and ".externlabel" are a directive with its operand
 * stuck to it, while ".strings" in front of an operand is a wrong name: an
 * unknown directive.
 *   parser  the parser of the line
 *   start   the first character of the field, the dot
 *   end     the position just after the field
 */
static void report_unknown_directive(Parser *parser, const char *start,
                                     const char *end)
{
    size_t name_length = directive_prefix_length(start, end);

    if (name_length > 0 &&
        (has_non_letter(start + name_length, end) || util_is_blank(end)))
    {
        fail(parser, "white space is missing after the directive name '%.*s'",
             (int)name_length, start);
        return;
    }
    fail(parser, "unknown directive '%.*s'", (int)(end - start), start);
}

/*
 * The directives of the parser, one row each (DIR-4). A label in front of
 * .entry or .extern is ignored (DIR-35).
 */
static const DirectiveRule directive_rules[] = {
    {DIRECTIVE_DATA, STATEMENT_DATA, parse_data, FALSE},
    {DIRECTIVE_STRING, STATEMENT_STRING, parse_string, FALSE},
    {DIRECTIVE_MAT, STATEMENT_MAT, parse_mat, FALSE},
    {DIRECTIVE_ENTRY, STATEMENT_ENTRY, parse_symbol_operand, TRUE},
    {DIRECTIVE_EXTERN, STATEMENT_EXTERN, parse_symbol_operand, TRUE}};

/*
 * Finds the row of a directive whose name is a piece of the line.
 *   start  the first character of the name, the dot
 *   end    the position just after the name
 * Returns the row, or NULL when the piece is not the name of a directive.
 */
static const DirectiveRule *find_directive_rule(const char *start,
                                                const char *end)
{
    DirectiveKind directive =
        directive_find_text(start, (size_t)(end - start));
    size_t index;

    for (index = 0; index < ARRAY_LENGTH(directive_rules); index++)
    {
        if (directive_rules[index].directive == directive)
        {
            return &directive_rules[index];
        }
    }
    return NULL;
}

/*
 * Parses a directive statement (DIR-1).
 *   parser  the parser of the line
 *   start   the first character of the directive name, the dot
 */
static void parse_directive(Parser *parser, const char *start)
{
    const char *end = util_skip_field(start);
    const DirectiveRule *rule = find_directive_rule(start, end);

    if (rule == NULL)
    {
        report_unknown_directive(parser, start, end);
        return;
    }
    statement_set_kind(parser->statement, rule->kind);
    rule->parse(parser, end);
}

/*
 * Parses an immediate operand: '#' and, attached to it, a legal integer in
 * the range of eight bits (ADR-2, ERR-24, ERR-39), and adds it to the
 * statement.
 *   parser  the parser of the line
 *   text    the position of the '#'
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_immediate(Parser *parser, const char *text)
{
    const char *end = token_end(text);
    long value;

    if (!read_integer(text + 1, end, &value))
    {
        fail(parser,
             "the immediate operand '%.*s' is not '%c' followed by a legal "
             "integer",
             (int)(end - text), text, IMMEDIATE_CHARACTER);
        return NULL;
    }
    if (value < IMMEDIATE_MIN || value > IMMEDIATE_MAX)
    {
        fail(parser,
             "the immediate operand '%.*s' is out of range; an immediate "
             "operand holds %ld to %ld",
             (int)(end - text), text, IMMEDIATE_MIN, IMMEDIATE_MAX);
        return NULL;
    }
    statement_add_immediate_operand(parser->statement, (int)value);
    return end;
}

/*
 * Parses one index of a matrix operand: '[', a register, ']', with optional
 * white space inside the brackets (ADR-6, SYN-18, ERR-23, ERR-25).
 *   parser    the parser of the line
 *   text      the position of the opening bracket
 *   register_ receives the number of the register
 * Returns the position just after the closing bracket, or NULL after an
 * error.
 */
static const char *parse_index(Parser *parser, const char *text,
                               int *register_)
{
    const char *start = util_skip_spaces(text + 1);
    const char *close = strchr(text, CLOSE_BRACKET);
    const char *end = close;

    if (close == NULL)
    {
        fail(parser, "the closing square bracket of a matrix index is "
                     "missing");
        return NULL;
    }
    while (end > start && util_is_space(end[-1]))
    {
        end--;
    }
    *register_ = register_number_text(start, (size_t)(end - start));
    if (*register_ == NOT_A_REGISTER)
    {
        fail(parser,
             "the matrix index '%.*s' is not a register; one of 'r%d' to "
             "'r%d' is expected",
             (int)(end - start), start, FIRST_REGISTER_NUMBER,
             LAST_REGISTER_NUMBER);
        return NULL;
    }
    return close + 1;
}

/*
 * Parses the two indices of a matrix operand, after its label (ADR-6,
 * SYN-18, ERR-25, ERR-37).
 *   parser  the parser of the line
 *   text    the position of the first opening bracket
 *   row     receives the number of the register of the first index
 *   column  receives the number of the register of the second index
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_indices(Parser *parser, const char *text, int *row,
                                 int *column)
{
    const char *position = parse_index(parser, text, row);

    if (position == NULL)
    {
        return NULL;
    }
    if (*position != OPEN_BRACKET)
    {
        fail(parser, (*util_skip_spaces(position) == OPEN_BRACKET)
                         ? "white space is not allowed between the two "
                           "indices of a matrix operand"
                         : "the second index of a matrix operand is "
                           "missing; two indices in square brackets are "
                           "expected");
        return NULL;
    }
    position = parse_index(parser, position, column);
    if (position == NULL)
    {
        return NULL;
    }
    if (!ends_token(*position))
    {
        fail(parser, "unexpected text after the second index of a matrix "
                     "operand");
        return NULL;
    }
    return position;
}

/*
 * Parses an operand that names a label, alone (ADR-4) or with two indices
 * (ADR-6), and adds it to the statement.
 *   parser  the parser of the line
 *   name    the name as written, a string on the heap; the statement owns
 *           it after a success, and it stays the caller's after an error
 *   end     the position just after the name in the line
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_label_operand(Parser *parser, char *name,
                                       const char *end)
{
    int row;
    int column;

    if (!require_name(parser, name, ROLE_OPERAND))
    {
        return NULL;
    }
    if (*end != OPEN_BRACKET)
    {
        statement_add_direct_operand(parser->statement, name);
        return end;
    }
    end = parse_indices(parser, end, &row, &column);
    if (end != NULL)
    {
        statement_add_matrix_operand(parser->statement, name, row, column);
    }
    return end;
}

/*
 * Parses an operand that starts with a name: a register (ADR-8), a label
 * (ADR-4) or a label with two indices (ADR-6), and adds it to the
 * statement. A name such as r8 is a label (LBL-16).
 *   parser  the parser of the line
 *   text    the first character of the name
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_named_operand(Parser *parser, const char *text)
{
    const char *end = text;
    char *name;

    while (!ends_token(*end) && *end != OPEN_BRACKET)
    {
        end++;
    }
    name = copy_piece(parser, text, end);
    if (name == NULL)
    {
        return NULL;
    }
    if (*end != OPEN_BRACKET && register_number(name) != NOT_A_REGISTER)
    {
        statement_add_register_operand(parser->statement,
                                       register_number(name));
        free(name);
        return end;
    }
    end = parse_label_operand(parser, name, end);
    if (end == NULL)
    {
        free(name);
    }
    return end;
}

/*
 * Parses the next operand of an instruction and adds it to the statement.
 * The addressing mode follows from the first character (ADR-15).
 *   parser  the parser of the line
 *   text    the position at which the operand is expected
 * Returns the position just after the operand, or NULL after an error
 * (ERR-20, ERR-26, ERR-42 and the errors of the operand itself).
 */
static const char *parse_operand(Parser *parser, const char *text)
{
    const Statement *statement = parser->statement;
    const char *start = util_skip_spaces(text);

    if (*start == '\0')
    {
        fail(parser, (statement_operand_count(statement) == 0)
                         ? "an operand is missing; the operation takes %d"
                         : "an operand is missing after the comma; the "
                           "operation takes %d",
             instruction_operand_count(statement_instruction(statement)));
        return NULL;
    }
    if (*start == OPERAND_SEPARATOR)
    {
        fail(parser, "unexpected comma where an operand is expected");
        return NULL;
    }
    if (*start == OPEN_BRACKET)
    {
        fail(parser, "the label of a matrix operand is missing before its "
                     "first index");
        return NULL;
    }
    return (*start == IMMEDIATE_CHARACTER)
               ? parse_immediate(parser, start)
               : parse_named_operand(parser, start);
}

/*
 * Checks what follows an operand that is not the last: exactly one comma
 * (SYN-13, ERR-20, ERR-26, ERR-37).
 *   parser  the parser of the line
 *   text    the position just after the operand
 * Returns the position just after the comma, or NULL after an error.
 */
static const char *expect_comma(Parser *parser, const char *text)
{
    const char *position = util_skip_spaces(text);

    if (*position == OPERAND_SEPARATOR)
    {
        return position + 1;
    }
    if (*position == '\0')
    {
        fail(parser, "an operand is missing; the operation takes %d",
             instruction_operand_count(
                 statement_instruction(parser->statement)));
    }
    else if (*position == OPEN_BRACKET)
    {
        fail(parser, "white space is not allowed between the label of a "
                     "matrix operand and its first index");
    }
    else
    {
        fail(parser, "a comma is missing between two operands");
    }
    return NULL;
}

/*
 * Checks that nothing follows the last operand of an instruction (ERR-19,
 * ERR-26, ERR-27, ERR-37).
 *   parser  the parser of the line
 *   text    the position just after the last operand, or after the
 *           operation name when the operation has no operands
 * Returns FALSE after an error.
 */
static Boolean expect_end(Parser *parser, const char *text)
{
    const char *position = util_skip_spaces(text);

    if (*position == '\0')
    {
        return TRUE;
    }
    if (statement_operand_count(parser->statement) == 0)
    {
        return fail(parser, "unexpected text after the operation name; the "
                            "operation takes no operands");
    }
    if (*position == OPEN_BRACKET)
    {
        return fail(parser, "white space is not allowed between the label "
                            "of a matrix operand and its first index");
    }
    if (*position != OPERAND_SEPARATOR)
    {
        return fail(parser, "unexpected text after the last operand");
    }
    if (util_is_blank(position + 1))
    {
        return fail(parser, "unexpected comma after the last operand");
    }
    return fail(parser, "unexpected operand after the last one; the "
                        "operation takes only %d",
                statement_operand_count(parser->statement));
}

/*
 * Checks every operand's addressing mode against the instruction table
 * (INS-4, ERR-21, ERR-22).
 *   parser  the parser of the line; its statement has all its operands
 */
static void check_modes(Parser *parser)
{
    const Statement *statement = parser->statement;
    const Instruction *instruction = statement_instruction(statement);
    AddressingMode mode;
    int index;

    for (index = 0; index < statement_operand_count(statement); index++)
    {
        mode = statement_operand_mode(statement, index);
        if (statement_operand_is_source(statement, index))
        {
            if (!instruction_allows_source(instruction, mode))
            {
                fail(parser, "the source operand is %s, which the "
                             "operation does not allow",
                     mode_descriptions[mode]);
                return;
            }
        }
        else if (!instruction_allows_destination(instruction, mode))
        {
            fail(parser, "the destination operand is %s, which the "
                         "operation does not allow",
                 mode_descriptions[mode]);
            return;
        }
    }
}

/*
 * Parses the operands of an instruction, as many as its operation takes
 * (SYN-10, SYN-13), and checks their addressing modes.
 *   parser  the parser of the line; its statement has its instruction
 *   text    the text after the operation name
 */
static void parse_operands(Parser *parser, const char *text)
{
    int expected = instruction_operand_count(
        statement_instruction(parser->statement));
    const char *position = text;
    int index;

    for (index = 0; index < expected; index++)
    {
        position = parse_operand(parser, position);
        if (position != NULL && index + 1 < expected)
        {
            position = expect_comma(parser, position);
        }
        if (position == NULL)
        {
            return;
        }
    }
    if (expect_end(parser, position))
    {
        check_modes(parser);
    }
}

/*
 * Finds the longest operation name with which a longer word begins.
 *   start        the first character of the word
 *   end          the position just after the word
 *   instruction  receives the row of that operation, or NULL when the word
 *                begins with no operation name
 * Returns the length of that name, or 0 when there is none.
 */
static size_t operation_prefix_length(const char *start, const char *end,
                                      const Instruction **instruction)
{
    size_t length = (size_t)(end - start);

    *instruction = NULL;
    while (length > 0 && *instruction == NULL)
    {
        length--;
        *instruction = instruction_find_text(start, length);
    }
    return length;
}

/*
 * Reports a first field that is not an operation name: an operation glued
 * to its operand (SYN-12, ERR-35) or an unknown operation (ERR-17). The
 * rule: the field is a glued operation when it begins with an operation
 * name, the longest one that fits, when that operation takes operands, and
 * when what follows the name in the field holds a character that is not a
 * letter. So "movr1,r2" and "inc1" are an operation with its operand stuck
 * to it, while "stopper" (nothing can be stuck to "stop", which takes no
 * operand) and "increase" (letters only: a misspelt name, or the name of a
 * macro left unexpanded, MAC-32) are unknown operations.
 *   parser  the parser of the line
 *   start   the first character of the field
 *   end     the position just after the field
 */
static void report_unknown_operation(Parser *parser, const char *start,
                                     const char *end)
{
    const Instruction *instruction;
    const char *name_end = start + operation_prefix_length(start, end,
                                                           &instruction);

    if (instruction != NULL && instruction_operand_count(instruction) > 0 &&
        has_non_letter(name_end, end))
    {
        fail(parser, "white space is missing after the operation name '%.*s'",
             (int)(name_end - start), start);
        return;
    }
    fail(parser, "unknown operation '%.*s'", (int)(end - start), start);
}

/*
 * Parses an instruction statement (SYN-10, SYN-11).
 *   parser  the parser of the line
 *   start   the first character of the operation name
 */
static void parse_instruction(Parser *parser, const char *start)
{
    const char *end = util_skip_field(start);
    const Instruction *instruction =
        instruction_find_text(start, (size_t)(end - start));

    if (instruction == NULL)
    {
        report_unknown_operation(parser, start, end);
        return;
    }
    statement_set_instruction(parser->statement, instruction);
    parse_operands(parser, end);
}

/*
 * Recognises a label in front of .entry or .extern by the fields of the
 * line alone: the first field ends with a colon and the second field is
 * exactly ".entry" or ".extern" (DIR-35). The label is not validated and
 * not stored.
 *   parser  the parser of the line
 *   first   the first field of the line
 * Returns the position of the directive when the line has this form, and
 * first when it does not.
 */
static const char *skip_ignored_label(Parser *parser, const char *first)
{
    const char *end = util_skip_field(first);
    const char *second = util_skip_spaces(end);
    const DirectiveRule *rule;

    if (end == first || end[-1] != LABEL_END_CHARACTER)
    {
        return first;
    }
    rule = find_directive_rule(second, util_skip_field(second));
    if (rule == NULL || !rule->ignores_label)
    {
        return first;
    }
    statement_ignore_label(parser->statement);
    return second;
}

/*
 * Stores the label of the line in the statement when its name is legal,
 * and reports it when it is not (ERR-11, ERR-12, ERR-13). A name that is
 * not legal is never stored, so that the label which the first pass defines
 * for a faulty line is always a legal one (LBL-23).
 *   parser  the parser of the line
 *   start   the first character of the name
 *   colon   the position of the colon that ends the name
 * Returns FALSE after an error or when memory is exhausted.
 */
static Boolean store_label(Parser *parser, const char *start,
                           const char *colon)
{
    char *name = copy_piece(parser, start, colon);

    if (name == NULL)
    {
        return FALSE;
    }
    if (!require_name(parser, name, ROLE_LABEL))
    {
        free(name);
        return FALSE;
    }
    statement_set_label(parser->statement, name);
    return TRUE;
}

/*
 * Parses the label definition at the start of a line, if there is one
 * (LBL-1, LBL-4, LBL-18, LBL-22; ERR-16, ERR-38, ERR-45). A first field
 * that starts with a dot is a directive and never a label.
 *   parser  the parser of the line
 *   first   the first field of the line
 * Returns the position of the statement after the label, first when the
 * line has no label, or NULL after an error.
 */
static const char *parse_label(Parser *parser, const char *first)
{
    const char *end = util_skip_field(first);
    const char *colon = (const char *)memchr(first, LABEL_END_CHARACTER,
                                             (size_t)(end - first));

    if (*first == DIRECTIVE_CHARACTER)
    {
        return first;
    }
    if (colon == NULL)
    {
        if (*util_skip_spaces(end) != LABEL_END_CHARACTER)
        {
            return first;
        }
        fail(parser, "white space is not allowed between the label '%.*s' "
                     "and its colon",
             (int)(end - first), first);
        return NULL;
    }
    if (!store_label(parser, first, colon))
    {
        return NULL;
    }
    if (colon + 1 != end)
    {
        fail(parser, "white space is missing after the colon of the label "
                     "'%s'",
             statement_label(parser->statement));
        return NULL;
    }
    if (util_is_blank(end))
    {
        fail(parser, "a statement is missing after the label '%s'",
             statement_label(parser->statement));
        return NULL;
    }
    return util_skip_spaces(end);
}

/*
 * Parses a whole line into the statement of the parser (SYN-3).
 *   parser  the parser of the line
 *   line    the text of the line
 */
static void parse_line(Parser *parser, const char *line)
{
    const char *start;

    /* A comment is a line whose very first character is ';' (SYN-9). */
    if (line[0] == COMMENT_CHARACTER)
    {
        statement_set_kind(parser->statement, STATEMENT_COMMENT);
        return;
    }
    if (util_is_blank(line))
    {
        statement_set_kind(parser->statement, STATEMENT_EMPTY);
        return;
    }
    start = skip_ignored_label(parser, util_skip_spaces(line));
    if (!statement_has_ignored_label(parser->statement))
    {
        start = parse_label(parser, start);
    }
    if (start == NULL)
    {
        return;
    }
    if (*start == DIRECTIVE_CHARACTER)
    {
        parse_directive(parser, start);
    }
    else
    {
        parse_instruction(parser, start);
    }
}

/* Parses one line; see syntax.h. */
Statement *syntax_parse(const char *line, int line_number,
                        ErrorContext *errors)
{
    Parser parser;

    parser.statement = statement_create();
    if (parser.statement == NULL)
    {
        return NULL;
    }
    parser.errors = errors;
    parser.line_number = line_number;
    parser.no_memory = FALSE;
    parse_line(&parser, line);
    if (parser.no_memory)
    {
        statement_destroy(parser.statement);
        return NULL;
    }
    return parser.statement;
}
