#include "output_writer.h"
#include "memory_builder.h"
#include "line_analysis.h"
#include "instruction_table.h"

/* Generate all output files */
Boolean generate_output_files(const char *filename, MemoryImage *memory,
                              ExtRef *ext_list, EntryPoint *entry_list)
{
    Boolean success = TRUE;

    /* Always generate .ob file */
    if (!write_object_file(filename, memory))
    {
        printf("Error: Failed to write object file\n");
        success = FALSE;
    }

    /* Generate .ent file only if there are entry points */
    if (count_entries(entry_list) > 0)
    {
        if (!write_entries_file(filename, entry_list))
        {
            printf("Error: Failed to write entries file\n");
            success = FALSE;
        }
    }

    /* Generate .ext file only if there are external references */
    if (count_externals(ext_list) > 0)
    {
        if (!write_externals_file(filename, ext_list))
        {
            printf("Error: Failed to write externals file\n");
            success = FALSE;
        }
    }

    return success;
}

/* Write .ob (object) file */
Boolean write_object_file(const char *filename, MemoryImage *memory)
{
    FILE *file;
    char *base_name;
    char *obj_filename;
    char *ic_base4, *dc_base4, *ic_trimmed, *dc_trimmed;
    char *address_base4, *word_base4;
    int i;
    int calculated_address;
    /* Create output filename */
    base_name = get_base_filename(filename);
    if (!base_name)
        return FALSE;

    obj_filename = malloc(strlen(base_name) + 4 + 1); /* ".ob" + '\0' */
    if (!obj_filename)
    {
        free(base_name);
        return FALSE;
    }
    sprintf(obj_filename, "%s.ob", base_name);

    /* Open file for writing */
    file = fopen(obj_filename, "w");
    if (!file)
    {
        printf("Error: Cannot create object file %s\n", obj_filename);
        free(base_name);
        free(obj_filename);
        return FALSE;
    }

    /* Header: IC and DC in base-4 (trim leading 'a's) */

    ic_base4 = decimal_to_base4(memory->instruction_count, 5);
    dc_base4 = decimal_to_base4(memory->data_count, 5);

    


    if (!ic_base4 || !dc_base4)
    {
        printf("Error: Failed to convert header to base 4\n");
        fclose(file);
        free(base_name);
        free(obj_filename);
        if (ic_base4)
            free(ic_base4);
        if (dc_base4)
            free(dc_base4);
        return FALSE;
    }

    ic_trimmed = ic_base4;
    dc_trimmed = dc_base4;
    while (*ic_trimmed == 'a' && strlen(ic_trimmed) > 1)
        ic_trimmed++;
    while (*dc_trimmed == 'a' && strlen(dc_trimmed) > 1)
        dc_trimmed++;

    fprintf(file, "%s %s\n", ic_trimmed, dc_trimmed);
    /* Write instruction image: addr(4), word(5) */
    for (i = 0; i < memory->instruction_count; i++)
    {
        printf("Instruction[%d] at addr %d: decimal=%d, binary=", 
           i, BASE_ADDRESS + i, memory->instruction_image[i].bits);
        address_base4 = decimal_to_base4(BASE_ADDRESS + i, 4);
        
        word_base4 = decimal_to_base4(memory->instruction_image[i].bits & 0x3FF, 5);
         
        if (!address_base4 || !word_base4)
        {
            printf("Error: Failed to convert instruction to base 4\n");
            fclose(file);
            free(base_name);
            free(obj_filename);
            free(ic_base4);
            free(dc_base4);
            if (address_base4)
                free(address_base4);
            if (word_base4)
                free(word_base4);
            return FALSE;
        }
        fprintf(file, "%s %s\n", address_base4, word_base4);
        free(address_base4);
        free(word_base4);
        }
    

    
    for (i = 0; i < memory->data_count; i++)
    {
        calculated_address = BASE_ADDRESS + memory->instruction_count + i;
        printf("Data[%d] at addr %d: decimal=%d\n", i, BASE_ADDRESS + memory->instruction_count + i, memory->data_image[i].bits);
         
    
        address_base4 = decimal_to_base4(calculated_address, 4);
        word_base4 = decimal_to_base4(memory->data_image[i].bits & 0x3FF, 5);
        if (!address_base4 || !word_base4)
        {
            printf("Error: Failed to convert data to base 4\n");
            fclose(file);
            free(base_name);
            free(obj_filename);
            free(ic_base4);
            free(dc_base4);
            if (address_base4)
                free(address_base4);
            if (word_base4)
                free(word_base4);
            return FALSE;
        }
        fprintf(file, "%s %s\n", address_base4, word_base4);
        free(address_base4);
        free(word_base4);
    }

    fclose(file);

    printf("Generated object file: %s\n", obj_filename);

    free(base_name);
    free(obj_filename);
    free(ic_base4);
    free(dc_base4);

    return TRUE;
}

