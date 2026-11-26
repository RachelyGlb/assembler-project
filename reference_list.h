/*
 * reference_list.h - an ordered list of (name, address) pairs.
 *
 * The second pass records in such a list every machine word that refers to
 * an external symbol: the name of the symbol and the address of the word
 * (ALG-10 step 6). The output writer turns the list into the .ext file, one
 * line per reference (OUT-9). The list keeps the order in which the
 * references were added and grows as needed.
 */
#ifndef REFERENCE_LIST_H
#define REFERENCE_LIST_H

#include "common.h"

/* A list of references. The structure is private. */
typedef struct ReferenceList ReferenceList;

/*
 * Creates an empty list.
 * Returns the list, or NULL when memory is exhausted.
 */
ReferenceList *reference_list_create(void);

/*
 * Frees a list with all its references.
 *   list  the list to free; NULL is allowed and does nothing
 */
void reference_list_destroy(ReferenceList *list);

/*
 * Appends one reference.
 *   list     the list to extend
 *   name     the name of the symbol referred to; it is copied
 *   address  the address of the machine word that refers to the symbol
 * Returns FALSE when memory is exhausted; the list is then unchanged.
 */
Boolean reference_list_append(ReferenceList *list, const char *name,
                              int address);

/*
 * Returns the number of references in the list.
 *   list  the list to ask
 */
int reference_list_count(const ReferenceList *list);

/*
 * Returns the symbol name of one reference.
 *   list   the list to ask
 *   index  the position of the reference, from 0 to the count - 1
 */
const char *reference_list_name(const ReferenceList *list, int index);

/*
 * Returns the address of one reference.
 *   list   the list to ask
 *   index  the position of the reference, from 0 to the count - 1
 */
int reference_list_address(const ReferenceList *list, int index);

#endif /* REFERENCE_LIST_H */
