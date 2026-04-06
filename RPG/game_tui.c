#define _POSIX_C_SOURCE 200809L
/* game_tui.c — ncurses-based TUI for Warden real-time mode.
 *
 * NOTE: This file intentionally does NOT use printf().  The global
 * game_printf() function is defined here; it must call vprintf() (the real
 * stdio function) as a fallback — not printf() itself — to avoid infinite
 * recursion through the macro defined in game_shared.h.
 */
/* Prevent game_shared.h from redefining printf in this file, since we are
 * the ones defining game_printf(). */
#define GAME_TUI_NO_PRINTF_REDIRECT
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "game_tui.h"
/* game_shared.h is included after game_tui.h so GameState is fully defined */
#include "game_shared.h"

/* ---- Layout constants ---- */
#define HEADER_ROWS   1
#define INPUT_ROWS    1
#define MIN_LOG_ROWS  5
#define MIN_UPPER_ROWS 12
/* Each map cell: "[ XX ]" where XX = 2 Korean chars (4 display cols).
 * Display width = 2+4+2 = 8.  Plus "--" connector (2) = 10 per cell.
 * 4 cells: 4*8 + 3*2 = 38 inner cols.  Box border adds 2 → 40 map_cols. */
#define MAP_CELL_W    8   /* display width of one zone cell       */
#define MAP_STRIDE   10   /* cell + connector width (except last) */
#define MAP_INNER_W  38   /* 4*8 + 3*2                            */
#define MAP_COLS     40   /* MAP_INNER_W + 2 (box border)         */

/* ---- Global TUI pointer ---- */
static TuiState *g_tui = NULL;

void tui_set_global(TuiState *tui)  { g_tui = tui; }
TuiState *tui_get_global(void)      { return g_tui; }

/* ---- Internal: compute layout from current terminal dimensions ---- */
static void compute_layout(TuiState *tui)
{
    getmaxyx(stdscr, tui->rows, tui->cols);

    tui->map_cols = MAP_COLS;
    if (tui->cols < MAP_COLS + 20) {
        /* Terminal too narrow — shrink map panel proportionally */
        tui->map_cols = tui->cols / 2;
    }

    int usable = tui->rows - HEADER_ROWS - INPUT_ROWS;
    /* Give log roughly one third of usable rows */
    tui->log_rows = usable / 3;
    if (tui->log_rows < MIN_LOG_ROWS)    tui->log_rows = MIN_LOG_ROWS;
    tui->upper_rows = usable - tui->log_rows;
    if (tui->upper_rows < MIN_UPPER_ROWS) tui->upper_rows = MIN_UPPER_ROWS;
}

/* ---- Internal: create all ncurses windows ---- */
static void create_windows(TuiState *tui)
{
    int info_cols = tui->cols - tui->map_cols;
    int log_start = HEADER_ROWS + tui->upper_rows;
    int input_row = tui->rows - INPUT_ROWS;

    tui->win_header = newwin(HEADER_ROWS, tui->cols, 0, 0);
    tui->win_map    = newwin(tui->upper_rows, tui->map_cols,
                             HEADER_ROWS, 0);
    tui->win_info   = newwin(tui->upper_rows, info_cols,
                             HEADER_ROWS, tui->map_cols);
    tui->win_log    = newwin(tui->log_rows, tui->cols, log_start, 0);
    tui->win_input  = newwin(INPUT_ROWS, tui->cols, input_row, 0);
}

/* ---- tui_init ---- */
bool tui_init(TuiState *tui)
{
    setlocale(LC_ALL, "");
    memset(tui, 0, sizeof(*tui));

    if (initscr() == NULL) return false;
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_NORMAL,  COLOR_WHITE,   -1);
        init_pair(CP_HP_GOOD, COLOR_GREEN,   -1);
        init_pair(CP_HP_MED,  COLOR_YELLOW,  -1);
        init_pair(CP_HP_LOW,  COLOR_RED,     -1);
        init_pair(CP_GOLD,    COLOR_YELLOW,  -1);
        init_pair(CP_DOOM,    COLOR_RED,     -1);
        init_pair(CP_HEADER,  COLOR_CYAN,    -1);
        init_pair(CP_WORLD,   COLOR_CYAN,    -1);
        init_pair(CP_ZONE,    COLOR_CYAN,    -1);
        init_pair(CP_DIM,     COLOR_WHITE,   -1);
        init_pair(CP_PLAYER,  COLOR_YELLOW,  -1);
        init_pair(CP_XP,      COLOR_BLUE,    -1);
        init_pair(CP_COMBAT,  COLOR_RED,     -1);
    }

    compute_layout(tui);
    create_windows(tui);
    tui->initialized = true;
    return true;
}

