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
 * See syntax.h for the contracts of the public functions.
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "syntax.h"
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

/* The numbers of words of the parts of an instruction (ADR-14). */
#define FIRST_WORD_COUNT 1
#define SIMPLE_OPERAND_WORDS 1
#define MATRIX_OPERAND_WORDS 2

/* The first size of the array of data values, and its growth factor. */
#define VALUES_INITIAL_CAPACITY 8
#define VALUES_GROWTH_FACTOR 2

/* Turns the value of a constant into a string literal, for messages. */
#define STRINGIFY_TOKEN(token) #token
#define STRINGIFY(constant) STRINGIFY_TOKEN(constant)

/* What is wrong with a candidate label name, if anything. */
typedef enum
{
    NAME_LEGAL,      /* The name may be a label. */
    NAME_MALFORMED,  /* Not a letter followed by letters and digits. */
    NAME_TOO_LONG,   /* More than MAX_NAME_LENGTH characters. */
    NAME_RESERVED    /* A reserved word (LBL-20). */
} NameStatus;

/* One operand of an instruction. */
typedef struct
{
    AddressingMode mode; /* The addressing mode. */
    int value;           /* The immediate value or the register number. */
    char *symbol;        /* The name as written (not immediate); owned. */
    int row_register;    /* The row register of a matrix operand. */
    int column_register; /* The column register of a matrix operand. */
} Operand;

/* One parsed line. */
struct Statement
{
    StatementKind kind;             /* The kind of the statement. */
    Boolean valid;                  /* FALSE once an error was reported. */
    char *label;                    /* The legal label, or NULL; owned. */
    Boolean label_ignored;          /* A label before .entry or .extern. */
    const Instruction *instruction; /* The operation, or NULL. */
    int operand_count;              /* The number of operands parsed. */
    Operand operands[MAX_OPERAND_COUNT]; /* The operands, as written. */
    char *directive_symbol;         /* The operand of .entry/.extern; owned. */
    int *values;                    /* The listed data values; owned. */
    int value_count;                /* The number of listed values. */
    int value_capacity;             /* The number of elements allocated. */
    int data_count;                 /* The number of data words taken. */
};

/* What every parsing function needs. */
typedef struct
{
    Statement *statement; /* The statement being filled. */
    ErrorContext *errors; /* The diagnostics context of the file. */
    int line_number;      /* The .as line number, for the message. */
    Boolean no_memory;    /* TRUE once an allocation failed. */
} Parser;

/*
 * Reports the syntax error of the line and marks the statement as not
 * valid.
 *   parser  the parser of the line
 *   text    the message
 * Returns FALSE, so that a caller can write "return fail(...)".
 */
static Boolean fail(Parser *parser, const char *text)
{
    errors_report(parser->errors, parser->line_number, "%s", text);
    parser->statement->valid = FALSE;
    return FALSE;
}

/*
 * Reports the syntax error of the line with a piece of the line quoted in
 * the message, and marks the statement as not valid.
 *   parser  the parser of the line
 *   format  a printf format with exactly one "%.*s" conversion
 *   start   the first character of the piece
 *   end     the position just after the piece
 * Returns FALSE.
 */
