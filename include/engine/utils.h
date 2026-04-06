#ifndef ENGINE_UTILS_H
#define ENGINE_UTILS_H

/*
 * engine/utils.h — Pure utility functions (math, strings, RNG).
 *
 * These functions have no dependency on game state, ncurses, or Feather.
 * They can be compiled and tested in isolation.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* ---- Math ---- */

/* Clamp value to [min_value, max_value]. */
int clamp_int(int value, int min_value, int max_value);

/* Convert game-minutes to milliseconds (1 game-minute == GAME_MINUTE_MS ms). */
uint64_t minutes_to_ms(int minutes);

/* Return a random integer in [min_value, max_value] (inclusive). */
int roll_range(int min_value, int max_value);

/* ---- Strings ---- */

/* Return true if text is NULL, empty, or all whitespace. */
bool is_blank(const char *text);

/*
 * Lower-case, collapse internal whitespace, and trim leading/trailing spaces
 * from input, writing the result into output (at most output_size bytes
 * including the NUL terminator).
 */
void canonicalize_input(const char *input, char *output, size_t output_size);

/* Return true if text begins with prefix. */
bool starts_with(const char *text, const char *prefix);

#endif /* ENGINE_UTILS_H */