/* ---- tui_deinit ---- */
void tui_deinit(TuiState *tui)
{
    if (!tui->initialized) return;
    if (tui->win_header) { delwin(tui->win_header); tui->win_header = NULL; }
    if (tui->win_map)    { delwin(tui->win_map);    tui->win_map    = NULL; }
    if (tui->win_info)   { delwin(tui->win_info);   tui->win_info   = NULL; }
    if (tui->win_log)    { delwin(tui->win_log);    tui->win_log    = NULL; }
    if (tui->win_input)  { delwin(tui->win_input);  tui->win_input  = NULL; }
    endwin();
    tui->initialized = false;
}

/* ---- tui_handle_resize ---- */
void tui_handle_resize(TuiState *tui)
{
    if (!tui->initialized) return;
    if (tui->win_header) delwin(tui->win_header);
    if (tui->win_map)    delwin(tui->win_map);
    if (tui->win_info)   delwin(tui->win_info);
    if (tui->win_log)    delwin(tui->win_log);
    if (tui->win_input)  delwin(tui->win_input);
    endwin();
    refresh();
    clear();
    compute_layout(tui);
    create_windows(tui);
}

/* ---- tui_append_log ---- */
/* Splits 'text' on '\n' and stores each non-empty line in the ring buffer. */
void tui_append_log(TuiState *tui, const char *text)
{
    if (!tui) return;

    /* Work on a copy so we can tokenise */
    char buf[TUI_LOG_LINE_LEN * 4];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *start = buf;
    char *p;
    while ((p = strchr(start, '\n')) != NULL) {
        *p = '\0';
        if (start[0] != '\0') {
            int idx = (tui->log_head + tui->log_count) % TUI_LOG_CAP;
            strncpy(tui->log_buf[idx], start, TUI_LOG_LINE_LEN - 1);
            tui->log_buf[idx][TUI_LOG_LINE_LEN - 1] = '\0';
            if (tui->log_count < TUI_LOG_CAP) {
                tui->log_count++;
            } else {
                /* Ring is full: advance head to overwrite oldest */
                tui->log_head = (tui->log_head + 1) % TUI_LOG_CAP;
            }
        }
        start = p + 1;
    }
    /* Trailing text without newline */
    if (start[0] != '\0') {
        int idx = (tui->log_head + tui->log_count) % TUI_LOG_CAP;
        strncpy(tui->log_buf[idx], start, TUI_LOG_LINE_LEN - 1);
        tui->log_buf[idx][TUI_LOG_LINE_LEN - 1] = '\0';
        if (tui->log_count < TUI_LOG_CAP) {
            tui->log_count++;
        } else {
            tui->log_head = (tui->log_head + 1) % TUI_LOG_CAP;
        }
    }
}

/* ---- game_printf — printf replacement ---- */
int game_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    TuiState *tui = tui_get_global();
    if (tui && tui->initialized) {
        char tmp[TUI_LOG_LINE_LEN * 4];
        vsnprintf(tmp, sizeof(tmp), fmt, ap);
        tui_append_log(tui, tmp);
    } else {
        /* TUI not up yet — fall back to real stdio */
        vprintf(fmt, ap);
    }

    va_end(ap);
    return 0;
}

