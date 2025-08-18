#include "preassembler.h"
#include "first_pass.h"
#include "second_pass.h"
#include "memory_builder.h"
#include "output_writer.h"
 #include "assembler.h"
 #include "types.h"



int main(int argc, char *argv[])
{
    Boolean success = TRUE;

    /* Check if any input files were provided */
    if (argc < 2)
    {
        printf("Warning: No input files provided.\n");
        printf("Usage: %s <file1> [file2] ...\n", argv[0]);
        return 0;
    }

    /* Process all input files */
    success = process_files(argc, argv);

    if (!success)
    {
        printf("Assembler terminated due to errors.\n");
        return 1;
    }

    printf("Assembly process completed successfully.\n");
    return 0;
}

/* Process all files provided in command line arguments */
Boolean process_files(int argc, char *argv[])
{
    int i;
    Boolean overall_success = TRUE;

    for (i = 1; i < argc; i++)
    {
        printf("Processing file: %s\n", argv[i]);

        if (!process_single_file(argv[i]))
        {
            overall_success = FALSE;
            printf("Error processing file: %s\n", argv[i]);
        }
    }

    return overall_success;
}

/* Process a single input file through all phases */
Boolean process_single_file(const char *filename)
{
    char *input_filename = NULL;
    char *am_filename = NULL;
    Boolean success = FALSE;
    MemoryImage memory;

    /* Create full input filename with .as extension */
    input_filename = create_filename_with_extension(filename, AS_EXTENSION);
    if (!input_filename)
    {
        print_error(MEMORY_ALLOCATION_ERROR, 0, "allocating buffer for input filename");
        return FALSE;
    }

    /* Check if input file exists */
    if (!file_exists(input_filename))
    {
        print_error(FILE_ERROR, 0, "input file does not exist");
        free(input_filename);
        return FALSE;
    }

    /* Create .am filename */
    am_filename = create_filename_with_extension(filename, AM_EXTENSION);
    if (!am_filename)
    {
        print_error(MEMORY_ALLOCATION_ERROR, 0, "allocating buffer for .am filename");
        free(input_filename);
        return FALSE;
    }

    printf("\n=== PHASE 1: PRE-ASSEMBLER ===\n");

    /* Phase 1: Pre-assembler (macro expansion) */
    success = preassembler(filename);
    if (!success)
    {
        printf("Pre-assembler phase failed for file: %s\n", filename);
        free(input_filename);
        free(am_filename);
        return FALSE;
    }
    printf("Pre-assembler phase completed successfully.\n");

    printf("\n=== PHASE 2: FIRST PASS ===\n");

    /* Phase 2: First pass (symbol table building) */
    first_pass(am_filename);

    printf("\n=== PHASE 3: MEMORY IMAGE BUILDING ===\n");

    /* Phase 3: Build memory image */
    if (!build_memory_image(am_filename, &memory))
    {
        printf("Memory image building failed for file: %s\n", filename);
        free(input_filename);
        free(am_filename);
        return FALSE;
    }
    printf("Memory image built successfully.\n");

    printf("\n=== PHASE 4: SECOND PASS ===\n");

    /* Phase 4: Second pass (complete encoding and generate output) */
    if (!second_pass(am_filename, &memory))
    {
        printf("Second pass failed for file: %s\n", filename);
        free(input_filename);
        free(am_filename);
        return FALSE;
    }
    printf("Second pass completed successfully.\n");

    /* Cleanup memory */
    free_memory_image(&memory);

    /* Cleanup */
    free(input_filename);
    free(am_filename);

    printf("Successfully processed file: %s\n", filename);
    return TRUE;
}