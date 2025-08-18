/* output_writer.h - output file generation */
#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include "types.h" /* includes all required definitions */

/* output file generation functions */
Boolean generate_output_files(const char *filename, MemoryImage *memory,
                              ExtRef *ext_list, EntryPoint *entry_list);

Boolean write_object_file(const char *filename, MemoryImage *memory);
Boolean write_entries_file(const char *filename, EntryPoint *entry_list);
Boolean write_externals_file(const char *filename, ExtRef *ext_list);

/* helper functions */
char *get_base_filename(const char *filename);
int count_entries(EntryPoint *list);
int count_externals(ExtRef *list);

/* base-4 conversion */
char *decimal_to_base4(int decimal, int width);

#endif /* OUTPUT_WRITER_H */