/* ---- tui_draw_bar ---- */
void tui_draw_bar(WINDOW *win, int y, int x, int w,
                  int cur, int max, int cp_fill, int cp_empty)
{
    if (!win || w <= 0 || max <= 0) return;
    int filled = (cur * w) / max;
    if (filled > w) filled = w;
    if (filled < 0) filled = 0;

    int col;
    for (col = 0; col < w; col++) {
        if (col < filled) {
            wattron(win, COLOR_PAIR(cp_fill) | A_BOLD);
            mvwaddch(win, y, x + col, ACS_BLOCK);
            wattroff(win, COLOR_PAIR(cp_fill) | A_BOLD);
        } else {
            wattron(win, COLOR_PAIR(cp_empty) | A_DIM);
            mvwaddch(win, y, x + col, ACS_CKBOARD);
            wattroff(win, COLOR_PAIR(cp_empty) | A_DIM);
        }
    }
}

/* ---- tui_draw_map ---- */
void tui_draw_map(TuiState *tui, const struct GameState *game)
{
    WINDOW *w = tui->win_map;
    if (!w) return;

    werase(w);
    box(w, 0, 0);

    /* Panel title */
    wattron(w, COLOR_PAIR(CP_ZONE) | A_BOLD);
    mvwaddstr(w, 0, 2, " 세계 지도 ");
    wattroff(w, COLOR_PAIR(CP_ZONE) | A_BOLD);

    int row, col;
    for (row = 0; row < 6; row++) {
        int y = 1 + row * 2; /* 2 display rows per grid row (cells + connectors) */
        if (y >= tui->upper_rows - 1) break;

        for (col = 0; col < 4; col++) {
            int zone_id = row * 4 + col;
            /* x: col 1 (border), then col*MAP_STRIDE for each cell */
            int x = 1 + col * MAP_STRIDE;
            if (x + MAP_CELL_W >= tui->map_cols - 1) break;

            bool is_player     = (zone_id == game->player.zone);
            bool is_discovered = game->player.discovered[zone_id];
            const char *label  = is_discovered ? kZones[zone_id].short_name : "????";

            if (is_player) {
                wattron(w, COLOR_PAIR(CP_PLAYER) | A_BOLD);
                mvwprintw(w, y, x, "[*%s*]", label);
                wattroff(w, COLOR_PAIR(CP_PLAYER) | A_BOLD);
            } else if (is_discovered) {
                wattron(w, COLOR_PAIR(CP_NORMAL));
                mvwprintw(w, y, x, "[ %s ]", label);
                wattroff(w, COLOR_PAIR(CP_NORMAL));
            } else {
                wattron(w, COLOR_PAIR(CP_DIM) | A_DIM);
                mvwaddstr(w, y, x, "[ ?? ]");
                wattroff(w, COLOR_PAIR(CP_DIM) | A_DIM);
            }

            /* Horizontal connector to the right */
            if (col < 3 && x + MAP_CELL_W + 2 < tui->map_cols - 1) {
                mvwaddstr(w, y, x + MAP_CELL_W, "--");
            }
        }

        /* Vertical connectors between grid rows */
        if (row < 5 && y + 1 < tui->upper_rows - 1) {
            for (col = 0; col < 4; col++) {
                int zone_id = row * 4 + col;
                bool top = game->player.discovered[zone_id];
                bool bot = game->player.discovered[zone_id + 4];
                if (!top && !bot) continue;
                /* Draw connector at horizontal centre of the cell */
                int cx = 1 + col * MAP_STRIDE + MAP_CELL_W / 2;
                if (cx < tui->map_cols - 1) {
                    wattron(w, COLOR_PAIR(CP_DIM) | A_DIM);
                    mvwaddch(w, y + 1, cx, ACS_VLINE);
                    wattroff(w, COLOR_PAIR(CP_DIM) | A_DIM);
                }
            }
        }
    }

    wnoutrefresh(w);
}

