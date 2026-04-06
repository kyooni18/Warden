#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

/*
 * engine/render.h — Rendering subsystem interface.
 *
 * Warden's renderer is a terminal UI built on ncurses (implemented in
 * src/game/game_tui.c).  This header defines the abstract interface so
 * future renderers (SDL, HTML canvas, …) can be swapped in by replacing the
 * implementation file without touching game logic.
 *
 * The concrete TUI types (TuiState, WINDOW*, …) are kept private inside
 * src/game/game_tui.h and game_tui.c.
 */

#include <stdbool.h>

/* Opaque handle to a renderer instance. */
typedef void EngineRenderer;

typedef struct EngineRenderConfig {
    int  target_fps;   /* desired frame rate; 0 = unlimited */
    int  cols;         /* terminal columns (0 = auto-detect) */
    int  rows;         /* terminal rows    (0 = auto-detect) */
    bool color;        /* enable color output if available   */
} EngineRenderConfig;

#define ENGINE_RENDER_DEFAULTS { .target_fps = 20, .cols = 0, .rows = 0, .color = true }

#endif /* ENGINE_RENDER_H */
