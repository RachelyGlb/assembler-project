/* second_pass.h - the second pass */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "types.h"
#include "symbol_table.h"

/* context structure for the second pass */
typedef struct
{
    MemoryImage *memory;
    ExtRef *ext_list;
    EntryPoint *entry_list;
    Boolean has_errors;
    int current_ic;
} SecondPassContext;

/* second pass functions */
Boolean second_pass(const char *filename, MemoryImage *memory);
void process_instruction_second_pass(const char *line, SecondPassContext *ctx, int line_number);
void process_entry_directive(const char *line, SecondPassContext *ctx, int line_number);

/* encoding functions */
Boolean encode_operand(const char *operand, MachineWord *word, int *are_bits,
                       SecondPassContext *ctx, int line_number, int target_address);
int get_instruction_length(const char *instruction, const char *operand1, const char *operand2);

/* helper functions */
void add_external_reference(SecondPassContext *ctx, const char *symbol_name, int address);
void add_entry_point(SecondPassContext *ctx, const char *symbol_name, int address);

/* memory functions - moved to memory_builder.h */
/* void init_memory_image(MemoryImage *memory); - already in memory_builder.h */
void free_memory_image(MemoryImage *memory);

/* utility functions */
char *decimal_to_base4(int decimal, int digits);
void free_ext_list(ExtRef *head);
void free_entry_list(EntryPoint *head);

#endif /* SECOND_PASS_H */