/* ---- tui_draw_info ---- */
/* Shows player stats and current zone info in the right-hand panel. */
void tui_draw_info(TuiState *tui, const struct GameState *game)
{
    WINDOW *w = tui->win_info;
    if (!w) return;

    int rows, cols;
    getmaxyx(w, rows, cols);

    werase(w);
    box(w, 0, 0);

    if (tui->combat_mode) {
        /* ---- Combat overlay in the info panel ---- */
        wattron(w, COLOR_PAIR(CP_COMBAT) | A_BOLD);
        mvwprintw(w, 0, 2, " 전투 중 ");
        wattroff(w, COLOR_PAIR(CP_COMBAT) | A_BOLD);

        int y = 1;
        /* Enemy */
        wattron(w, COLOR_PAIR(CP_COMBAT) | A_BOLD);
        mvwprintw(w, y++, 1, "적: %s", game->combat.enemy.name);
        wattroff(w, COLOR_PAIR(CP_COMBAT) | A_BOLD);

        int bar_w = cols - 4;
        if (bar_w > 20) bar_w = 20;
        tui_draw_bar(w, y, 1, bar_w,
                     game->combat.enemy.hp, game->combat.enemy.max_hp,
                     CP_HP_LOW, CP_DIM);
        mvwprintw(w, y++, bar_w + 2, "%d/%d",
                  game->combat.enemy.hp, game->combat.enemy.max_hp);

        /* Enemy status */
        char estatus[64] = "";
        if (game->combat.enemy_burn_turns > 0)
            snprintf(estatus + strlen(estatus),
                     sizeof(estatus) - strlen(estatus),
                     " [화상%d]", game->combat.enemy_burn_turns);
        if (game->combat.enemy_stun_turns > 0)
            snprintf(estatus + strlen(estatus),
                     sizeof(estatus) - strlen(estatus),
                     " [기절%d]", game->combat.enemy_stun_turns);
        if (game->combat.enemy_charging)
            snprintf(estatus + strlen(estatus),
                     sizeof(estatus) - strlen(estatus),
                     " [충전중]");
        if (estatus[0] != '\0') {
            wattron(w, COLOR_PAIR(CP_HP_LOW));
            mvwprintw(w, y++, 1, "%s", estatus);
            wattroff(w, COLOR_PAIR(CP_HP_LOW));
        } else {
            y++;
        }

        y++; /* separator */

        /* Player HP */
        wattron(w, COLOR_PAIR(CP_NORMAL) | A_BOLD);
        mvwprintw(w, y++, 1, "%.20s", game->player.name);
        wattroff(w, COLOR_PAIR(CP_NORMAL) | A_BOLD);

        int hp_cp = CP_HP_GOOD;
        if (game->player.hp * 4 <= game->player.max_hp)
            hp_cp = CP_HP_LOW;
        else if (game->player.hp * 2 <= game->player.max_hp)
            hp_cp = CP_HP_MED;

        tui_draw_bar(w, y, 1, bar_w,
                     game->player.hp, game->player.max_hp,
                     hp_cp, CP_DIM);
        mvwprintw(w, y++, bar_w + 2, "%d/%d",
                  game->player.hp, game->player.max_hp);

        /* Player status */
        char pstatus[64] = "";
        if (game->combat.player_bleed_turns > 0)
            snprintf(pstatus + strlen(pstatus),
                     sizeof(pstatus) - strlen(pstatus),
                     " [출혈%d]", game->combat.player_bleed_turns);
        if (game->combat.weaken_turns > 0)
            snprintf(pstatus + strlen(pstatus),
                     sizeof(pstatus) - strlen(pstatus),
                     " [약화%d]", game->combat.weaken_turns);
        if (game->combat.guard_active)
            snprintf(pstatus + strlen(pstatus),
                     sizeof(pstatus) - strlen(pstatus), " [방어]");
        if (game->combat.parry_active)
            snprintf(pstatus + strlen(pstatus),
                     sizeof(pstatus) - strlen(pstatus), " [막기]");
        if (game->combat.holy_barrier_active)
            snprintf(pstatus + strlen(pstatus),
                     sizeof(pstatus) - strlen(pstatus), " [신성방벽]");
        if (pstatus[0] != '\0') {
            wattron(w, COLOR_PAIR(CP_HP_MED));
            mvwprintw(w, y++, 1, "%s", pstatus);
            wattroff(w, COLOR_PAIR(CP_HP_MED));
        } else {
            y++;
        }

        y++; /* separator */

        /* Action menu */
        if (y < rows - 2) {
            wattron(w, COLOR_PAIR(CP_ZONE) | A_BOLD);
            mvwaddstr(w, y++, 1, "행동:");
            wattroff(w, COLOR_PAIR(CP_ZONE) | A_BOLD);
        }
        const char *actions[8][2] = {
            {"1", "공격(attack)"},
            {"2", "방어(guard)"},
            {NULL, NULL},
            {"4", "도주(flee)"},
            {"5", "포션"},
            {"6", "폭탄"},
            {"7", "성수"},
            {NULL, NULL}
        };
        /* slot 3 = class ability */
        const char *class_label = "특기";
        switch (game->player.player_class) {
        case CLASS_WARRIOR: class_label = "3 막기(parry)";   break;
        case CLASS_SCOUT:   class_label = "3 기습(backstab)"; break;
        case CLASS_MAGE:    class_label = "3 화염구(fireball)"; break;
        case CLASS_CLERIC:  class_label = "3 신성(smite)";   break;
        }
        if (y < rows - 2) {
            wattron(w, COLOR_PAIR(CP_NORMAL));
            mvwprintw(w, y++, 1, "%s", class_label);
            wattroff(w, COLOR_PAIR(CP_NORMAL));
        }
        int ai;
        for (ai = 0; ai < 8 && y < rows - 2; ai++) {
            if (actions[ai][0] == NULL) continue;
            mvwprintw(w, y++, 1, "[%s] %s", actions[ai][0], actions[ai][1]);
        }

    } else {
        /* ---- Normal info panel ---- */
        wattron(w, COLOR_PAIR(CP_HEADER) | A_BOLD);
        mvwprintw(w, 0, 2, " 수호자 정보 ");
        wattroff(w, COLOR_PAIR(CP_HEADER) | A_BOLD);

        int y = 1;
        int bar_w = cols - 4;
        if (bar_w > 24) bar_w = 24;

        /* Name + class + level */
        wattron(w, COLOR_PAIR(CP_NORMAL) | A_BOLD);
        mvwprintw(w, y++, 1, "%.18s  %s  Lv.%d",
                  game->player.name,
                  class_name(game->player.player_class),
                  game->player.level);
        wattroff(w, COLOR_PAIR(CP_NORMAL) | A_BOLD);

        /* HP bar */
        if (y < rows - 2) {
            int hp_cp = CP_HP_GOOD;
            if (game->player.hp * 4 <= game->player.max_hp)
                hp_cp = CP_HP_LOW;
            else if (game->player.hp * 2 <= game->player.max_hp)
                hp_cp = CP_HP_MED;

            mvwaddstr(w, y, 1, "HP");
            tui_draw_bar(w, y, 4, bar_w,
                         game->player.hp, game->player.max_hp,
                         hp_cp, CP_DIM);
            wattron(w, COLOR_PAIR(hp_cp));
            mvwprintw(w, y++, 4 + bar_w + 1, "%d/%d",
                      game->player.hp, game->player.max_hp);
            wattroff(w, COLOR_PAIR(hp_cp));
        }

        /* XP bar */
        if (y < rows - 2) {
            mvwaddstr(w, y, 1, "XP");
            tui_draw_bar(w, y, 4, bar_w,
                         game->player.xp, game->player.xp_to_next,
                         CP_XP, CP_DIM);
            wattron(w, COLOR_PAIR(CP_XP));
            mvwprintw(w, y++, 4 + bar_w + 1, "%d/%d",
                      game->player.xp, game->player.xp_to_next);
            wattroff(w, COLOR_PAIR(CP_XP));
        }

        /* Gold + doom */
        if (y < rows - 2) {
            wattron(w, COLOR_PAIR(CP_GOLD));
            mvwprintw(w, y, 1, "골드: %d", game->player.gold);
            wattroff(w, COLOR_PAIR(CP_GOLD));

            int doom_cp = (game->doom >= 7) ? CP_DOOM : CP_HP_MED;
            wattron(w, COLOR_PAIR(doom_cp));
            mvwprintw(w, y++, cols / 2, "파멸: %d/12", game->doom);
            wattroff(w, COLOR_PAIR(doom_cp));
        }

        /* Attack / Defense */
        if (y < rows - 2) {
            mvwprintw(w, y++, 1, "공격:%d  방어:%d  승리:%d",
                      player_attack_value(game),
                      player_defense_value(game),
                      game->player.victories);
        }

        /* Equipment summary */
        if (y < rows - 2) {
            const char *weapon = game->player.titan_blade ? "타이탄검"
                               : game->player.steel_edge  ? "강철칼날" : "여행자검";
            const char *armor  = game->player.ward_mail ? "수호갑옷" : "가죽방어";
            mvwprintw(w, y++, 1, "%.10s / %.10s", weapon, armor);
        }

        /* Class-specific resource */
        if (y < rows - 2) {
            if (game->player.player_class == CLASS_MAGE) {
                wattron(w, COLOR_PAIR(CP_XP));
                mvwprintw(w, y++, 1, "룬 파편: %d", game->player.rune_shards);
                wattroff(w, COLOR_PAIR(CP_XP));
            } else if (game->player.player_class == CLASS_CLERIC ||
                       game->player.holy_water > 0) {
                mvwprintw(w, y++, 1, "성수: %d  포션: %d",
                          game->player.holy_water, game->player.potions);
            } else {
                mvwprintw(w, y++, 1, "포션: %d  폭탄: %d",
                          game->player.potions, game->player.bombs);
            }
        }

        y++; /* separator */

        /* Current zone */
        if (y < rows - 2) {
            const ZoneData *zone = &kZones[game->player.zone];
            wattron(w, COLOR_PAIR(CP_ZONE) | A_BOLD);
            mvwprintw(w, y++, 1, "%.20s", zone->name);
            wattroff(w, COLOR_PAIR(CP_ZONE) | A_BOLD);

            if (y < rows - 2) {
                mvwprintw(w, y++, 1, "날씨: %.10s  위험: %d",
                          kWeatherNames[game->weather],
                          zone->safe ? 0 : zone->danger + game->doom / 3);
            }

            /* Time */
            if (y < rows - 2) {
                mvwprintw(w, y++, 1, "%d일 %02d:%02d %s",
                          current_day(game),
                          current_hour(game),
                          current_minute(game),
                          time_band(game));
            }

            /* Available exits */
            if (y < rows - 2) {
                char exits[32] = "이동: ";
                if (zone_from_direction(game->player.zone, "north") != ZONE_NONE)
                    strcat(exits, "북 ");
                if (zone_from_direction(game->player.zone, "east")  != ZONE_NONE)
                    strcat(exits, "동 ");
                if (zone_from_direction(game->player.zone, "south") != ZONE_NONE)
                    strcat(exits, "남 ");
                if (zone_from_direction(game->player.zone, "west")  != ZONE_NONE)
                    strcat(exits, "서 ");
                mvwaddstr(w, y++, 1, exits);
            }

            /* Merchant / forge indicators */
            if (y < rows - 2 && zone_has_merchant(game, game->player.zone)) {
                wattron(w, COLOR_PAIR(CP_GOLD));
                mvwaddstr(w, y++, 1, "[상점 이용 가능]");
                wattroff(w, COLOR_PAIR(CP_GOLD));
            }
            if (y < rows - 2 && zone->forge) {
                mvwaddstr(w, y++, 1, "[대장간 이용 가능]");
            }
        }
    }

    wnoutrefresh(w);
}

