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
