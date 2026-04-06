#ifndef ENGINE_PHYSICS_H
#define ENGINE_PHYSICS_H

/*
 * engine/physics.h — Physics / collision interface.
 *
 * Warden's "physics" is discrete grid movement and turn-based combat damage
 * resolution (no continuous simulation).  This header defines the vocabulary
 * types used by those systems so they can be reasoned about independently of
 * the game's concrete types.
 *
 * Spatial collision (zone-grid movement) is handled in game_state.c via
 * zone_from_direction().  Combat damage resolution lives in game_combat.c.
 */

#include <stdbool.h>

/* ---- Grid movement ---- */

typedef enum EngineDirection {
    ENGINE_DIR_NONE  = 0,
    ENGINE_DIR_NORTH = 1,
    ENGINE_DIR_SOUTH = 2,
    ENGINE_DIR_EAST  = 3,
    ENGINE_DIR_WEST  = 4
} EngineDirection;

/* Outcome of a movement attempt. */
typedef enum EngineMoveResult {
    ENGINE_MOVE_OK      = 0,  /* moved successfully       */
    ENGINE_MOVE_BLOCKED = 1,  /* no exit in that direction */
    ENGINE_MOVE_INVALID = 2   /* bad input                 */
} EngineMoveResult;

/* ---- Damage resolution ---- */

typedef struct EngineDamageEvent {
    int  raw_damage;    /* damage before mitigation      */
    int  final_damage;  /* damage after defence & modifiers */
    bool is_critical;
    bool blocked;
} EngineDamageEvent;

#endif /* ENGINE_PHYSICS_H */
