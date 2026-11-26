/*
 * reference_list.c - an ordered list of (name, address) pairs.
 *
 * The list is an array that grows when it is full, so a reference is
 * reached by its position in constant time. See reference_list.h for the
 * contracts.
 */
#include "reference_list.h"

#include <stdlib.h>

#include "util.h"

/* The first size of the array. */
#define LIST_INITIAL_CAPACITY 16

/* One reference. */
typedef struct
{
    char *name;  /* The name of the symbol; owned. */
    int address; /* The address of the word that refers to it. */
} Reference;

/* A list of references. */
struct ReferenceList
{
    Reference *items; /* The references; NULL while the list is empty. */
    int count;        /* The number of references stored. */
    size_t capacity;  /* The number of elements allocated. */
};

/* Creates an empty list; see reference_list.h. */
ReferenceList *reference_list_create(void)
{
    ReferenceList *list = (ReferenceList *)malloc(sizeof(ReferenceList));

    if (list != NULL)
    {
        list->items = NULL;
        list->count = 0;
        list->capacity = 0;
    }
    return list;
}

/* Frees a list with all its references; see reference_list.h. */
void reference_list_destroy(ReferenceList *list)
{
    int index;

    if (list == NULL)
    {
        return;
    }
    for (index = 0; index < list->count; index++)
    {
        free(list->items[index].name);
    }
    free(list->items);
    free(list);
}

/*
 * Makes sure that the array has room for one more reference.
 *   list  the list to enlarge
 * Returns FALSE when the array had to grow and could not; the list is then
 * unchanged.
 */
static Boolean reserve_item(ReferenceList *list)
{
    Reference *items = (Reference *)util_reserve(list->items, &list->capacity,
                                                 (size_t)list->count + 1,
                                                 LIST_INITIAL_CAPACITY,
                                                 sizeof(Reference));

    if (items == NULL)
    {
        return FALSE;
    }
    list->items = items;
    return TRUE;
}

/* Appends one reference; see reference_list.h. */
Boolean reference_list_append(ReferenceList *list, const char *name,
                              int address)
{
    char *copy;

    if (!reserve_item(list))
    {
        return FALSE;
    }
    copy = util_copy_string(name);
    if (copy == NULL)
    {
        return FALSE;
    }
    list->items[list->count].name = copy;
    list->items[list->count].address = address;
    list->count++;
    return TRUE;
}

/* Returns the number of references; see reference_list.h. */
int reference_list_count(const ReferenceList *list)
{
    return list->count;
}

/* Returns the symbol name of one reference; see reference_list.h. */
const char *reference_list_name(const ReferenceList *list, int index)
{
    return list->items[index].name;
}

/* Returns the address of one reference; see reference_list.h. */
int reference_list_address(const ReferenceList *list, int index)
{
    return list->items[index].address;
}
