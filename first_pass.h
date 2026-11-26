/*
 * first_pass.h - the first pass over the expanded source file.
 *
 * The first pass reads the .am file once. It defines every label and every
 * external symbol in the symbol table, stores the data image, and stores
 * each instruction in the code image with every word that does not depend
 * on a symbol (ALG-7, ALG-9). At the end it moves the data symbols behind
 * the code (ALG-6).
 *
 * It reports every first-pass error of SPEC section 5.3 and the warning
 * WRN-1 (section 5.6), with .as line numbers (ERR-R12), one message per
 * line (ERR-R16), in the order of the .am file (ERR-R17). The syntax errors
 * among them are found by the syntax module, which this pass calls for
 * every line.
 */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include <stdio.h>

#include "common.h"
#include "errors.h"
#include "line_map.h"
#include "macro_table.h"
#include "memory_image.h"
#include "symbol_table.h"

/*
 * Runs the first pass on one expanded source file.
 *   am_file  the .am file, open for reading at its beginning; the caller
 *            closes it
 *   lines    the line map of the file, for the .as line numbers (MAC-35)
 *   macros   the macros of the file, for the clash between a label and a
 *            macro name (LBL-21)
 *   symbols  an empty symbol table; on return it holds the symbols
 *   image    an empty memory image; on return it holds the data image and
 *            the code image, in which the words that hold the address of a
 *            symbol are still zero
 *   errors   the diagnostics context of the file
 * Returns STAGE_OK when no error was found; STAGE_ERRORS when at least one
 * error was reported, in which case the second pass must not run (ERR-R7);
 * STAGE_FATAL when memory ran out (the caller reports that, ERR-R10).
 */
StageResult first_pass(FILE *am_file, const LineMap *lines,
                       const MacroTable *macros, SymbolTable *symbols,
                       MemoryImage *image, ErrorContext *errors);

#endif /* FIRST_PASS_H */
