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
#define GAME_MINUTE_MS 1000ULL
#define MAX_INPUT 256
#define MAX_EVENTS 24
#define MAX_EVENT_TEXT 256
#define MAX_NAME 48
#define SAVE_FILE_PATH "savegame.dat"
#define SAVE_MAGIC 0x52504753u
#define SAVE_VERSION 1u
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
  ZONE_COUNT = 16
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
  int victories;
  bool discovered[ZONE_COUNT];
  bool claimed_gate_supplies;
  bool steel_edge;
  bool ward_mail;
  bool abbey_sigil;
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
} Enemy;
typedef struct CombatState {
  bool active;
  bool guard_active;
  bool enemy_charging;
  int weaken_turns;
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
  bool fragment_found[FRAGMENT_COUNT];
  bool bandit_reeve_defeated;
  bool dawn_key_forged;
  bool basilica_blessing;
  bool final_boss_defeated;
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
  bool fragment_found[FRAGMENT_COUNT];
  bool bandit_reeve_defeated;
  bool dawn_key_forged;
  bool basilica_blessing;
  bool final_boss_defeated;
  bool running;
} SaveData;

extern const ZoneData kZones[ZONE_COUNT];
extern const char *kWeatherNames[WEATHER_COUNT];
extern const char *kFragmentNames[FRAGMENT_COUNT];

#endif
