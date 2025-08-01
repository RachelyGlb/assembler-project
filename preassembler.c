#include "assembler.h" 
#include "preassembler.h"


Boolean preassembler(char* filename) {

    char *input_name = create_filename_with_extension(filename, AS_EXTENSION);
    FILE *input = fopen(input_name, "r");
    if(!input)
    {
        print_error(FILE_ERROR, 0, input_name);
        free(input_name);
        return FALSE;
    }

    char *output_name = create_filename_with_extension(filename, AM_EXTENSION);
    FILE *output = fopen(output_name, "w");
    if(!output)
    {
        print_error(FILE_ERROR, 0, output_name);
        free(output_name);
        return FALSE;
    }

    MacroTable *macro_table = create_macro_table();

    if(!macro_table)
    {
        print_error(MEMORY_ALLOCATION_ERROR, 0, filename);
        fclose(input);
        fclose(output);
        free(input_name);
        free(output_name);
        return FALSE;
    }

    char line[MAX_LINE_LENGTH];
    char the_first_character;
    int i;

    i=0;

    line = fgets(line, sizeof(line), input);
    
    
    if(strtok(line,MACRO_START)==TRUE)
    {
        if(is_valid_macro(input, line))
        {
            macro_table.
        }
    }