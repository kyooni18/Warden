#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

/*
 * engine/input.h — Input abstraction layer.
 *
 * Wraps the raw key/event source (ncurses getch() in Warden) behind a small
 * set of symbolic constants so game logic does not depend on ncurses key
 * codes directly.
 *
 * The actual polling is performed by the TUI renderer layer (game_tui.c);
 * this header just defines the shared key vocabulary.
 */

/* Symbolic directional keys (mapped from ncurses KEY_UP/DOWN/LEFT/RIGHT). */
#define ENGINE_KEY_UP     0x1001
#define ENGINE_KEY_DOWN   0x1002
#define ENGINE_KEY_LEFT   0x1003
#define ENGINE_KEY_RIGHT  0x1004
#define ENGINE_KEY_ENTER  0x100A
#define ENGINE_KEY_BKSP   0x107F
#define ENGINE_KEY_ERR    (-1)   /* no input available (non-blocking) */

/*
 * Abstract input event carrying a single key press.
 * Extended later with mouse / gamepad support if needed.
 */
typedef struct EngineInputEvent {
    int key;   /* one of ENGINE_KEY_*, or a printable ASCII value */
} EngineInputEvent;

#endif /* ENGINE_INPUT_H */
