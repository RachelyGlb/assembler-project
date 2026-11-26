/*
 * line_map.h - the map from .am line numbers to .as line numbers.
 *
 * Macro expansion moves lines, so the passes, which read the .am file,
 * cannot report the line numbers of the .as file by counting. The
 * pre-assembler therefore records, for every line it writes to the .am
 * file, the number of the .as line the text was copied from; for a line of
 * a macro body that is the line inside the definition, not the line of the
 * call (MAC-35). The passes translate with line_map_source_line before they
 * report (ERR-R12). The map exists only in memory and grows as needed.
 */
#ifndef LINE_MAP_H
#define LINE_MAP_H

#include "common.h"

/* The line map of one source file. The structure is private. */
typedef struct LineMap LineMap;

/*
 * Creates an empty line map.
 * Returns the map, or NULL when memory is exhausted.
 */
LineMap *line_map_create(void);

/*
 * Frees a line map.
 *   map  the map to free; NULL is allowed and does nothing
 */
void line_map_destroy(LineMap *map);

/*
 * Records the origin of the next line of the .am file. The first call
 * describes .am line 1, the second call .am line 2, and so on.
 *   map          the map to extend
 *   source_line  the number of the .as line the .am line was copied from
 * Returns FALSE when memory is exhausted; the map is then unchanged.
 */
Boolean line_map_append(LineMap *map, int source_line);

/*
 * Returns the number of .am lines recorded in the map.
 *   map  the map to ask
 */
int line_map_count(const LineMap *map);

/*
 * Translates a line number of the .am file.
 *   map           the map to ask
 *   expanded_line the line number in the .am file, counted from 1
 * Returns the number of the .as line it was copied from, or NO_LINE when
 * the map holds no such .am line.
 */
int line_map_source_line(const LineMap *map, int expanded_line);

#endif /* LINE_MAP_H */
