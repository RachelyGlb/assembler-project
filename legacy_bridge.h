/*
 * legacy_bridge.h - the temporary bridge to the legacy passes.
 *
 * TEMPORARY. The first pass, the memory builder, the second pass and the
 * output writer are legacy code that the next two stages of the rebuild
 * replace. Until then the driver reaches them only through the one function
 * below, so that the rebuilt modules include no legacy header. When the new
 * passes exist, the driver calls them directly and this pair of files is
 * deleted together with types.h.
 */
#ifndef LEGACY_BRIDGE_H
#define LEGACY_BRIDGE_H

#include "common.h"

/*
 * Runs the legacy first pass, memory builder and second pass (which also
 * writes the output files) on an expanded source file.
 *   am_file_name  the name of the .am file written by the pre-assembler
 * Returns TRUE when the legacy code reported success. The legacy code
 * prints its own messages, does not use the error context, the macro table
 * or the line map, and keeps its symbol table in a global variable.
 */
Boolean legacy_run_passes(const char *am_file_name);

#endif /* LEGACY_BRIDGE_H */
