#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <signal.h>
#include "game_shared.h"

/* ---- SIGWINCH handler for terminal resize ---- */
#ifdef SIGWINCH
static volatile sig_atomic_t g_resize_pending = 0;
static void handle_sigwinch(int sig) {
  (void)sig;
  g_resize_pending = 1;
}
#endif

/* ---- Startup: class and name selection via ncurses ---- */
static void startup_screen(GameState *game, TuiState *tui)
{
  /* Temporarily enable echo / blocking input for the setup prompts */
  nodelay(stdscr, FALSE);
  echo();
  curs_set(1);

  int rows, cols;
  getmaxyx(stdscr, rows, cols);
  (void)rows;

  clear();

  /* Title */
  attron(COLOR_PAIR(CP_HEADER) | A_BOLD | A_REVERSE);
  mvhline(0, 0, ' ', cols);
  const char *title = " WARDEN OF THE VOID CROWN — 공허의 왕관의 재 ";
  int tx = (cols - (int)strlen(title)) / 2;
  if (tx < 0) tx = 0;
  mvaddstr(0, tx, title);
  attroff(COLOR_PAIR(CP_HEADER) | A_BOLD | A_REVERSE);

  attron(COLOR_PAIR(CP_NORMAL));
  mvaddstr(2, 2, "직업을 선택하세요:");
  mvaddstr(4, 4, "[1] 전사    — 높은 체력/방어.  parry(막기), bash(방패 강타) 특기");
  mvaddstr(5, 4, "[2] 척후    — 빠른 공격/이탈.  backstab(기습), vanish(은신) 특기");
  mvaddstr(6, 4, "[3] 마법사  — 마력 공격.       fireball(화염구), frost(냉기화살) 특기");
  mvaddstr(7, 4, "[4] 성직자  — 치유 중심.       smite(신성 피해+자가치유), holy_barrier(Lv4) 특기");
  attroff(COLOR_PAIR(CP_NORMAL));

  attron(COLOR_PAIR(CP_ZONE) | A_BOLD);
  mvaddstr(9, 2, "선택 (1/2/3/4, 엔터=전사): ");
  attroff(COLOR_PAIR(CP_ZONE) | A_BOLD);
  refresh();

  int ch = getch();
  if (ch == '2') {
    select_class(game, CLASS_SCOUT);
    mvaddstr(10, 2, "척후 선택. 빠른 발과 날카로운 눈으로 길을 개척하십시오.");
  } else if (ch == '3') {
    select_class(game, CLASS_MAGE);
    mvaddstr(10, 2, "마법사 선택. 아는 것이 힘입니다. 룬 파편으로 화염구와 냉기 화살을 사용하세요.");
  } else if (ch == '4') {
    select_class(game, CLASS_CLERIC);
    mvaddstr(10, 2, "성직자 선택. 신성한 빛이 어둠을 밝힙니다.");
  } else {
    select_class(game, CLASS_WARRIOR);
    mvaddstr(10, 2, "전사 선택. 강철이 말하게 하십시오.");
  }

  /* Name input */
  attron(COLOR_PAIR(CP_ZONE) | A_BOLD);
  mvaddstr(12, 2, "수호자의 이름 (빈칸이면 '수호자'): ");
  attroff(COLOR_PAIR(CP_ZONE) | A_BOLD);
  refresh();

  char name_buf[MAX_NAME];
  memset(name_buf, 0, sizeof(name_buf));
  getnstr(name_buf, (int)sizeof(name_buf) - 1);
  /* Strip trailing newline/whitespace */
  int len = (int)strlen(name_buf);
  while (len > 0 && (name_buf[len-1] == '\n' || name_buf[len-1] == '\r' ||
                     name_buf[len-1] == ' '))
    name_buf[--len] = '\0';

  if (len > 0) {
    snprintf(game->player.name, sizeof(game->player.name), "%s", name_buf);
  }

  /* Restore non-blocking mode */
  noecho();
  curs_set(0);
  nodelay(stdscr, TRUE);

  /* Announce arrival in the TUI log */
  tui_append_log(tui, "");
  {
    char msg[128];
    snprintf(msg, sizeof(msg),
             "%s이(가) 저녁 빛이 희미해지는 엠버폴 관문에 도착했습니다.",
             game->player.name);
    tui_append_log(tui, msg);
  }
  tui_append_log(tui, "방향키 또는 n/s/e/w: 이동  |  명령어 입력 후 엔터  |  help: 도움말");
}

