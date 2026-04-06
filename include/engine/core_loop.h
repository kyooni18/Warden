#ifndef ENGINE_CORE_LOOP_H
#define ENGINE_CORE_LOOP_H

/*
 * engine/core_loop.h — Core game-loop timing primitives.
 *
 * Provides wall-clock time, a step-cap (to handle debugger pauses), and the
 * target tick rate configuration used by the main loop.
 */

#include <stdint.h>

/* Configuration for the engine main loop. */
typedef struct EngineLoopConfig {
    int    target_hz;         /* desired update frequency (e.g. 20)      */
    uint64_t max_step_ms;    /* largest time step to apply per tick (ms) */
} EngineLoopConfig;

/* Default configuration used by Warden's main loop. */
#define ENGINE_LOOP_DEFAULTS { .target_hz = 20, .max_step_ms = 200 }

/*
 * Return the current wall-clock time in milliseconds (monotonic clock).
 * Safe to call from any thread; suitable as a Feather time-source.
 */
uint64_t engine_wall_ms(void);

/*
 * Sleep for approximately ms milliseconds.
 * Used between ticks to yield CPU without busy-waiting.
 */
void engine_sleep_ms(uint64_t ms);

#endif /* ENGINE_CORE_LOOP_H */
