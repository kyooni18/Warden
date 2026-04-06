#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H

/*
 * engine/engine.h — Master engine header.
 *
 * Include this single header to pull in all engine subsystem interfaces.
 * Individual subsystem headers may also be included directly when only a
 * subset of the engine API is needed.
 *
 * Subsystem summary
 * -----------------
 *  core_loop  Timing, wall-clock, tick configuration
 *  scene      Scene / game-state stack (gameplay, pause, game-over …)
 *  entity     Entity tags and component vocabulary
 *  input      Abstract input events and key constants
 *  render     Rendering subsystem interface (TUI implementation)
 *  audio      Audio subsystem interface (stub — no audio in current build)
 *  resource   Resource / asset loading and caching (stub)
 *  physics    Discrete grid movement and damage-resolution types
 *  utils      Pure math, string, and RNG utilities
 */

#include "engine/core_loop.h"
#include "engine/scene.h"
#include "engine/entity.h"
#include "engine/input.h"
#include "engine/render.h"
#include "engine/audio.h"
#include "engine/resource.h"
#include "engine/physics.h"
#include "engine/utils.h"

#endif /* ENGINE_ENGINE_H */
