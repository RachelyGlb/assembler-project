/*
 * output_writer.h - the writer of the output files.
 *
 * After two passes without an error the writer produces, for one source
 * file, the object file .ob and, only when they have content, the entries
 * file .ent and the externals file .ext (OUT-1 to OUT-4, OUT-7 to OUT-9).
 * The exact layout of every line is that of OUT-13.
 */
#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include "common.h"
#include "errors.h"
#include "memory_image.h"
#include "reference_list.h"
#include "symbol_table.h"

/*
 * Writes the output files of one source file.
 *   base_name  the name of the source file without the extension (OUT-1)
 *   image      the complete memory image
 *   symbols    the symbol table, with the entry symbols marked
 *   externals  the uses of external symbols, in the order of the code
 *   errors     the diagnostics context of the file
 * Returns STAGE_OK when every file was written; STAGE_ERRORS when a file
 * could not be created or written, which is reported, and then none of the
 * three files is left; STAGE_FATAL when memory ran out (the caller reports
 * that, ERR-R10).
 */
StageResult write_output_files(const char *base_name,
                               const MemoryImage *image,
                               const SymbolTable *symbols,
                               const ReferenceList *externals,
                               ErrorContext *errors);

/*
 * Removes the output files of a source file: the object file, the entries
 * file and the externals file. This is the one list of the files that the
 * writer makes; the driver uses it for the files left by an earlier run
 * (OUT-14) and the writer for its own files after a failure. A file that
 * does not exist is not an error.
 *   base_name  the name of the source file without the extension
 * Returns FALSE when memory is exhausted; every file whose name could
 * still be built was removed all the same.
 */
Boolean remove_output_files(const char *base_name);

#endif /* OUTPUT_WRITER_H */
