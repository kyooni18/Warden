#ifndef RPG_GAME_SHARED_H
#define RPG_GAME_SHARED_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "Feather.h"
#include "game_tui.h"
#define GAME_MINUTE_MS 1000ULL
#define MAX_INPUT 256
#define MAX_EVENTS 24
#define MAX_EVENT_TEXT 256
#define MAX_NAME 48
#define SAVE_FILE_PATH "savegame.dat"
#define SAVE_MAGIC 0x52504753u
#define SAVE_VERSION 3u
#define ZONE_NONE (-1)
typedef enum ZoneId {
  ZONE_EMBERFALL_GATE = 0,
  ZONE_BRASS_MARKET = 1,
  ZONE_LANTERN_WARD = 2,
  ZONE_WHISPER_LIBRARY = 3,
  ZONE_IRONWOOD_PASS = 4,
  ZONE_VERDANT_ABBEY = 5,
  ZONE_MOONFEN = 6,
  ZONE_STORMWATCH_CLIFFS = 7,
  ZONE_ASHEN_QUARRY = 8,
  ZONE_SUNKEN_ARCHIVE = 9,
  ZONE_GLOAM_PORT = 10,
  ZONE_FROSTSPIRE_TRAIL = 11,
  ZONE_CINDER_GROVE = 12,
  ZONE_OBSIDIAN_CRATER = 13,
  ZONE_RUINED_BASILICA = 14,
  ZONE_HOLLOW_THRONE = 15,
  ZONE_DEEPWOOD_HOLLOW = 16,
  ZONE_MAGMA_RIFT = 17,
  ZONE_ANCIENT_BEACON = 18,
  ZONE_SHATTERED_VAULT = 19,
  ZONE_ECHO_SHORE = 20,
  ZONE_BONE_TOMB = 21,
  ZONE_LIGHT_SPIRE = 22,
  ZONE_IRON_CITADEL = 23,
  ZONE_COUNT = 24
} ZoneId;
typedef enum WeatherId {
  WEATHER_CLEAR = 0,
  WEATHER_RAIN = 1,
  WEATHER_FOG = 2,
  WEATHER_WIND = 3,
  WEATHER_ASH = 4,
  WEATHER_STORM = 5,
  WEATHER_COUNT = 6
} WeatherId;
typedef enum PlayerClass {
  CLASS_WARRIOR = 0,
  CLASS_SCOUT   = 1,
  CLASS_MAGE    = 2,
  CLASS_CLERIC  = 3
} PlayerClass;
typedef enum ResourceId {
  RESOURCE_NONE = 0,
  RESOURCE_HERB = 1,
  RESOURCE_ORE = 2
} ResourceId;
typedef enum QuestStage {
  QUEST_LOCKED = 0,
  QUEST_ACTIVE = 1,
  QUEST_COMPLETE = 2
} QuestStage;
typedef enum FragmentId {
  FRAGMENT_TIDAL = 0,
  FRAGMENT_FROST = 1,
  FRAGMENT_EMBER = 2,
  FRAGMENT_COUNT = 3
} FragmentId;
typedef enum EnemyRole {
  ENEMY_ROLE_SKIRMISHER = 0,
  ENEMY_ROLE_BRUTE = 1,
  ENEMY_ROLE_CASTER = 2,
  ENEMY_ROLE_BOSS = 3
} EnemyRole;
typedef enum BattleResult {
  BATTLE_RESULT_VICTORY = 0,
  BATTLE_RESULT_FLED = 1,
  BATTLE_RESULT_DEFEAT = 2
} BattleResult;
typedef struct ZoneData {
  const char *name;
  const char *short_name;
  const char *description;
  const char *scout_text;
  const char *npc;
  ResourceId resource;
  int danger;
  bool safe;
  bool merchant;
  bool healer;
  bool forge;
  bool archive;
} ZoneData;
typedef struct Player {
  char name[MAX_NAME];
  PlayerClass player_class;
  int zone;
  int level;
  int xp;
  int xp_to_next;
  int hp;
  int max_hp;
  int strength;
  int guard;
  int gold;
  int potions;
  int bombs;
  int herbs;
  int ore;
  int relic_dust;
  int rune_shards;
  int holy_water;
  int victories;
  bool discovered[ZONE_COUNT];
  bool claimed_gate_supplies;
  bool steel_edge;
  bool ward_mail;
  bool abbey_sigil;
  bool spirit_totem;
  bool titan_blade;
} Player;
typedef struct Enemy {
  char name[64];
  char intro[192];
  char special[192];
  int max_hp;
  int hp;
  int attack;
  int defense;
  int xp_reward;
  int gold_reward;
  int tier;
  EnemyRole role;
  bool boss;
  bool steals_gold;
  bool inflicts_weakness;
  bool burns_player;
  bool bleeds_player;
  bool is_elite;
} Enemy;
typedef struct CombatState {
  bool active;
  bool guard_active;
  bool parry_active;
  bool holy_barrier_active;
  bool enemy_charging;
  int weaken_turns;
  int enemy_burn_turns;
  int enemy_stun_turns;
  int player_bleed_turns;
  Enemy enemy;
} CombatState;
typedef struct GameState {
  struct Feather feather;
  uint64_t clock_ms;
  WeatherId weather;
  int doom;
  int event_count;
  int market_potions;
  int port_potions;
  int caravan_zone;
  char rumor[MAX_EVENT_TEXT];
  char events[MAX_EVENTS][MAX_EVENT_TEXT];
  Player player;
  CombatState combat;
  QuestStage remedy_quest;
  QuestStage caravan_quest;
  QuestStage fragments_quest;
  QuestStage crown_quest;
  QuestStage beacon_quest;
  QuestStage druid_quest;
  QuestStage vault_quest;
  QuestStage shore_quest;
  QuestStage citadel_quest;
  bool fragment_found[FRAGMENT_COUNT];
  bool bandit_reeve_defeated;
  bool dawn_key_forged;
  bool basilica_blessing;
  bool final_boss_defeated;
  bool beacon_lit;
  bool citadel_warden_defeated;
  bool running;
} GameState;
typedef struct SaveData {
  uint32_t magic;
  uint32_t version;
  uint64_t clock_ms;
  int weather;
  int doom;
  int market_potions;
  int port_potions;
  int caravan_zone;
  char rumor[MAX_EVENT_TEXT];
  Player player;
  int remedy_quest;
  int caravan_quest;
  int fragments_quest;
  int crown_quest;
  int beacon_quest;
  int druid_quest;
  int vault_quest;
  int shore_quest;
  int citadel_quest;
  bool fragment_found[FRAGMENT_COUNT];
  bool bandit_reeve_defeated;
  bool dawn_key_forged;
  bool basilica_blessing;
  bool final_boss_defeated;
  bool beacon_lit;
  bool citadel_warden_defeated;
  bool running;
} SaveData;

