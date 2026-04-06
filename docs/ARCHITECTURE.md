# Warden — Engine Architecture

## Directory Layout

```
Warden/
├── include/                 Public headers (safe to include from outside src/)
│   ├── engine/              Engine subsystem interfaces
│   │   ├── engine.h         Master include (pulls in all engine headers)
│   │   ├── audio.h          Audio subsystem (stub — no audio yet)
│   │   ├── core_loop.h      Timing primitives (wall clock, sleep)
│   │   ├── entity.h         Entity tag / component vocabulary
│   │   ├── input.h          Abstract input event types and key constants
│   │   ├── physics.h        Discrete movement and damage-resolution types
│   │   ├── render.h         Renderer configuration interface
│   │   ├── resource.h       Asset load / cache interface (stub)
│   │   ├── scene.h          Scene / game-state lifecycle callbacks
│   │   └── utils.h          Pure math, string, and RNG utilities
│   └── game/
│       └── game.h           Single public entry-point: rpg_run()
│
├── src/                     All implementation files
│   ├── engine/              Engine module implementations
│   │   ├── audio.c          Audio stub (always returns "unavailable")
│   │   ├── core_loop.c      engine_wall_ms(), engine_sleep_ms()
│   │   ├── resource.c       Resource stub (always returns NULL)
│   │   └── utils.c          clamp_int, roll_range, is_blank, …
│   ├── game/                Game-specific logic (uses engine + Feather + ncurses)
│   │   ├── game_shared.h    Internal aggregate header (not public)
│   │   ├── game_tui.h       ncurses TUI types (internal)
│   │   ├── game.c           Main game loop, startup screen, input dispatch
│   │   ├── state.c          GameState lifecycle, save/load, Feather tasks
│   │   ├── world.c          Static zone/weather/fragment data tables
│   │   ├── ui.c             In-game display helpers (describe_zone, show_stats…)
│   │   ├── combat.c         Enemy generation and battle resolution
│   │   ├── actions.c        Player actions (hunt, gather, shop, forge, rest…)
│   │   └── game_tui.c       ncurses TUI renderer and input reader
│   └── main.c               Program entry point — calls rpg_run()
│
├── tests/
│   └── test_utils.c         Unit tests for engine/utils (no Feather/ncurses)
│
├── docs/
│   └── ARCHITECTURE.md      This file
│
├── Feather/                 Git submodule — scheduler library
├── CMakeLists.txt           Build system
└── Makefile                 Convenience wrappers around cmake
```

---

## Module Dependency Diagram

```
src/main.c
    └── include/game/game.h  ──────────────────────┐
                                                    │
src/game/game.c  (rpg_run)                         │
    ├── src/game/game_shared.h (internal)           │
    │       ├── Feather.h  (submodule)              │
    │       ├── src/game/game_tui.h  (internal)     │
    │       └── include/engine/utils.h  ◄────────── warden_engine
    ├── src/game/state.c    (GameState, save/load)
    ├── src/game/world.c    (kZones[], kWeatherNames[])
    ├── src/game/ui.c       (describe_zone, show_stats…)
    ├── src/game/combat.c   (run_battle, build_*_enemy)
    ├── src/game/actions.c  (hunt, gather, shop…)
    └── src/game/game_tui.c (ncurses renderer)

warden_engine  (static library, no external deps)
    ├── src/engine/utils.c    ← include/engine/utils.h
    ├── src/engine/core_loop.c← include/engine/core_loop.h
    ├── src/engine/audio.c    ← include/engine/audio.h   (stub)
    └── src/engine/resource.c ← include/engine/resource.h (stub)

tests/test_utils.c
    └── warden_engine  (only dep — no Feather, no ncurses)
```

---

## Adding a New Game State / Scene

Warden's main loop drives a single implicit scene (gameplay).  To add a new
scene (e.g. a main-menu or cutscene):

1. **Define the scene struct** in `src/game/game_shared.h`:
   ```c
   typedef struct MainMenuState { int selected_option; } MainMenuState;
   ```

2. **Implement lifecycle callbacks** in a new file, e.g. `src/game/scene_menu.c`:
   ```c
   static void menu_on_enter(EngineSceneContext *ctx) { ... }
   static void menu_on_update(EngineSceneContext *ctx) { ... }
   static void menu_on_exit(EngineSceneContext *ctx)   { ... }

   const EngineScene kSceneMainMenu = {
       .id        = ENGINE_SCENE_GAMEPLAY,   /* extend enum as needed */
       .name      = "main_menu",
       .on_enter  = menu_on_enter,
       .on_update = menu_on_update,
       .on_exit   = menu_on_exit,
   };
   ```

3. **Push/pop the scene** in `src/game/game.c` (e.g. in `rpg_run()` before the
   main loop, or on a "quit to menu" command).

4. **Register the new source file** in `CMakeLists.txt` under `add_executable(warden …)`.

---

## Adding a New Entity Type

1. **Define a data struct** in `src/game/game_shared.h` (e.g. `NpcData`).
2. **Add an `ENGINE_ENTITY_NPC` (or similar) tag** in `include/engine/entity.h` if
   generic code needs to reference the entity kind.
3. **Store instances** inside `GameState` (or a separate manager struct).
4. **Implement init/update/destroy** in a new `src/game/` file.
5. **Wire into the main loop** in `src/game/game.c` or `state.c`.

---

## Adding a New Engine Module

1. Create `include/engine/<module>.h` with the public interface.
2. Create `src/engine/<module>.c` with the implementation.
3. Add `src/engine/<module>.c` to the `warden_engine` target in `CMakeLists.txt`.
4. Include the new header in `include/engine/engine.h` (the master include).
5. Write tests in `tests/` that link against `warden_engine` only.
