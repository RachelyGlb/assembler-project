/*
 * assembler.c - the program entry and the per-file driver.
 *
 * Usage: assembler file1 [file2 ...]
 *
 * Every argument is the name of a source file without its ".as" extension
 * (CLI-2, CLI-8). The files are processed one after the other, each
 * independently of the others (CLI-3). For one file the driver removes the
 * output files left by an earlier run (OUT-14), opens the source file
 * (ERR-1), runs the stages in order and stops at the first stage that
 * fails. It owns everything that belongs to the file, in a FileJob, and
 * frees it on every path before the next file starts.
 *
 * The exit status is 0 when every file was assembled without errors and 1
 * otherwise (CLI-9). When memory runs out, the stage that noticed returns
 * STAGE_FATAL up to here; everything is freed, the failure is reported and
 * the whole program ends (ERR-R10).
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "errors.h"
#include "first_pass.h"
#include "line_map.h"
#include "macro_table.h"
#include "memory_image.h"
#include "output_writer.h"
#include "preassembler.h"
#include "reference_list.h"
#include "second_pass.h"
#include "symbol_table.h"
#include "util.h"

/* The exit statuses of the program (CLI-9). */
#define EXIT_STATUS_SUCCESS 0
#define EXIT_STATUS_FAILURE 1

/* The position of the first file name in the argument vector. */
#define FIRST_FILE_ARGUMENT 1

/* The line printed when the program is run without arguments (CLI-10). */
#define USAGE_LINE "usage: assembler file1 [file2 ...]"

/* Everything that belongs to the processing of one source file. */
typedef struct
{
    const char *base_name; /* The argument: the name without extension. */
    char *source_name;     /* The name of the .as file; owned. */
    char *am_name;         /* The name of the .am file; owned. */
    FILE *source;          /* The open .as file, or NULL. */
    FILE *expanded;        /* The open .am file, or NULL. */
    ErrorContext *errors;  /* The diagnostics context; owned. */
    MacroTable *macros;    /* The macro table; owned. */
    LineMap *lines;        /* The line map; owned. */
    SymbolTable *symbols;  /* The symbol table; owned. */
    MemoryImage *image;    /* The code image and the data image; owned. */
    ReferenceList *externals; /* The uses of external symbols; owned. */
} FileJob;

/*
 * Removes the output files that an earlier run may have left for a source
 * file, so that afterwards only the files of this run exist (OUT-14): the
 * .am file, which the pre-assembler makes, and the files of the output
 * writer. A file that does not exist is not an error. The source file is
 * not touched.
 *   base_name  the name of the source file without the extension
 * Returns FALSE when memory is exhausted.
 */
static Boolean remove_stale_outputs(const char *base_name)
{
    char *am_name = util_make_file_name(base_name, EXPANDED_EXTENSION);

    if (am_name == NULL)
    {
        return FALSE;
    }
    remove(am_name);
    free(am_name);
    return remove_output_files(base_name);
}

/*
 * Allocates everything a job needs before its source file is opened.
 *   job        the job to fill; every member is set, to NULL on failure
 *   base_name  the name of the source file without the extension; it must
 *              outlive the job
 * Returns FALSE when memory is exhausted; the job must still be released.
 */
static Boolean job_prepare(FileJob *job, const char *base_name)
{
    job->base_name = base_name;
    job->source = NULL;
    job->expanded = NULL;
    job->source_name = util_make_file_name(base_name, SOURCE_EXTENSION);
    job->am_name = util_make_file_name(base_name, EXPANDED_EXTENSION);
    job->errors = errors_create(base_name);
    job->macros = macro_table_create();
    job->lines = line_map_create();
    job->symbols = symbol_table_create();
    job->image = memory_image_create();
    job->externals = reference_list_create();
    return (job->source_name != NULL && job->am_name != NULL &&
            job->errors != NULL && job->macros != NULL &&
            job->lines != NULL && job->symbols != NULL &&
            job->image != NULL && job->externals != NULL)
               ? TRUE
               : FALSE;
}

/*
 * Frees everything a job owns and closes its files (ERR-R11). It is safe
 * after a job_prepare that failed half way.
 *   job  the job to release
 */
static void job_release(FileJob *job)
{
    if (job->source != NULL)
    {
        fclose(job->source);
    }
    if (job->expanded != NULL)
    {
        fclose(job->expanded);
    }
    reference_list_destroy(job->externals);
    memory_image_destroy(job->image);
    symbol_table_destroy(job->symbols);
    line_map_destroy(job->lines);
    macro_table_destroy(job->macros);
    errors_destroy(job->errors);
    free(job->am_name);
    free(job->source_name);
}

