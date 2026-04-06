#ifndef ENGINE_ENTITY_H
#define ENGINE_ENTITY_H

/*
 * engine/entity.h — Minimal entity / component interface.
 *
 * Warden uses struct-based composition (Player, Enemy, CombatState) rather
 * than a generic ECS.  This header defines the vocabulary types that bridge
 * the two worlds so higher-level engine code can reason about "things in the
 * world" without depending on concrete game structs.
 *
 * To add a new entity type:
 *   1. Define a struct for its data in src/game/game_shared.h.
 *   2. Implement its lifecycle in src/game/ (init, update, destroy).
 *   3. Store instances inside GameState (or a dedicated manager).
 *   4. Optionally tag them with an EngineEntityKind for generic code.
 */

#include <stdint.h>
#include <stdbool.h>

/* Unique runtime id for a live entity (0 == invalid). */
typedef uint32_t EngineEntityId;

typedef enum EngineEntityKind {
    ENGINE_ENTITY_NONE   = 0,
    ENGINE_ENTITY_PLAYER = 1,
    ENGINE_ENTITY_ENEMY  = 2,
    ENGINE_ENTITY_NPC    = 3
} EngineEntityKind;

/* Minimum tag carried by every game entity. */
typedef struct EngineEntityTag {
    EngineEntityId   id;
    EngineEntityKind kind;
    bool             alive;
} EngineEntityTag;

#endif /* ENGINE_ENTITY_H */
