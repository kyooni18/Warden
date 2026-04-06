#ifndef GAME_GAME_H
#define GAME_GAME_H

/*
 * game/game.h — Public game entry-point.
 *
 * This is the only header external callers (e.g. main.c) need to include.
 * All internal game and engine implementation details are hidden in
 * src/game/ and src/engine/.
 */

/*
 * Initialise the engine, run the game loop, and clean up before returning.
 * Returns 0 on success, non-zero on fatal initialisation failure.
 */
int rpg_run(void);

#endif /* GAME_GAME_H */