/*
 * Removes the .am file when a pass could not read it back, so that no
 * output file is left for the source file (CLI-12). The pass has already
 * reported the failure. The file is closed first, because a file that is
 * still open cannot be removed on every system.
 *   job  the job, with its .am file open
 */
static void discard_unreadable_expanded(FileJob *job)
{
    if (ferror(job->expanded))
    {
        fclose(job->expanded);
        job->expanded = NULL;
        remove(job->am_name);
    }
}

/*
 * Runs the stages that work on the expanded file: the first pass, the
 * second pass and the output writer. Each stage runs only when the stage
 * before it found no error (ERR-R6, ERR-R7); both passes read the .am file
 * from its beginning (ALG-8). A .am file that a pass could not read is
 * removed (CLI-12).
 *   job  the prepared job, after a pre-assembler that succeeded
 * Returns STAGE_OK when the output files were written, STAGE_ERRORS when a
 * stage reported errors, STAGE_FATAL when memory ran out.
 */
static StageResult job_assemble(FileJob *job)
{
    StageResult result;

    job->expanded = fopen(job->am_name, READ_MODE);
    if (job->expanded == NULL)
    {
        errors_report(job->errors, NO_LINE, "cannot open the %s file",
                      EXPANDED_EXTENSION);
        return STAGE_ERRORS;
    }
    result = first_pass(job->expanded, job->lines, job->macros, job->symbols,
                        job->image, job->errors);
    if (result == STAGE_OK)
    {
        rewind(job->expanded);
        result = second_pass(job->expanded, job->lines, job->symbols,
                             job->image, job->externals, job->errors);
    }
    if (result != STAGE_OK)
    {
        discard_unreadable_expanded(job);
        return result;
    }
    return write_output_files(job->base_name, job->image, job->symbols,
                              job->externals, job->errors);
}

/*
 * Runs the stages of the assembler on a prepared job, in order, and stops
 * after the first stage that does not succeed.
 *   job  the prepared job
 * Returns STAGE_OK when the file was assembled, STAGE_ERRORS when it could
 * not be opened (ERR-1) or read (ERR-47) or a stage found errors,
 * STAGE_FATAL when memory ran out.
 */
static StageResult job_run(FileJob *job)
{
    StageResult result;

    job->source = fopen(job->source_name, READ_MODE);
    if (job->source == NULL)
    {
        errors_report(job->errors, NO_LINE, "cannot open the source file");
        return STAGE_ERRORS;
    }
    result = preassemble(job->source, job->am_name, job->macros, job->lines,
                         job->errors);
    if (result != STAGE_OK)
    {
        return result;
    }
    return job_assemble(job);
}

/*
 * Processes one source file from start to end and frees everything that
 * belongs to it.
 *   base_name  the command-line argument: the name without the extension
 * Returns the result for the file; STAGE_FATAL means that memory ran out.
 */
static StageResult process_file(const char *base_name)
{
    FileJob job;
    StageResult result = STAGE_FATAL;

    if (!remove_stale_outputs(base_name))
    {
        return STAGE_FATAL;
    }
    if (job_prepare(&job, base_name))
    {
        result = job_run(&job);
    }
    job_release(&job);
    return result;
}

/*
 * The program entry: processes every file named on the command line.
 *   argc  the number of arguments, including the program name
 *   argv  the arguments; argv[1] onwards are file names without extension
 * Returns 0 when there was no argument or every file was assembled without
 * errors, and 1 otherwise (CLI-9, CLI-10).
 */
int main(int argc, char *argv[])
{
    int status = EXIT_STATUS_SUCCESS;
    int index;
    StageResult result;

    if (argc <= FIRST_FILE_ARGUMENT)
    {
        puts(USAGE_LINE);
        return EXIT_STATUS_SUCCESS;
    }
    for (index = FIRST_FILE_ARGUMENT; index < argc; index++)
    {
        result = process_file(argv[index]);
        if (result == STAGE_FATAL)
        {
            /* Everything is already freed; the program ends (ERR-R10). */
            errors_report_no_memory(argv[index]);
            return EXIT_STATUS_FAILURE;
        }
        if (result != STAGE_OK)
        {
            status = EXIT_STATUS_FAILURE;
        }
    }
    return status;
}
