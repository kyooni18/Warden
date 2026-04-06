#include "engine/resource.h"

/* Resource manager — stub implementation.
 * Warden's content lives entirely in C source data (kZones[], enemy tables,
 * quest strings).  This stub satisfies the interface so code can call
 * engine_resource_load() without crashing; it always returns NULL.
 * Replace with a real implementation when file-based assets are introduced.
 */

const EngineResource *engine_resource_load(const char *key,
                                           EngineResourceKind kind)
{
    (void)key;
    (void)kind;
    return NULL;
}

void engine_resource_unload(const char *key)     { (void)key; }
void engine_resource_unload_all(void)            {}