/* ---- Non-blocking input dispatch ---- */
static void handle_char(GameState *game, TuiState *tui, int ch)
{
  /* Arrow keys → directional movement */
  const char *dir = NULL;
  switch (ch) {
  case KEY_UP:    dir = "north"; break;
  case KEY_DOWN:  dir = "south"; break;
  case KEY_LEFT:  dir = "west";  break;
  case KEY_RIGHT: dir = "east";  break;
  default: break;
  }
  if (dir) {
    maybe_handle_movement_command(game, dir);
    flush_events(game);
    tui_refresh_all(tui, game);
    return;
  }

  /* Backspace */
  if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
    if (tui->input_len > 0) {
      tui->input_buf[--tui->input_len] = '\0';
    }
    return;
  }

  /* Enter: dispatch the accumulated command */
  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    if (tui->input_len == 0) return;

    char command[MAX_INPUT];
    canonicalize_input(tui->input_buf, command, sizeof(command));
    tui->input_buf[0] = '\0';
    tui->input_len = 0;

    if (command[0] == '\0') return;

    /* Echo the command to the log */
    {
      char echo_line[MAX_INPUT + 8];
      snprintf(echo_line, sizeof(echo_line), "> %s", command);
      tui_append_log(tui, echo_line);
    }

    if (maybe_handle_movement_command(game, command)) {
      /* handled */
    } else if (strcmp(command, "interact") == 0 || strcmp(command, "use") == 0) {
      interact_here(game);
    } else if (strcmp(command, "help") == 0 || strcmp(command, "도움말") == 0) {
      show_help(game);
    } else if (strcmp(command, "look") == 0 || strcmp(command, "l") == 0) {
      describe_zone(game);
    } else if (strcmp(command, "map") == 0) {
      show_map(game);
    } else if (strcmp(command, "stats") == 0 || strcmp(command, "status") == 0) {
      show_stats(game);
    } else if (strcmp(command, "inventory") == 0 || strcmp(command, "inv") == 0) {
      show_inventory(game);
    } else if (strcmp(command, "quests") == 0 || strcmp(command, "journal") == 0) {
      show_quests(game);
    } else if (strcmp(command, "time") == 0) {
      show_time(game);
    } else if (strcmp(command, "rumor")  == 0 ||
               strcmp(command, "rumours") == 0 ||
               strcmp(command, "rumors")  == 0) {
      tui_append_log(tui, game->rumor);
    } else if (strcmp(command, "scout") == 0) {
      scout_zone(game);
    } else if (strcmp(command, "hunt") == 0 || strcmp(command, "fight") == 0) {
      hunt_current_zone(game);
    } else if (strcmp(command, "gather") == 0) {
      gather_resources(game);
    } else if (strcmp(command, "explore") == 0 || strcmp(command, "search") == 0) {
      explore_special_location(game);
    } else if (strcmp(command, "talk") == 0) {
      talk_here(game);
    } else if (strcmp(command, "shop") == 0) {
      shop_here(game);
    } else if (strcmp(command, "forge") == 0) {
      forge_here(game);
    } else if (handle_trade_command(game, command)) {
      /* handled */
    } else if (handle_craft_command(game, command)) {
      /* handled */
    } else if (strcmp(command, "rest") == 0) {
      rest_here(game);
    } else if (strcmp(command, "use potion") == 0 || strcmp(command, "potion") == 0) {
      use_potion_outside_combat(game);
    } else if (strcmp(command, "use holy water") == 0 ||
               strcmp(command, "holy water") == 0) {
      use_holy_water_outside_combat(game);
    } else if (strcmp(command, "use relic dust") == 0 ||
               strcmp(command, "relic dust") == 0 ||
               strcmp(command, "dust") == 0) {
      use_relic_dust_outside_combat(game);
    } else if (strcmp(command, "save") == 0 || strcmp(command, "저장") == 0) {
      if (save_game(game, SAVE_FILE_PATH)) {
        tui_append_log(tui, "게임이 저장되었습니다.");
      } else {
        tui_append_log(tui, "저장에 실패했습니다.");
      }
    } else if (strcmp(command, "load") == 0 || strcmp(command, "불러오기") == 0) {
      if (load_game(game, SAVE_FILE_PATH)) {
        tui_append_log(tui, "게임을 불러왔습니다.");
        describe_zone(game);
      } else {
        tui_append_log(tui, "불러오기에 실패했습니다.");
      }
    } else if (strcmp(command, "quit") == 0 ||
               strcmp(command, "exit") == 0 ||
               strcmp(command, "종료") == 0) {
      tui_append_log(tui, "가장 가까운 불빛 쪽으로 물러납니다...");
      game->running = false;
    } else {
      tui_append_log(tui, "알 수 없는 명령어입니다. `help`로 목록을 확인하세요.");
    }

    flush_events(game);
    return;
  }

  /* Printable character: append to input buffer */
  if (ch >= 32 && ch < 127 && tui->input_len < (int)sizeof(tui->input_buf) - 1) {
    tui->input_buf[tui->input_len++] = (char)ch;
    tui->input_buf[tui->input_len]   = '\0';
  }
}

