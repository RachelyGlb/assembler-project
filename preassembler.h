/*
 * preassembler.h - the pre-assembler: macro expansion.
 *
 * The pre-assembler reads the source file, checks the length of every line
 * (SYN-2b), collects the macro definitions, replaces every macro call by
 * the body of the macro and writes the result to the .am file (MAC-29). It
 * reports every pre-assembler error of SPEC section 5.2 (ERR-2 to ERR-10),
 * a line that contains a NUL byte (ERR-46) and a macro definition that is
 * not closed (ERR-48), each with its .as line number, one message per line
 * (ERR-R16), and does not stop at the first (MAC-22). A source file that cannot be read is reported without a line
 * number (ERR-47). The pre-assembler does not look into macro bodies and
 * does not check labels (MAC-24); the passes do that.
 */
#ifndef PREASSEMBLER_H
#define PREASSEMBLER_H

#include <stdio.h>

#include "common.h"
#include "errors.h"
#include "line_map.h"
#include "macro_table.h"

/*
 * Runs the pre-assembler on one source file.
 *   source        the .as file, open for reading at its beginning; the
 *                 caller closes it
 *   am_file_name  the name of the .am file to write
 *   macros        an empty macro table; on return it holds the macros that
 *                 were defined correctly, for the first pass (LBL-21)
 *   lines         an empty line map; on return it holds, for every line of
 *                 the .am file, its .as line number (MAC-35)
 *   errors        the diagnostics context of the file
 * Returns STAGE_OK when the .am file was written and no error was found;
 * STAGE_ERRORS when at least one error was reported, in which case no .am
 * file is left (MAC-23); STAGE_FATAL when memory ran out, in which case no
 * .am file is left either and nothing was reported about it (the caller
 * does that, ERR-R10).
 */
StageResult preassemble(FILE *source, const char *am_file_name,
                        MacroTable *macros, LineMap *lines,
                        ErrorContext *errors);

#endif /* PREASSEMBLER_H */
