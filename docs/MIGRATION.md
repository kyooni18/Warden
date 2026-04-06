# Migration Note — Modular Engine Refactor

## What changed

| Before | After | Notes |
|--------|-------|-------|
| `RPG/main.c` | `src/main.c` | Entry point — now includes `game/game.h` |
| `RPG/game.h` | `include/game/game.h` | Public API header (same `rpg_run()` signature) |
| `RPG/game.c` | `src/game/game.c` | Main loop, input dispatch, startup screen |
| `RPG/game_shared.h` | `src/game/game_shared.h` | Internal aggregate header; now includes `engine/utils.h` instead of redeclaring utilities |
| `RPG/game_utils.c` | `src/engine/utils.c` | Moved to the engine layer; includes `engine/utils.h` only (no Feather/ncurses) |
| `RPG/game_state.c` | `src/game/state.c` | GameState lifecycle, Feather tasks, save/load |
| `RPG/game_world.c` | `src/game/world.c` | Static zone/weather data tables |
| `RPG/game_ui.c` | `src/game/ui.c` | In-game display helpers |
| `RPG/game_combat.c` | `src/game/combat.c` | Enemy generation and combat resolution |
| `RPG/game_actions.c` | `src/game/actions.c` | Player actions (hunt, gather, shop …) |
| `RPG/game_tui.c` | `src/game/game_tui.c` | ncurses TUI renderer |
| `RPG/game_tui.h` | `src/game/game_tui.h` | Internal TUI types |

## New files

| File | Purpose |
|------|---------|
| `include/engine/engine.h` | Master engine header |
| `include/engine/utils.h` | Pure utility function declarations |
| `include/engine/core_loop.h` | Timing/loop configuration interface |
| `include/engine/scene.h` | Scene lifecycle callbacks interface |
| `include/engine/entity.h` | Entity tag vocabulary |
| `include/engine/input.h` | Abstract input event constants |
| `include/engine/render.h` | Renderer configuration interface |
| `include/engine/audio.h` | Audio interface (stub) |
| `include/engine/resource.h` | Resource management interface (stub) |
| `include/engine/physics.h` | Movement and damage-resolution types |
| `src/engine/core_loop.c` | `engine_wall_ms()`, `engine_sleep_ms()` |
| `src/engine/audio.c` | Audio stub implementation |
| `src/engine/resource.c` | Resource stub implementation |
| `tests/test_utils.c` | Unit tests for engine/utils |
| `docs/ARCHITECTURE.md` | Full module diagram and extension guide |
| `.github/workflows/ci.yml` | GitHub Actions CI (build + test) |

## Build system changes

- `CMakeLists.txt` now defines a `warden_engine` static library (engine modules only, no Feather/ncurses) and links it into the `warden` executable.
- A `test_utils` test executable is added, linked only against `warden_engine`.
- `ctest` is enabled; `make test` runs all tests.
- Source file list updated from `RPG/*.c` to `src/engine/*.c` + `src/game/*.c` + `src/main.c`.

## Behaviour

No gameplay behaviour has changed.  All game logic files were moved verbatim;
only `#include` paths were updated where necessary (the source files in
`src/game/` still use `#include "game_shared.h"` which resolves to
`src/game/game_shared.h` via the CMake include-directory setting).

## How to port a project that used the old RPG/ headers

Replace:
```c
#include "RPG/game.h"
```
with:
```c
#include "game/game.h"
```
The `rpg_run()` signature is identical.
