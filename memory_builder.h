/* memory_builder.h - building a memory image */
#ifndef MEMORY_BUILDER_H
#define MEMORY_BUILDER_H

#include "types.h"

/* main functions */
Boolean build_memory_image(const char *am_filename, MemoryImage *memory);
void init_memory_image(MemoryImage *memory);

/* instruction processing */
void encode_instruction_first_pass(const char *line, MemoryImage *memory, int *current_ic, int line_number);

/* data directives processing */
void encode_data_directive(const char *line, MemoryImage *memory, int *current_dc, int line_number);
void encode_string_directive(const char *line, MemoryImage *memory, int *current_dc, int line_number);
void encode_matrix_directive(const char *line, MemoryImage *memory, int *current_dc, int line_number);

/* memory functions */
Boolean add_data_word(MemoryImage *memory, int address, MachineWord word);
Boolean add_instruction_word(MemoryImage *memory, int address, MachineWord word);
Boolean update_instruction_word(MemoryImage *memory, int address, MachineWord word);
void free_memory_image(MemoryImage *memory);

/* helper functions */
int get_opcode_value(const char *instruction_name);
int get_addressing_method(const char *operand);
MachineWord encode_first_word(int opcode, int src_method, int dest_method);
MachineWord encode_immediate_operand(int value);
MachineWord encode_register_operand(int src_reg, int dest_reg);
void extract_matrix_registers(const char *matrix_operand, int *row_reg, int *col_reg);

#endif /* MEMORY_BUILDER_H */
