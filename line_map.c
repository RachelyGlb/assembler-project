/*
 * line_map.c - the map from .am line numbers to .as line numbers.
 *
 * The map is an array of .as line numbers indexed by the .am line number
 * minus one. The array doubles when it is full. See line_map.h for the
 * contracts.
 */
#include <stdlib.h>

#include "line_map.h"

/* The first size of the array, and its growth factor. */
#define MAP_INITIAL_CAPACITY 64
#define MAP_GROWTH_FACTOR 2

/* The line map of one source file. */
struct LineMap
{
    int *source_lines; /* The .as line of every .am line; NULL while empty. */
    int count;         /* The number of .am lines recorded. */
    int capacity;      /* The number of elements allocated. */
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
    int capacity;
    int *source_lines;

    if (map->count == map->capacity)
    {
        capacity = (map->capacity == 0) ? MAP_INITIAL_CAPACITY
                                        : map->capacity * MAP_GROWTH_FACTOR;
        source_lines = (int *)realloc(map->source_lines,
                                      (size_t)capacity * sizeof(int));
        if (source_lines == NULL)
        {
            return FALSE;
        }
        map->source_lines = source_lines;
        map->capacity = capacity;
    }
    map->source_lines[map->count++] = source_line;
    return TRUE;
}

/* Returns the number of .am lines recorded; see line_map.h. */
int line_map_count(const LineMap *map)
{
    return map->count;
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
