#ifndef ENGINE_SCENE_H
#define ENGINE_SCENE_H

/*
 * engine/scene.h — Scene / game-state management interface.
 *
 * A Scene represents a discrete game mode (main menu, gameplay, pause,
 * game-over, credits, …).  The engine drives a stack of scenes; only the
 * top scene receives updates and rendering calls.
 *
 * Warden currently maps to a single SCENE_GAMEPLAY state.  New states (e.g.
 * a main-menu, a cutscene) can be added by registering additional SceneDesc
 * entries and pushing/popping them on the scene stack.
 */

#include <stdbool.h>

/* Opaque forward declaration — game code fills in the actual context. */
typedef void EngineSceneContext;

typedef enum EngineSceneId {
    ENGINE_SCENE_NONE     = 0,
    ENGINE_SCENE_GAMEPLAY = 1,
    ENGINE_SCENE_PAUSE    = 2,
    ENGINE_SCENE_GAMEOVER = 3
} EngineSceneId;

/* Lifecycle callbacks for a scene. */
typedef struct EngineScene {
    EngineSceneId id;
    const char   *name;

    /* Called when the scene becomes active. */
    void (*on_enter)(EngineSceneContext *ctx);

    /* Called once per tick while the scene is active. */
    void (*on_update)(EngineSceneContext *ctx);

    /* Called when the scene is deactivated (but not destroyed). */
    void (*on_exit)(EngineSceneContext *ctx);
} EngineScene;

#endif /* ENGINE_SCENE_H */
