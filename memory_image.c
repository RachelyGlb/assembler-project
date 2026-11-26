/*
 * memory_image.c - the code image and the data image of one source file.
 *
 * Each image is an array of MAX_PROGRAM_WORDS words: either image alone may
 * fill the whole memory, and the sum of the two counters is what is checked
 * against the limit (ALG-15). See memory_image.h for the contracts.
 */
#include "memory_image.h"

#include <stdlib.h>

/* The two images of one source file. */
struct MemoryImage
{
    unsigned int code[MAX_PROGRAM_WORDS]; /* The words of the instructions. */
    unsigned int data[MAX_PROGRAM_WORDS]; /* The words of the data. */
    int code_count;                       /* The number of code words. */
    int data_count;                       /* The number of data words: DC. */
};

/* Creates an empty image; see memory_image.h. */
MemoryImage *memory_image_create(void)
{
    MemoryImage *image = (MemoryImage *)malloc(sizeof(MemoryImage));

    if (image != NULL)
    {
        image->code_count = 0;
        image->data_count = 0;
    }
    return image;
}

/* Frees an image; see memory_image.h. */
void memory_image_destroy(MemoryImage *image)
{
    free(image);
}

/* Returns the number of words that still fit; see memory_image.h. */
int memory_image_free_words(const MemoryImage *image)
{
    return MAX_PROGRAM_WORDS - image->code_count - image->data_count;
}

/* Appends one word to the code image; see memory_image.h. */
Boolean memory_image_add_code(MemoryImage *image, unsigned int word)
{
    if (memory_image_free_words(image) <= 0)
    {
        return FALSE;
    }
    image->code[image->code_count++] = word;
    return TRUE;
}

/* Appends one word to the data image; see memory_image.h. */
Boolean memory_image_add_data(MemoryImage *image, unsigned int word)
{
    if (memory_image_free_words(image) <= 0)
    {
        return FALSE;
    }
    image->data[image->data_count++] = word;
    return TRUE;
}

/* Sets bits of a code word of the image; see memory_image.h. */
void memory_image_merge_code(MemoryImage *image, int address,
                             unsigned int bits)
{
    int index = address - FIRST_LOAD_ADDRESS;

    if (index >= 0 && index < image->code_count)
    {
        image->code[index] |= bits;
    }
}

/* Returns IC; see memory_image.h. */
int memory_image_instruction_counter(const MemoryImage *image)
{
    return FIRST_LOAD_ADDRESS + image->code_count;
}

/* Returns DC; see memory_image.h. */
int memory_image_data_counter(const MemoryImage *image)
{
    return image->data_count;
}

/* Returns the number of code words; see memory_image.h. */
int memory_image_code_count(const MemoryImage *image)
{
    return image->code_count;
}

/* Returns one word of the code image; see memory_image.h. */
unsigned int memory_image_code_word(const MemoryImage *image, int index)
{
    if (index < 0 || index >= image->code_count)
    {
        return 0;
    }
    return image->code[index];
}

/* Returns one word of the data image; see memory_image.h. */
unsigned int memory_image_data_word(const MemoryImage *image, int index)
{
    if (index < 0 || index >= image->data_count)
    {
        return 0;
    }
    return image->data[index];
}
