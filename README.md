# CLITextGame

This repository contains a standalone text RPG application that links against
Feather as a library.

## Structure

- `Feather/` contains the reusable scheduler library.
- `RPG/main.c` contains a thin executable entrypoint.
- `RPG/game.c` contains the game application logic.
- `RPG/game.h` declares the game runner interface.

## Build

```sh
make rpg
```

## Run

```sh
make run-rpg
```

The executable is generated at `build/CLITextRPG`.

## Notes

- The game now shows core UI prompts/messages in Korean.
- In-game commands include save/load support:
  - `save` to write progress to `savegame.dat`
  - `load` to restore progress from `savegame.dat`
