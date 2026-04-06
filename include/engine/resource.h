#ifndef ENGINE_RESOURCE_H
#define ENGINE_RESOURCE_H

/*
 * engine/resource.h — Resource / asset management interface.
 *
 * Provides a minimal load-and-cache abstraction.  Assets are identified by a
 * string key and an EngineResourceKind tag.  Warden currently has no binary
 * assets (all content is in C source), so this module is a stub that can be
 * extended when file-based assets are introduced.
 *
 * To add a new asset type:
 *   1. Add an ENGINE_RESOURCE_<TYPE> entry to EngineResourceKind.
 *   2. Implement load/unload in src/engine/resource.c.
 *   3. Cache the result in the internal resource table.
 */

#include <stdbool.h>
#include <stddef.h>

typedef enum EngineResourceKind {
    ENGINE_RESOURCE_NONE   = 0,
    ENGINE_RESOURCE_DATA   = 1,  /* generic binary blob */
    ENGINE_RESOURCE_TEXT   = 2,  /* UTF-8 text file     */
    ENGINE_RESOURCE_SPRITE = 3   /* image / sprite data */
} EngineResourceKind;

/* Opaque resource handle (NULL = not loaded). */
typedef struct EngineResource {
    EngineResourceKind kind;
    const char        *key;
    void              *data;   /* owned by resource manager */
    size_t             size;
} EngineResource;

/*
 * Load (or return cached) resource for key.
 * Returns NULL if the resource cannot be found.
 * Stub implementation always returns NULL.
 */
const EngineResource *engine_resource_load(const char *key,
                                           EngineResourceKind kind);

/* Release a previously loaded resource and remove it from the cache. */
void engine_resource_unload(const char *key);

/* Release all cached resources. */
void engine_resource_unload_all(void);

#endif /* ENGINE_RESOURCE_H */
