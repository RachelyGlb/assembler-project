/*
 * legacy_bridge.c - the temporary bridge to the legacy passes.
 *
 * TEMPORARY; see legacy_bridge.h. This file is the only rebuilt code that
 * includes legacy headers. It repeats, unchanged, the sequence of calls that
 * the old driver made after the pre-assembler; it neither fixes nor improves
 * the legacy behaviour (for example, the result of the legacy first pass is
 * still not available, because that function returns nothing).
 */
#include "legacy_bridge.h"

#include "first_pass.h"
#include "memory_builder.h"
#include "second_pass.h"
#include "types.h"

/* Runs the legacy passes on an expanded source file; see legacy_bridge.h. */
Boolean legacy_run_passes(const char *am_file_name)
{
    MemoryImage memory;

    first_pass(am_file_name);
    if (!build_memory_image(am_file_name, &memory))
    {
        return FALSE;
    }
    if (!second_pass(am_file_name, &memory))
    {
        return FALSE;
    }
    free_memory_image(&memory);
    return TRUE;
}
