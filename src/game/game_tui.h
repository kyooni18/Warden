#ifndef RPG_GAME_TUI_H
#define RPG_GAME_TUI_H

#include <ncurses.h>
#include <stdarg.h>
#include <stdbool.h>

/* Forward declaration — full definition is in game_shared.h */
struct GameState;

/* ---- Log ring-buffer capacity ---- */
#define TUI_LOG_CAP      512
#define TUI_LOG_LINE_LEN 256

/* ---- ncurses color pair indices ---- */
#define CP_NORMAL  1   /* white on default bg          */
#define CP_HP_GOOD 2   /* green  — HP > 50%            */
#define CP_HP_MED  3   /* yellow — HP 26–50%           */
#define CP_HP_LOW  4   /* red    — HP ≤ 25%            */
#define CP_GOLD    5   /* yellow — gold / XP gain      */
#define CP_DOOM    6   /* red    — doom meter          */
#define CP_HEADER  7   /* cyan   — title bar           */
#define CP_WORLD   8   /* cyan   — [세계] prefix       */
#define CP_ZONE    9   /* cyan   — zone name           */
#define CP_DIM    10   /* dim    — undiscovered zone   */
#define CP_PLAYER 11   /* yellow bold — player marker  */
#define CP_XP     12   /* blue   — XP bar              */
#define CP_COMBAT 13   /* red bold — combat damage     */

/* ---- TUI window state ---- */
typedef struct TuiState {
    /* ncurses windows */
    WINDOW *win_header;  /* row 0: title bar                    */
    WINDOW *win_map;     /* upper-left: 6×4 zone grid           */
    WINDOW *win_info;    /* upper-right: player stats/zone info */
    WINDOW *win_log;     /* lower section: scrolling event log  */
    WINDOW *win_input;   /* bottom row: command input bar       */

    /* Log ring-buffer */
    char log_buf[TUI_LOG_CAP][TUI_LOG_LINE_LEN];
    int  log_head;   /* index of oldest valid line  */
    int  log_count;  /* number of valid lines stored */

    /* Command input buffer (main loop; combat uses its own local buf) */
    char input_buf[256];
    int  input_len;

    /* Terminal / layout dimensions */
    int rows;
    int cols;
    int upper_rows;   /* rows for map + info panels */
    int log_rows;     /* rows for the log panel     */
    int map_cols;     /* column width of map panel  */
    int visible_rows; /* rows guaranteed visible (mobile-safe) */
    bool mobile_vertical;

    bool initialized;
    bool combat_mode; /* true while run_battle() is executing */
} TuiState;

/* ---- Lifecycle ---- */
bool tui_init(TuiState *tui);
void tui_deinit(TuiState *tui);
void tui_handle_resize(TuiState *tui);

/* ---- Logging ---- */
/* Append a (possibly newline-containing) string to the scrolling log. */
void tui_append_log(TuiState *tui, const char *text);

/* ---- Panel drawing ---- */
void tui_refresh_all(TuiState *tui, const struct GameState *game);
void tui_draw_map(TuiState *tui, const struct GameState *game);
void tui_draw_info(TuiState *tui, const struct GameState *game);
void tui_draw_log(TuiState *tui);
void tui_draw_input(TuiState *tui, const char *prompt, const char *buf, int buf_len);
void tui_draw_combat_overlay(TuiState *tui, const struct GameState *game);
/* Travel animation overlay shown when moving between zones (~600 ms). */
void tui_draw_travel_animation(TuiState *tui, const struct GameState *game,
                               int dest_zone);

/* Helper: draw a filled/empty progress bar inside window win. */
void tui_draw_bar(WINDOW *win, int y, int x, int w,
                  int cur, int max, int cp_fill, int cp_empty);

/* ---- Global TUI accessor ---- */
/* Set once at startup; allows game logic to log without passing TuiState. */
void      tui_set_global(TuiState *tui);
TuiState *tui_get_global(void);

/* ---- printf replacement ---- */
/* All printf() calls in game source files are routed here via the macro
   defined at the bottom of game_shared.h.  When the TUI is active the text
   is appended to the scrolling log; otherwise it falls back to stdout. */
int game_printf(const char *fmt, ...);

#endif /* RPG_GAME_TUI_H */
