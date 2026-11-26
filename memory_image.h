/*
 * memory_image.h - the code image and the data image of one source file.
 *
 * The image holds the machine words of the instructions and the machine
 * words of the data separately, each with its counter (ALG-4): IC, which
 * starts at the load address 100 (ALG-2), and DC, which starts at 0. The
 * two images together may hold at most MAX_PROGRAM_WORDS words, because the
 * machine has 256 cells and the code is loaded at address 100 (ALG-15); a
 * word that does not fit is refused. This is the only module that stores
 * what the input controls in arrays of a fixed size (ALG-12).
 */
#ifndef MEMORY_IMAGE_H
#define MEMORY_IMAGE_H

#include "common.h"

/* The two images of one source file. The structure is private. */
typedef struct MemoryImage MemoryImage;

/*
 * Creates an empty image: no code, no data.
 * Returns the image, or NULL when memory is exhausted.
 */
MemoryImage *memory_image_create(void);

/*
 * Frees an image.
 *   image  the image to free; NULL is allowed and does nothing
 */
void memory_image_destroy(MemoryImage *image);

/*
 * Returns the number of words that can still be added, to the code and the
 * data together, before the memory of the machine is full (ALG-15).
 *   image  the image to ask
 */
int memory_image_free_words(const MemoryImage *image);

/*
 * Appends one word to the code image and advances IC (ALG-3).
 *   image  the image to extend
 *   word   the machine word
 * Returns FALSE, and changes nothing, when the memory is full.
 */
Boolean memory_image_add_code(MemoryImage *image, unsigned int word);

/*
 * Appends one word to the data image and advances DC (DIR-7).
 *   image  the image to extend
 *   word   the machine word
 * Returns FALSE, and changes nothing, when the memory is full.
 */
Boolean memory_image_add_data(MemoryImage *image, unsigned int word);

/*
 * Sets bits of a code word that is already in the image. The passes add an
 * instruction as zero words and then switch on the bits of each word as
 * they become known (ALG-7).
 *   image    the image to change
 *   address  the address of the word, from FIRST_LOAD_ADDRESS up to IC - 1;
 *            any other address is ignored
 *   bits     the bits to switch on in the word
 */
void memory_image_merge_code(MemoryImage *image, int address,
                             unsigned int bits);

/*
 * Returns IC: the address that the next code word will get (ALG-2, ALG-3).
 *   image  the image to ask
 */
int memory_image_instruction_counter(const MemoryImage *image);

/*
 * Returns DC: the number of data words, which is the offset that the next
 * data word will get inside the data image (LBL-9).
 *   image  the image to ask
 */
int memory_image_data_counter(const MemoryImage *image);

/*
 * Returns the number of words in the code image.
 *   image  the image to ask
 */
int memory_image_code_count(const MemoryImage *image);

/*
 * Returns one word of the code image.
 *   image  the image to ask
 *   index  the position of the word, from 0 to the code count - 1
 * Returns the word, or zero for an index outside the image.
 */
unsigned int memory_image_code_word(const MemoryImage *image, int index);

/*
 * Returns one word of the data image.
 *   image  the image to ask
 *   index  the position of the word, from 0 to DC - 1
 * Returns the word, or zero for an index outside the image.
 */
unsigned int memory_image_data_word(const MemoryImage *image, int index);

#endif /* MEMORY_IMAGE_H */