static Boolean fail_at(Parser *parser, const char *format, const char *start,
                       const char *end)
{
    errors_report(parser->errors, parser->line_number, format,
                  (int)(end - start), start);
    parser->statement->valid = FALSE;
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
 * Returns the words that describe a name that is not legal, for a message.
 *   status  the result of check_name, not NAME_LEGAL
 */
static const char *name_problem(NameStatus status)
{
    switch (status)
    {
    case NAME_TOO_LONG:
        return "is longer than " STRINGIFY(MAX_NAME_LENGTH) " characters";
    case NAME_RESERVED:
        return "is a reserved word";
    default:
        return "must be a letter followed by letters and digits";
    }
}

/*
 * Checks that a name is a legal label name and reports it when it is not.
 *   parser  the parser of the line
 *   name    the name, a string
 *   role    what the name is used as, for the message
 * Returns TRUE when the name is legal.
 */
static Boolean require_name(Parser *parser, const char *name,
                            const char *role)
{
    NameStatus status = check_name(name);

    if (status == NAME_LEGAL)
    {
        return TRUE;
    }
    errors_report(parser->errors, parser->line_number, "%s '%s' %s", role,
                  name, name_problem(status));
    parser->statement->valid = FALSE;
    return FALSE;
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
 * Appends one value to the data values of the statement.
 *   parser  the parser of the line; it notes an allocation failure
 *   value   the value to append
 * Returns FALSE when memory is exhausted.
 */
static Boolean append_value(Parser *parser, int value)
{
    Statement *statement = parser->statement;
    int capacity;
    int *values;

    if (statement->value_count == statement->value_capacity)
    {
        capacity = (statement->value_capacity == 0)
                       ? VALUES_INITIAL_CAPACITY
                       : statement->value_capacity * VALUES_GROWTH_FACTOR;
        values = (int *)realloc(statement->values,
                                (size_t)capacity * sizeof(int));
        if (values == NULL)
        {
            parser->no_memory = TRUE;
            return FALSE;
        }
        statement->values = values;
        statement->value_capacity = capacity;
    }
    statement->values[statement->value_count++] = value;
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
        fail(parser, "a comma stands where a number is expected");
        return NULL;
    }
    if (*start == '\0')
    {
        fail(parser, "a comma follows the last number of the list");
        return NULL;
    }
    if (!read_integer(start, end, &value))
    {
        fail_at(parser, "'%.*s' is not a legal integer", start, end);
        return NULL;
    }
    if (value < DATA_VALUE_MIN || value > DATA_VALUE_MAX)
    {
        fail_at(parser,
                "the value %.*s does not fit in a data word of "
                STRINGIFY(WORD_BITS) " bits",
                start, end);
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
                           : fail(parser, "the directive has no values");
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
 * Parses the operands of .data (DIR-5 to DIR-7, ERR-28 to ERR-30).
 *   parser  the parser of the line
 *   text    the text after the directive name
 */
static void parse_data(Parser *parser, const char *text)
{
    if (parse_number_list(parser, text, FALSE))
    {
        parser->statement->data_count = parser->statement->value_count;
    }
}

/*
 * Stores the characters of a string and the terminating zero as data values
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
            return fail(parser, "the string holds a character that is not "
                                "printable ASCII");
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
        fail(parser, "the operand of .string must start with a quotation "
                     "mark");
        return;
    }
    if (close == open)
    {
        fail(parser, "the string has no closing quotation mark");
        return;
    }
    if (!util_is_blank(close + 1))
    {
        fail(parser, "there is text after the closing quotation mark");
        return;
    }
    if (store_string(parser, open + 1, close))
    {
        parser->statement->data_count = parser->statement->value_count;
    }
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
        fail(parser, "a matrix dimension in square brackets is expected");
        return NULL;
    }
    start = util_skip_spaces(text + 1);
    if (*start == PLUS_SIGN || *start == MINUS_SIGN)
    {
        fail(parser, "a matrix dimension must not have a sign");
        return NULL;
    }
    for (end = start; isdigit((unsigned char)*end); end++)
    {
    }
    if (!read_integer(start, end, &value) ||
        *util_skip_spaces(end) != CLOSE_BRACKET)
    {
        fail(parser, "a matrix dimension must be a number in square "
                     "brackets");
        return NULL;
    }
    if (value == 0)
    {
        fail(parser, "a matrix dimension must be at least 1");
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
    if (statement->value_count > rows * columns)
    {
        fail(parser, "the matrix has more values than cells");
        return;
    }
    statement->data_count = rows * columns;
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
        fail(parser, "the directive needs the name of a label");
        return;
    }
    name = copy_piece(parser, start, end);
    if (name == NULL)
    {
        return;
    }
    parser->statement->directive_symbol = name;
    if (!require_name(parser, name, "the operand"))
    {
        return;
    }
    if (!util_is_blank(end))
    {
        fail(parser, "the directive takes exactly one label");
    }
}

/*
 * Looks up the directive that a piece of the line names.
 *   parser  the parser of the line
 *   start   the first character of the piece
 *   end     the position just after the piece
 *   kind    receives the directive, or DIRECTIVE_NONE
 * Returns FALSE when memory is exhausted.
 */
static Boolean lookup_directive(Parser *parser, const char *start,
                                const char *end, DirectiveKind *kind)
{
    char *name = copy_piece(parser, start, end);

    if (name == NULL)
    {
        return FALSE;
    }
    *kind = directive_find(name);
    free(name);
    return TRUE;
}

/*
 * Reports a first field that starts with a dot and is not a directive: a
 * directive glued to its operand (SYN-17, ERR-36) or an unknown directive
 * (ERR-18).
 *   parser  the parser of the line
 *   start   the first character of the field, the dot
 *   end     the position just after the field
 */
static void report_unknown_directive(Parser *parser, const char *start,
                                     const char *end)
{
    const char *name_end = start + 1;
    DirectiveKind kind;

    /* A directive name is a dot and lowercase letters (DIR-2). */
    while (islower((unsigned char)*name_end))
    {
        name_end++;
    }
    if (!lookup_directive(parser, start, name_end, &kind))
    {
        return;
    }
    if (kind != DIRECTIVE_NONE)
    {
        fail_at(parser,
                "the directive name %.*s is not followed by white space", start,
                name_end);
        return;
    }
    fail_at(parser, "unknown directive '%.*s'", start, end);
}

/*
 * Parses a directive statement (DIR-1).
 *   parser  the parser of the line
 *   start   the first character of the directive name, the dot
 */
static void parse_directive(Parser *parser, const char *start)
{
    Statement *statement = parser->statement;
    const char *end = util_skip_field(start);
    DirectiveKind kind;

    if (!lookup_directive(parser, start, end, &kind))
    {
        return;
    }
    switch (kind)
    {
    case DIRECTIVE_DATA:
        statement->kind = STATEMENT_DATA;
        parse_data(parser, end);
        break;
    case DIRECTIVE_STRING:
        statement->kind = STATEMENT_STRING;
        parse_string(parser, end);
        break;
    case DIRECTIVE_MAT:
        statement->kind = STATEMENT_MAT;
        parse_mat(parser, end);
        break;
    case DIRECTIVE_ENTRY:
        statement->kind = STATEMENT_ENTRY;
        parse_symbol_operand(parser, end);
        break;
    case DIRECTIVE_EXTERN:
        statement->kind = STATEMENT_EXTERN;
        parse_symbol_operand(parser, end);
        break;
    default:
        report_unknown_directive(parser, start, end);
        break;
    }
}

/*
 * Parses an immediate operand: '#' and, attached to it, a legal integer in
 * the range of eight bits (ADR-2, ERR-24, ERR-39).
 *   parser   the parser of the line
 *   text     the position of the '#'
 *   operand  receives the operand
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_immediate(Parser *parser, const char *text,
                                   Operand *operand)
{
    const char *end = token_end(text);
    long value;

    if (!read_integer(text + 1, end, &value))
    {
        fail_at(parser, "'%.*s' is not '#' followed by a legal integer",
                text, end);
        return NULL;
    }
    if (value < IMMEDIATE_MIN || value > IMMEDIATE_MAX)
    {
        fail_at(parser,
                "the immediate operand %.*s does not fit in "
                STRINGIFY(OPERAND_FIELD_BITS) " bits",
                text, end);
        return NULL;
    }
    operand->mode = ADDRESSING_IMMEDIATE;
    operand->value = (int)value;
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
        fail(parser, "a matrix index has no closing bracket");
        return NULL;
    }
    while (end > start && util_is_space(end[-1]))
    {
        end--;
    }
    *register_ = register_number_text(start, (size_t)(end - start));
    if (*register_ == NOT_A_REGISTER)
    {
        fail_at(parser, "the matrix index '%.*s' is not a register r0 to r7",
                start, end);
        return NULL;
    }
    return close + 1;
}

/*
 * Parses the two indices of a matrix operand, after its label (ADR-6,
 * SYN-18, ERR-25, ERR-37).
 *   parser   the parser of the line
 *   text     the position of the first opening bracket
 *   operand  receives the index registers and the mode
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_matrix(Parser *parser, const char *text,
                                Operand *operand)
{
    const char *position = parse_index(parser, text, &operand->row_register);

    if (position == NULL)
    {
        return NULL;
    }
    if (*position != OPEN_BRACKET)
    {
        fail(parser, (*util_skip_spaces(position) == OPEN_BRACKET)
                         ? "white space separates the two indices of a "
                           "matrix operand"
                         : "a matrix operand needs two indices in brackets");
        return NULL;
    }
    position = parse_index(parser, position, &operand->column_register);
    if (position == NULL)
    {
        return NULL;
    }
    if (!ends_token(*position))
    {
        fail(parser, "there is text right after the second index of a "
                     "matrix operand");
        return NULL;
    }
    operand->mode = ADDRESSING_MATRIX;
    return position;
}

/*
 * Parses an operand that starts with a name: a register (ADR-8), a label
 * (ADR-4) or a label with two indices (ADR-6). A name such as r8 is a label
 * (LBL-16).
 *   parser   the parser of the line
 *   text     the first character of the name
 *   operand  receives the operand
 * Returns the position just after the operand, or NULL after an error.
 */
static const char *parse_named_operand(Parser *parser, const char *text,
                                       Operand *operand)
{
    const char *end = text;

    while (!ends_token(*end) && *end != OPEN_BRACKET)
    {
        end++;
    }
    operand->symbol = copy_piece(parser, text, end);
    if (operand->symbol == NULL)
    {
        return NULL;
    }
    if (*end != OPEN_BRACKET)
    {
        operand->value = register_number(operand->symbol);
        if (operand->value != NOT_A_REGISTER)
        {
            operand->mode = ADDRESSING_REGISTER;
            return end;
        }
    }
    if (!require_name(parser, operand->symbol, "the operand"))
    {
        return NULL;
    }
    operand->value = 0;
    operand->mode = ADDRESSING_DIRECT;
    return (*end == OPEN_BRACKET) ? parse_matrix(parser, end, operand) : end;
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
    Statement *statement = parser->statement;
    Operand *operand = &statement->operands[statement->operand_count];
    const char *start = util_skip_spaces(text);
    const char *end;

    if (*start == '\0')
    {
        fail(parser, (statement->operand_count == 0)
                         ? "the operation needs more operands"
                         : "an operand is missing after the comma");
        return NULL;
    }
    if (*start == OPERAND_SEPARATOR)
    {
        fail(parser, "a comma stands where an operand is expected");
        return NULL;
    }
    if (*start == OPEN_BRACKET)
    {
        fail(parser, "a matrix operand needs a label before its brackets");
        return NULL;
    }
    end = (*start == IMMEDIATE_CHARACTER)
              ? parse_immediate(parser, start, operand)
              : parse_named_operand(parser, start, operand);
    if (end != NULL)
    {
        statement->operand_count++;
    }
    return end;
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
        fail(parser, "the operation needs more operands");
    }
    else if (*position == OPEN_BRACKET)
    {
        fail(parser, "white space separates a matrix label from its "
                     "brackets");
    }
    else
    {
        fail(parser, "a comma is missing between the operands");
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
    if (parser->statement->operand_count == 0)
    {
        return fail(parser, "the operation takes no operands");
    }
    if (*position == OPEN_BRACKET)
    {
        return fail(parser, "white space separates a matrix label from its "
                            "brackets");
    }
    if (*position != OPERAND_SEPARATOR)
    {
        return fail(parser, "there is extra text after the last operand");
    }
    return fail(parser, util_is_blank(position + 1)
                            ? "a comma follows the last operand"
                            : "the operation has too many operands");
}

/*
 * Checks every operand's addressing mode against the instruction table
 * (INS-4, ERR-21, ERR-22).
 *   parser  the parser of the line; its statement has all its operands
 */
static void check_modes(Parser *parser)
{
    const Statement *statement = parser->statement;
    AddressingMode mode;
    int index;

    for (index = 0; index < statement->operand_count; index++)
    {
        mode = statement->operands[index].mode;
        if (statement_operand_is_source(statement, index))
        {
            if (!instruction_allows_source(statement->instruction, mode))
            {
                fail(parser, "the addressing mode of the source operand is "
                             "not legal for this operation");
                return;
            }
        }
        else if (!instruction_allows_destination(statement->instruction,
                                                 mode))
        {
            fail(parser, "the addressing mode of the destination operand "
                         "is not legal for this operation");
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
    int expected = instruction_operand_count(parser->statement->instruction);
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
 * Reports a first field that is not an operation name: an operation glued
 * to its operand (SYN-12, ERR-35) or an unknown operation (ERR-17).
 *   parser  the parser of the line
 *   start   the first character of the field
 *   end     the position just after the field
 */
static void report_unknown_operation(Parser *parser, const char *start,
                                     const char *end)
{
    const char *name_end = start;
    char *name;
    Boolean glued;

    /* An operation name is made of lowercase letters (ENC-14). */
    while (islower((unsigned char)*name_end))
    {
        name_end++;
    }
    name = copy_piece(parser, start, name_end);
    if (name == NULL)
    {
        return;
    }
    glued = (instruction_find(name) != NULL) ? TRUE : FALSE;
    free(name);
    if (glued)
    {
        fail_at(parser,
                "the operation name %.*s is not followed by white space", start,
                name_end);
        return;
    }
    fail_at(parser, "unknown operation '%.*s'", start, end);
}

/*
 * Parses an instruction statement (SYN-10, SYN-11).
 *   parser  the parser of the line
 *   start   the first character of the operation name
 */
static void parse_instruction(Parser *parser, const char *start)
{
    Statement *statement = parser->statement;
    const char *end = util_skip_field(start);
    char *name = copy_piece(parser, start, end);

    if (name == NULL)
    {
        return;
    }
    statement->instruction = instruction_find(name);
    free(name);
    if (statement->instruction == NULL)
    {
        report_unknown_operation(parser, start, end);
        return;
    }
    statement->kind = STATEMENT_INSTRUCTION;
    parse_operands(parser, end);
}

/*
 * Recognises a label in front of .entry or .extern by the fields of the
 * line alone: the first field ends with a colon and the second field is
 * exactly ".entry" or ".extern" (DIR-35). The label is not validated and
 * not stored.
 *   parser  the parser of the line
 *   first   the first field of the line
 * Returns the position of the directive when the line has this form, first
 * when it does not, or NULL when memory is exhausted.
 */
static const char *skip_ignored_label(Parser *parser, const char *first)
{
    const char *end = util_skip_field(first);
    const char *second = util_skip_spaces(end);
    DirectiveKind kind;

    if (end == first || end[-1] != LABEL_END_CHARACTER)
    {
        return first;
    }
    if (!lookup_directive(parser, second, util_skip_field(second), &kind))
    {
        return NULL;
    }
    if (kind != DIRECTIVE_ENTRY && kind != DIRECTIVE_EXTERN)
    {
        return first;
    }
    parser->statement->label_ignored = TRUE;
    return second;
}

/*
 * Stores the label of the line in the statement when its name is legal,
 * and reports it when it is not (ERR-11, ERR-12, ERR-13).
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
    if (!require_name(parser, name, "the label"))
    {
        free(name);
        return FALSE;
    }
    parser->statement->label = name;
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
        fail(parser, "white space separates the label from its colon");
        return NULL;
    }
    if (!store_label(parser, first, colon))
    {
        return NULL;
    }
    if (colon + 1 != end)
    {
        fail(parser, "white space is missing after the colon of the label");
        return NULL;
    }
    if (util_is_blank(end))
    {
        fail(parser, "the label is followed by no statement");
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
        parser->statement->kind = STATEMENT_COMMENT;
        return;
    }
    if (util_is_blank(line))
    {
        parser->statement->kind = STATEMENT_EMPTY;
        return;
    }
    start = skip_ignored_label(parser, util_skip_spaces(line));
    if (start != NULL && !parser->statement->label_ignored)
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

/*
 * Creates a statement with nothing in it.
 * Returns the statement, or NULL when memory is exhausted.
 */
static Statement *statement_create(void)
{
    Statement *statement = (Statement *)malloc(sizeof(Statement));
    int index;

    if (statement == NULL)
    {
        return NULL;
    }
    statement->kind = STATEMENT_UNKNOWN;
    statement->valid = TRUE;
    statement->label = NULL;
    statement->label_ignored = FALSE;
    statement->instruction = NULL;
    statement->operand_count = 0;
    for (index = 0; index < MAX_OPERAND_COUNT; index++)
    {
        statement->operands[index].mode = ADDRESSING_IMMEDIATE;
        statement->operands[index].value = 0;
        statement->operands[index].symbol = NULL;
        statement->operands[index].row_register = 0;
        statement->operands[index].column_register = 0;
    }
    statement->directive_symbol = NULL;
    statement->values = NULL;
    statement->value_count = 0;
    statement->value_capacity = 0;
    statement->data_count = 0;
    return statement;
}

/* Parses one line; see syntax.h. */
Statement *statement_parse(const char *line, int line_number,
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

/* Frees a statement; see syntax.h. */
void statement_destroy(Statement *statement)
{
    int index;

    if (statement == NULL)
    {
        return;
    }
    for (index = 0; index < MAX_OPERAND_COUNT; index++)
    {
        free(statement->operands[index].symbol);
    }
    free(statement->label);
    free(statement->directive_symbol);
    free(statement->values);
    free(statement);
}

/* Tells whether the line was free of syntax errors; see syntax.h. */
Boolean statement_is_valid(const Statement *statement)
{
    return statement->valid;
}

/* Returns the kind of a statement; see syntax.h. */
StatementKind statement_kind(const Statement *statement)
{
    return statement->kind;
}

/* Returns the label defined by the line; see syntax.h. */
const char *statement_label(const Statement *statement)
{
    return statement->label;
}

/* Tells whether the line has an ignored label; see syntax.h. */
Boolean statement_has_ignored_label(const Statement *statement)
{
    return statement->label_ignored;
}

/* Returns the operation of an instruction; see syntax.h. */
const Instruction *statement_instruction(const Statement *statement)
{
    return statement->instruction;
}

/* Returns the number of operands; see syntax.h. */
int statement_operand_count(const Statement *statement)
{
    return statement->operand_count;
}

/* Tells whether an operand is the source operand; see syntax.h. */
Boolean statement_operand_is_source(const Statement *statement, int index)
{
    /* Only the first of two operands is a source (ENC-1, ENC-9). */
    return (index == 0 && statement->operand_count == MAX_OPERAND_COUNT)
               ? TRUE
               : FALSE;
}

/* Returns the addressing mode of an operand; see syntax.h. */
AddressingMode statement_operand_mode(const Statement *statement, int index)
{
    return statement->operands[index].mode;
}

/* Returns the number that an operand carries; see syntax.h. */
int statement_operand_value(const Statement *statement, int index)
{
    return statement->operands[index].value;
}

/* Returns the symbol that an operand names; see syntax.h. */
const char *statement_operand_symbol(const Statement *statement, int index)
{
    AddressingMode mode = statement->operands[index].mode;

    if (mode != ADDRESSING_DIRECT && mode != ADDRESSING_MATRIX)
    {
        return NULL;
    }
    return statement->operands[index].symbol;
}

/* Returns one index register of a matrix operand; see syntax.h. */
int statement_operand_matrix_register(const Statement *statement, int index,
                                      MatrixIndex which)
{
    return (which == MATRIX_ROW) ? statement->operands[index].row_register
                                 : statement->operands[index].column_register;
}

/*
 * Returns the number of additional words that one operand takes when it
 * does not share a word (ADR-14).
 *   operand  the operand
 */
static int operand_words(const Operand *operand)
{
    return (operand->mode == ADDRESSING_MATRIX) ? MATRIX_OPERAND_WORDS
                                                : SIMPLE_OPERAND_WORDS;
}

/* Returns where the words of an operand lie; see syntax.h. */
int statement_operand_word_offset(const Statement *statement, int index)
{
    const Operand *first = &statement->operands[0];
    const Operand *operand = &statement->operands[index];

    /* The first operand follows the first word of the instruction. */
    if (index == 0)
    {
        return FIRST_WORD_COUNT;
    }
    /* Two registers share one word (ADR-11); nothing else does (ADR-13). */
    if (first->mode == ADDRESSING_REGISTER &&
        operand->mode == ADDRESSING_REGISTER)
    {
        return FIRST_WORD_COUNT;
    }
    return FIRST_WORD_COUNT + operand_words(first);
}

/*
 * Returns the length of an instruction in words (ADR-14): the offset of its
 * last operand plus the words of that operand, or one word when it has no
 * operand.
 *   statement  a valid instruction statement
 */
static int instruction_words(const Statement *statement)
{
    int last = statement->operand_count - 1;

    if (last < 0)
    {
        return FIRST_WORD_COUNT;
    }
    return statement_operand_word_offset(statement, last) +
           operand_words(&statement->operands[last]);
}

/* Returns the number of machine words of a statement; see syntax.h. */
int statement_word_count(const Statement *statement)
{
    if (statement->kind == STATEMENT_INSTRUCTION)
    {
        return instruction_words(statement);
    }
    return statement_data_count(statement);
}

/* Returns the number of data words of a directive; see syntax.h. */
int statement_data_count(const Statement *statement)
{
    return statement->data_count;
}

/* Returns one data word of a directive, as a number; see syntax.h. */
int statement_data_value(const Statement *statement, int index)
{
    /* A cell without a listed value is zero (DIR-13, DIR-24). */
    if (index < 0 || index >= statement->value_count)
    {
        return 0;
    }
    return statement->values[index];
}

/* Returns the symbol of .entry or .extern; see syntax.h. */
const char *statement_directive_symbol(const Statement *statement)
{
    return statement->directive_symbol;
}

/* The one rule for a label name; see syntax.h. */
Boolean syntax_is_valid_label(const char *name)
{
    return (check_name(name) == NAME_LEGAL) ? TRUE : FALSE;
}