/* Write .ent (entries) file */
Boolean write_entries_file(const char *filename, EntryPoint *entry_list)
{
    FILE *file;
    char *base_name;
    char *ent_filename;
    char *address_base4;
    EntryPoint *current;

    if (!entry_list)
        return TRUE; /* No entries to write */

    /* Create output filename */
    base_name = get_base_filename(filename);
    if (!base_name)
        return FALSE;

    ent_filename = malloc(strlen(base_name) + 6); /* +5 for ".ent" +1 for null */
    if (!ent_filename)
    {
        free(base_name);
        return FALSE;
    }
    sprintf(ent_filename, "%s.ent", base_name);

    /* Open file for writing */
    file = fopen(ent_filename, "w");
    if (!file)
    {
        printf("Error: Cannot create entries file %s\n", ent_filename);
        free(base_name);
        free(ent_filename);
        return FALSE;
    }

    /* Write each entry point */
    current = entry_list;
    while (current)
    {
        address_base4 = decimal_to_base4(current->address, 4);
        if (!address_base4)
        {
            printf("Error: Failed to convert entry address to base 4\n");
            fclose(file);
            free(base_name);
            free(ent_filename);
            return FALSE;
        }

        fprintf(file, "%s %s\n", current->symbol_name, address_base4);
        free(address_base4);
        current = current->next;
    }

    fclose(file);
    free(base_name);
    free(ent_filename);

    printf("Generated entries file: %s\n", ent_filename);
    return TRUE;
}

/* Write .ext (externals) file */
Boolean write_externals_file(const char *filename, ExtRef *ext_list)
{
    FILE *file;
    char *base_name;
    char *ext_filename;
    char *address_base4;
    ExtRef *current;

    if (!ext_list)
        return TRUE; /* No externals to write */

    /* Create output filename */
    base_name = get_base_filename(filename);
    if (!base_name)
        return FALSE;

    ext_filename = malloc(strlen(base_name) + 6); /* +5 for ".ext" +1 for null */
    if (!ext_filename)
    {
        free(base_name);
        return FALSE;
    }
    sprintf(ext_filename, "%s.ext", base_name);

    /* Open file for writing */
    file = fopen(ext_filename, "w");
    if (!file)
    {
        printf("Error: Cannot create externals file %s\n", ext_filename);
        free(base_name);
        free(ext_filename);
        return FALSE;
    }

    /* Write each external reference */
    current = ext_list;
    while (current)
    {
        address_base4 = decimal_to_base4(current->address, 4);
        if (!address_base4)
        {
            printf("Error: Failed to convert external address to base 4\n");
            fclose(file);
            free(base_name);
            free(ext_filename);
            return FALSE;
        }

        fprintf(file, "%s %s\n", current->symbol_name, address_base4);
        free(address_base4);
        current = current->next;
    }

    fclose(file);
    free(base_name);
    free(ext_filename);

    printf("Generated externals file: %s\n", ext_filename);
    return TRUE;
}

/* Get base filename without extension */
char *get_base_filename(const char *filename)
{
    char *base_name;
    char *dot_pos;
    int len;

    if (!filename)
        return NULL;

    /* Find last dot in filename */
    dot_pos = strrchr(filename, '.');
    if (dot_pos)
    {
        len = dot_pos - filename;
    }
    else
    {
        len = strlen(filename);
    }

    base_name = malloc(len + 1);
    if (!base_name)
        return NULL;

    strncpy(base_name, filename, len);
    base_name[len] = '\0';

    return base_name;
}

/* Count entries in list */
int count_entries(EntryPoint *list)
{
    int count = 0;
    EntryPoint *current = list;

    while (current)
    {
        count++;
        current = current->next;
    }

    return count;
}

/* Count externals in list */
int count_externals(ExtRef *list)
{
    int count = 0;
    ExtRef *current = list;

    while (current)
    {
        count++;
        current = current->next;
    }

    return count;
}