/* ---- tui_draw_log ---- */
void tui_draw_log(TuiState *tui)
{
    WINDOW *w = tui->win_log;
    if (!w) return;

    int rows, cols;
    getmaxyx(w, rows, cols);
    (void)cols;

    werase(w);
    box(w, 0, 0);

    wattron(w, COLOR_PAIR(CP_WORLD) | A_BOLD);
    mvwaddstr(w, 0, 2, " 게임 로그 ");
    wattroff(w, COLOR_PAIR(CP_WORLD) | A_BOLD);

    /* Inner usable rows (exclude box borders) */
    int inner = rows - 2;
    if (inner <= 0) { wnoutrefresh(w); return; }

    /* Show the last 'inner' lines of the ring buffer */
    int count = tui->log_count;
    int start = (count > inner) ? count - inner : 0;

    int row;
    for (row = 0; row < inner && (start + row) < count; row++) {
        int idx = (tui->log_head + start + row) % TUI_LOG_CAP;
        const char *line = tui->log_buf[idx];

        /* Color world-event lines differently */
        if (strncmp(line, "[세계]", 9) == 0) {   /* "[세계]" is 9 UTF-8 bytes */
            wattron(w, COLOR_PAIR(CP_WORLD));
            mvwprintw(w, row + 1, 1, "%.200s", line);
            wattroff(w, COLOR_PAIR(CP_WORLD));
        } else if (strncmp(line, "★", 3) == 0 ||
                   strncmp(line, "◆", 3) == 0 ||
                   strncmp(line, "▲", 3) == 0) {
            wattron(w, COLOR_PAIR(CP_GOLD) | A_BOLD);
            mvwprintw(w, row + 1, 1, "%.200s", line);
            wattroff(w, COLOR_PAIR(CP_GOLD) | A_BOLD);
        } else {
            wattron(w, COLOR_PAIR(CP_NORMAL));
            mvwprintw(w, row + 1, 1, "%.200s", line);
            wattroff(w, COLOR_PAIR(CP_NORMAL));
        }
    }

    wnoutrefresh(w);
}

