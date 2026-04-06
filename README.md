# Warden — 공허의 왕관의 재

A terminal text-RPG written in C, built on the [Feather](https://github.com/kyooni18/Feather)
task-scheduler library and ncurses.

---

## Quick Start

```sh
git submodule update --init --recursive
make rpg        # build
make run-rpg    # run
make test       # run unit tests (no Feather/ncurses needed)
```

The game binary is produced at `build/Warden`.

---

## Architecture

The codebase is split into two layers:

| Layer | Location | Description |
|-------|----------|-------------|
| **Engine** | `src/engine/`, `include/engine/` | Pure, reusable modules — utilities, timing, audio/resource stubs. No Feather or ncurses dependency. |
| **Game** | `src/game/`, `include/game/` | Warden-specific logic — state, world, combat, actions, TUI renderer. |

### Module overview

```
include/engine/
  engine.h      master include
  utils.h       math, string, RNG helpers
  core_loop.h   wall-clock timing primitives
  scene.h       scene / game-state lifecycle interface
  entity.h      entity tag vocabulary
  input.h       abstract input event constants
  render.h      renderer configuration interface
  audio.h       audio interface (stub — no audio yet)
  resource.h    asset load/cache interface (stub)
  physics.h     grid movement and damage-resolution types

include/game/
  game.h        public API: rpg_run()

src/engine/     implementations of the engine modules above
src/game/       game logic, ncurses TUI, Feather integration
src/main.c      program entry point
tests/          unit tests (warden_engine only, no ncurses/Feather)
docs/           ARCHITECTURE.md — full module diagram and extension guide
```

See **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** for the full module
diagram, dependency graph, and step-by-step guides for:

- Adding a new game state / scene
- Adding a new entity type
- Adding a new engine module

---

## Build targets

| Command | Description |
|---------|-------------|
| `make rpg` | Build the game executable |
| `make run-rpg` | Build and launch the game |
| `make test` | Build and run unit tests |
| `make clean` | Remove the build directory |

---

## Game notes

- UI messages are displayed in Korean (한국어).
- Save / load support: `save` writes `savegame.dat`; `load` restores it.
- Type `help` in-game for the full command list.
