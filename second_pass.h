/*
 * second_pass.h - the second pass over the expanded source file.
 *
 * The second pass reads the .am file again, after a first pass that found
 * no error. It gives the attribute "entry" to the symbols named by .entry
 * directives, completes the code words that hold the address of a symbol,
 * and records every word that refers to an external symbol (ALG-10).
 *
 * It reports the second-pass errors of SPEC section 5.4 (ERR-50, ERR-51,
 * ERR-52), with .as line numbers (ERR-R12), one message per line (ERR-R16),
 * in the order of the .am file (ERR-R17).
 */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include <stdio.h>

#include "common.h"
#include "errors.h"
#include "line_map.h"
#include "memory_image.h"
#include "reference_list.h"
#include "symbol_table.h"

/*
 * Runs the second pass on one expanded source file.
 *   am_file    the .am file, open for reading at its beginning; the caller
 *              closes it
 *   lines      the line map of the file, for the .as line numbers (MAC-35)
 *   symbols    the symbol table filled by the first pass; on return the
 *              entry symbols are marked
 *   image      the memory image filled by the first pass; on return every
 *              code word is complete
 *   externals  an empty list; on return it holds, for every use of an
 *              external symbol, the name and the address of the word
 *   errors     the diagnostics context of the file
 * Returns STAGE_OK when no error was found; STAGE_ERRORS when at least one
 * error was reported, in which case no output file may be written (ERR-R6);
 * STAGE_FATAL when memory ran out (the caller reports that, ERR-R10).
 */
StageResult second_pass(FILE *am_file, const LineMap *lines,
                        SymbolTable *symbols, MemoryImage *image,
                        ReferenceList *externals, ErrorContext *errors);

#endif /* SECOND_PASS_H */
