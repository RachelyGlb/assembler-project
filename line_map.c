/*
 * line_map.c - the map from .am line numbers to .as line numbers.
 *
 * The map is an array of .as line numbers indexed by the .am line number
 * minus one. The array grows when it is full. See line_map.h for the
 * contracts.
 */
#include "line_map.h"

#include <stdlib.h>

#include "util.h"

/* The first size of the array. */
#define MAP_INITIAL_CAPACITY 64

/* The line map of one source file. */
struct LineMap
{
    int *source_lines; /* The .as line of every .am line; NULL while empty. */
    int count;         /* The number of .am lines recorded. */
    size_t capacity;   /* The number of elements allocated. */
};

/* Creates an empty line map; see line_map.h. */
LineMap *line_map_create(void)
{
    LineMap *map = (LineMap *)malloc(sizeof(LineMap));

    if (map != NULL)
    {
        map->source_lines = NULL;
        map->count = 0;
        map->capacity = 0;
    }
    return map;
}

/* Frees a line map; see line_map.h. */
void line_map_destroy(LineMap *map)
{
    if (map != NULL)
    {
        free(map->source_lines);
        free(map);
    }
}

/* Records the origin of the next .am line; see line_map.h. */
Boolean line_map_append(LineMap *map, int source_line)
{
    int *source_lines = (int *)util_reserve(map->source_lines, &map->capacity,
                                            (size_t)map->count + 1,
                                            MAP_INITIAL_CAPACITY, sizeof(int));

    if (source_lines == NULL)
    {
        return FALSE;
    }
    map->source_lines = source_lines;
    map->source_lines[map->count++] = source_line;
    return TRUE;
}

/* Translates a line number of the .am file; see line_map.h. */
int line_map_source_line(const LineMap *map, int expanded_line)
{
    if (expanded_line < FIRST_LINE_NUMBER || expanded_line > map->count)
    {
        return NO_LINE;
    }
    return map->source_lines[expanded_line - FIRST_LINE_NUMBER];
}
