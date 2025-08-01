#include "assembler.h"

int main(int argc, char *argv[]) {
    Boolean success = TRUE;
    
    /* Check if any input files were provided */
    if (argc < 2) {
        printf("Warning: No input files provided.\n");
        return 0;
    }
    
    /* Process all input files */
    success = process_files(argc, argv);
    
    if (!success) {
        printf("Assembler terminated due to errors.\n");
        return 1;
    }
    
    printf("Pre-assembler phase completed successfully.\n");
    return 0;
}

/* Process all files provided in command line arguments */
Boolean process_files(int argc, char *argv[]) {
    int i;
    Boolean overall_success = TRUE;
    
    for (i = 1; i < argc; i++) {
        printf("Processing file: %s\n", argv[i]);
        
        if (!process_single_file(argv[i])) {
            overall_success = FALSE;
            printf("Error processing file: %s\n", argv[i]);
        }
    }
    
    return overall_success;
}

/* Process a single input file */
Boolean process_single_file(const char *filename) {
    char *input_filename = NULL;
    char *output_filename = NULL;
    Boolean success = FALSE;
    
    /* Create full input filename with .as extension */
    input_filename = create_filename_with_extension(filename, AS_EXTENSION);
    if (!input_filename) {
        print_error(MEMORY_ALLOCATION_ERROR, 0, filename);
        return FALSE;
    }
    
    /* Check if input file exists */
    if (!file_exists(input_filename)) {
        print_error(FILE_ERROR, 0, input_filename);
        free(input_filename);
        return FALSE;
    }
    
    /* Create output filename with .am extension */
    output_filename = create_filename_with_extension(filename, AM_EXTENSION);
    if (!output_filename) {
        print_error(MEMORY_ALLOCATION_ERROR, 0, filename);
        free(input_filename);
        return FALSE;
    }
    
    /* Process the file */
    success = preassembler(filename);
    
    /* Cleanup */
    free(input_filename);
    free(output_filename);
    
    return success;
}