/* ---- tui_draw_input ---- */
void tui_draw_input(TuiState *tui, const char *prompt,
                    const char *buf, int buf_len)
{
    WINDOW *w = tui->win_input;
    if (!w) return;
    (void)buf_len;

    werase(w);
    wattron(w, COLOR_PAIR(CP_ZONE) | A_BOLD);
    mvwaddstr(w, 0, 0, prompt);
    wattroff(w, COLOR_PAIR(CP_ZONE) | A_BOLD);

    wattron(w, COLOR_PAIR(CP_NORMAL));
    waddstr(w, buf);
    wattroff(w, COLOR_PAIR(CP_NORMAL));

    /* Blinking cursor indicator */
    wattron(w, COLOR_PAIR(CP_PLAYER) | A_BOLD);
    waddch(w, '_');
    wattroff(w, COLOR_PAIR(CP_PLAYER) | A_BOLD);

    wnoutrefresh(w);
}

/* ---- tui_draw_combat_overlay ---- */
/* Called from within run_battle() to refresh the full display during combat. */
void tui_draw_combat_overlay(TuiState *tui, const struct GameState *game)
{
    if (!tui || !tui->initialized) return;
    tui->combat_mode = true;
    tui_draw_map(tui, game);
    tui_draw_info(tui, game);
    tui_draw_log(tui);
    tui_draw_input(tui, "전투> ", tui->input_buf, tui->input_len);
    doupdate();
}

/* ---- tui_draw_header ---- */
static void tui_draw_header(TuiState *tui)
{
    WINDOW *w = tui->win_header;
    if (!w) return;

    int cols = getmaxx(w);

    werase(w);
    wattron(w, COLOR_PAIR(CP_HEADER) | A_BOLD | A_REVERSE);
    mvwhline(w, 0, 0, ' ', cols);
    const char *title = " WARDEN OF THE VOID CROWN — 공허의 왕관의 재 ";
    int tx = (cols - (int)strlen(title)) / 2;
    if (tx < 0) tx = 0;
    mvwaddstr(w, 0, tx, title);
    wattroff(w, COLOR_PAIR(CP_HEADER) | A_BOLD | A_REVERSE);
    wnoutrefresh(w);
}

/* ---- tui_refresh_all ---- */
void tui_refresh_all(TuiState *tui, const struct GameState *game)
{
    if (!tui || !tui->initialized) return;
    tui_draw_header(tui);
    tui_draw_map(tui, game);
    tui_draw_info(tui, game);
    tui_draw_log(tui);
    tui_draw_input(tui, "명령> ", tui->input_buf, tui->input_len);
    doupdate();
}