extern const ZoneData kZones[ZONE_COUNT];
extern const char *kWeatherNames[WEATHER_COUNT];
extern const char *kFragmentNames[FRAGMENT_COUNT];


/* ---- game_utils.c ---- */
int clamp_int(int value, int min_value, int max_value);
uint64_t minutes_to_ms(int minutes);
int roll_range(int min_value, int max_value);
bool is_blank(const char *text);
void canonicalize_input(const char *input, char *output, size_t output_size);
bool starts_with(const char *text, const char *prefix);

/* ---- game_state.c ---- */
void push_event(GameState *game, const char *fmt, ...);
void flush_events(GameState *game);
int current_day(const GameState *game);
int current_hour(const GameState *game);
int current_minute(const GameState *game);
const char *time_band(const GameState *game);
const char *class_name(PlayerClass cls);
bool zone_has_merchant(const GameState *game, int zone);
int zone_from_direction(int zone, const char *direction);
void show_exits(int zone);
int player_attack_value(const GameState *game);
int player_defense_value(const GameState *game);
void refresh_rumor(GameState *game);
void advance_time(GameState *game, int minutes);
void tick_game_tasks(GameState *game);
void select_class(GameState *game, PlayerClass cls);
void init_game(GameState *game);
void shutdown_game(GameState *game);
bool save_game(const GameState *game, const char *path);
bool load_game(GameState *game, const char *path);

/* ---- game_ui.c ---- */
void describe_zone(const GameState *game);
void show_help(const GameState *game);
void show_map(const GameState *game);
void show_stats(const GameState *game);
void show_inventory(const GameState *game);
void show_time(const GameState *game);
void show_quests(const GameState *game);
bool read_command(const char *prompt, char *buffer, size_t buffer_size);

/* ---- game_combat.c ---- */
Enemy build_regular_enemy(GameState *game, int zone);
Enemy build_fragment_guardian(GameState *game, FragmentId fragment);
Enemy build_citadel_warden(GameState *game);
Enemy build_final_boss(GameState *game);
BattleResult run_battle(GameState *game, Enemy enemy);

/* ---- game_actions.c ---- */
void use_potion_outside_combat(GameState *game);
void use_holy_water_outside_combat(GameState *game);
void hunt_current_zone(GameState *game);
void scout_zone(GameState *game);
void gather_resources(GameState *game);
void explore_special_location(GameState *game);
void talk_here(GameState *game);
void shop_here(GameState *game);
void forge_here(GameState *game);
void rest_here(GameState *game);
bool maybe_handle_movement_command(GameState *game, const char *command);

/* ---- Route printf through the TUI log ----
 * All printf() calls in game source files are redirected to game_printf()
 * which appends the text to the scrolling TUI log (or falls back to stdout
 * before the TUI is initialised).  This must come after all headers and
 * declarations so that the real printf prototype is already visible.        */
#ifndef GAME_TUI_NO_PRINTF_REDIRECT
#undef  printf
#define printf  game_printf
#endif

#endif