/* ---- Wall-clock helper ---- */
static uint64_t wall_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000);
}

/* ---- Ending screen (after game loop exits) ---- */
static void show_ending(GameState *game, TuiState *tui)
{
  if (game->final_boss_defeated) {
    if (game->doom <= 2) {
      tui_append_log(tui, "★ 찬란한 승리 ★");
      tui_append_log(tui, "공허의 왕관이 산산이 부서지고 어둠이 왕국 땅 위에서 물러납니다.");
    } else if (game->doom <= 6) {
      tui_append_log(tui, "◆ 수호자의 승리 ◆");
      tui_append_log(tui, "왕관은 부서졌습니다. 그러나 전투의 흔적은 깊습니다.");
    } else {
      tui_append_log(tui, "▲ 피루스적 승리 ▲");
      tui_append_log(tui, "왕관은 부서졌지만, 공허의 그림자가 너무 깊이 스며들었습니다.");
    }
    if (game->beacon_lit && game->doom <= 4) {
      tui_append_log(tui, "[에필로그] 고대 봉화의 빛이 다시 한 번 남쪽 하늘을 물들입니다.");
    }
  } else if (!game->running && game->player.hp <= 0) {
    tui_append_log(tui, "게임 오버.");
  }

  /* Show final screen for a moment */
  tui_refresh_all(tui, game);
  nodelay(stdscr, FALSE);
  tui_append_log(tui, "--- 계속하려면 아무 키나 누르세요 ---");
  tui_refresh_all(tui, game);
  getch();
}

/* ---- Main entry point ---- */
int rpg_run(void)
{
  GameState game;
  TuiState  tui;

  srand((unsigned int)time(NULL));
  init_game(&game);

  /* Initialise TUI */
  if (!tui_init(&tui)) {
    /* Fallback: shouldn't happen on modern terminals */
    fprintf(stderr, "ncurses 초기화에 실패했습니다.\n");
    shutdown_game(&game);
    return 1;
  }
  tui_set_global(&tui);

  /* Register resize handler */
#ifdef SIGWINCH
  signal(SIGWINCH, handle_sigwinch);
#endif

  /* Startup class/name selection */
  startup_screen(&game, &tui);

  /* Initial zone description in log */
  describe_zone(&game);
  show_help(&game);
  flush_events(&game);

  /* ---- Real-time main loop ---- */
  uint64_t last_wall = wall_ms();

  while (game.running) {
    /* Handle terminal resize */
#ifdef SIGWINCH
    if (g_resize_pending) {
      g_resize_pending = 0;
      tui_handle_resize(&tui);
    }
#endif

    /* Advance game clock with elapsed wall time */
    uint64_t now = wall_ms();
    uint64_t elapsed = now - last_wall;
    last_wall = now;
    if (elapsed > 200) elapsed = 200; /* clamp large jumps (e.g. debugger pause) */
    game.clock_ms += elapsed;

    /* Process any Feather scheduled tasks that are now due */
    tick_game_tasks(&game);
    flush_events(&game);

    /* Poll for input (non-blocking) */
    int ch = getch();
    if (ch != ERR) {
      handle_char(&game, &tui, ch);
    }

    /* Refresh all panels */
    tui_refresh_all(&tui, &game);

    /* Sleep ~50 ms between ticks (≈20 Hz refresh) */
    {
      struct timespec ts = {0, 50000000L};
      nanosleep(&ts, NULL);
    }
  }

  show_ending(&game, &tui);
  tui_deinit(&tui);
  tui_set_global(NULL);
  shutdown_game(&game);
  return 0;
}
