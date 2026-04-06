#define _POSIX_C_SOURCE 200809L
#include "game_shared.h"

static uint64_t game_now_ms(void *context) {
  const GameState *game = (const GameState *)context;
  return game->clock_ms;
}
void push_event(GameState *game, const char *fmt, ...) {
  va_list args;
  if (game->event_count >= MAX_EVENTS) {
    return;
  }
  va_start(args, fmt);
  vsnprintf(game->events[game->event_count],
            sizeof(game->events[game->event_count]), fmt, args);
  va_end(args);
  game->event_count++;
}
void flush_events(GameState *game) {
  int index;
  for (index = 0; index < game->event_count; index++) {
    printf("[세계] %s\n", game->events[index]);
  }
  game->event_count = 0;
}
static int total_minutes_elapsed(const GameState *game) {
  return (int)(game->clock_ms / GAME_MINUTE_MS);
}
int current_day(const GameState *game) {
  return total_minutes_elapsed(game) / 1440 + 1;
}
int current_hour(const GameState *game) {
  return (total_minutes_elapsed(game) / 60) % 24;
}
int current_minute(const GameState *game) {
  return total_minutes_elapsed(game) % 60;
}
const char *time_band(const GameState *game) {
  int hour = current_hour(game);
  if (hour < 5) {
    return "심야";
  }
  if (hour < 8) {
    return "새벽";
  }
  if (hour < 12) {
    return "오전";
  }
  if (hour < 17) {
    return "오후";
  }
  if (hour < 21) {
    return "저녁";
  }
  return "늦은 밤";
}
static bool zone_is_safe(int zone) {
  return kZones[zone].safe;
}
const char *class_name(PlayerClass cls) {
  switch (cls) {
  case CLASS_WARRIOR: return "전사";
  case CLASS_SCOUT:   return "척후";
  case CLASS_MAGE:    return "마법사";
  case CLASS_CLERIC:  return "성직자";
  default:            return "수호자";
  }
}
bool zone_has_merchant(const GameState *game, int zone) {
  return kZones[zone].merchant || game->caravan_zone == zone;
}
static int zone_north(int zone) {
  return zone >= 4 ? zone - 4 : ZONE_NONE;
}
static int zone_south(int zone) {
  return zone < 20 ? zone + 4 : ZONE_NONE;
}
static int zone_west(int zone) {
  return zone % 4 != 0 ? zone - 1 : ZONE_NONE;
}
static int zone_east(int zone) {
  return zone % 4 != 3 ? zone + 1 : ZONE_NONE;
}
int zone_from_direction(int zone, const char *direction) {
  if (strcmp(direction, "north") == 0 || strcmp(direction, "n") == 0) {
    return zone_north(zone);
  }
  if (strcmp(direction, "south") == 0 || strcmp(direction, "s") == 0) {
    return zone_south(zone);
  }
  if (strcmp(direction, "east") == 0 || strcmp(direction, "e") == 0) {
    return zone_east(zone);
  }
  if (strcmp(direction, "west") == 0 || strcmp(direction, "w") == 0) {
    return zone_west(zone);
  }
  return ZONE_NONE;
}
void show_exits(int zone) {
  printf("이동 가능:");
  if (zone_north(zone) != ZONE_NONE) {
    printf(" 북");
  }
  if (zone_east(zone) != ZONE_NONE) {
    printf(" 동");
  }
  if (zone_south(zone) != ZONE_NONE) {
    printf(" 남");
  }
  if (zone_west(zone) != ZONE_NONE) {
    printf(" 서");
  }
  printf("\n");
}
int player_attack_value(const GameState *game) {
  int attack = game->player.strength + game->player.level * 2;
  if (game->player.titan_blade) {
    attack += 7;
  } else if (game->player.steel_edge) {
    attack += 4;
  }
  if (game->player.abbey_sigil) {
    attack += 1;
  }
  return attack;
}
int player_defense_value(const GameState *game) {
  int defense = game->player.guard + game->player.level;
  if (game->player.ward_mail) {
    defense += 3;
  }
  if (game->basilica_blessing) {
    defense += 1;
  }
  return defense;
}
static void fill_rumor(GameState *game, const char *text) {
  snprintf(game->rumor, sizeof(game->rumor), "%s", text);
}
void refresh_rumor(GameState *game) {
  if (!game->bandit_reeve_defeated) {
    fill_rumor(
        game,
        "상인들 말로는 철목 고개에서 도적 영주가 유물 통행세를 걷기 시작했다.");
    return;
  }
  if (game->fragments_quest == QUEST_ACTIVE &&
      !game->fragment_found[FRAGMENT_TIDAL]) {
    fill_rumor(
        game,
        "침수 기록고의 가장 깊은 서가를 익사한 관리인이 아직도 지키고 있다.");
    return;
  }
  if (game->fragments_quest == QUEST_ACTIVE &&
      !game->fragment_found[FRAGMENT_FROST]) {
    fill_rumor(
        game,
        "새 눈 아래 묻힌 사당 근처에서 빙첨로 상단이 계속 사라지고 있다.");
    return;
  }
  if (game->fragments_quest == QUEST_ACTIVE &&
      !game->fragment_found[FRAGMENT_EMBER]) {
    fill_rumor(
        game,
        "흑요 분화구 어딘가 빛나는 방 주변을 광신도의 불씨 무리가 배회한다.");
    return;
  }
  if (game->dawn_key_forged && !game->final_boss_defeated) {
    fill_rumor(
        game,
        "폐허 대성당에서 종 없이 울림이 시작됐다. 왕좌가 깨어난다.");
    return;
  }
  if (!game->citadel_warden_defeated) {
    fill_rumor(
        game,
        "부서진 성채에서 철갑 구울의 행진 소리가 들린다. 누군가 해방시켜야 한다.");
    return;
  }
  if (game->shore_quest == QUEST_ACTIVE) {
    fill_rumor(
        game,
        "메아리 해안의 은자 레나가 신성한 의식을 위한 재료를 기다리고 있다.");
    return;
  }
  fill_rumor(
      game,
      "봉화는 아직 버티지만, 모든 순찰대는 밤마다 공허 왕좌의 울림이 커진다고 말한다.");
}
static void process_ready_tasks(GameState *game) {
  int safety = 0;
  while (safety < 256 && Feather_step(&game->feather)) {
    safety++;
  }
}
void advance_time(GameState *game, int minutes) {
  game->clock_ms += minutes_to_ms(minutes);
  process_ready_tasks(game);
}
static void task_weather_shift(void *context) {
  GameState *game = (GameState *)context;
  WeatherId next_weather = (WeatherId)roll_range(0, WEATHER_COUNT - 1);
  if (next_weather == game->weather) {
    next_weather = (WeatherId)((next_weather + 1) % WEATHER_COUNT);
  }
  game->weather = next_weather;
  push_event(game, "날씨 변화: %s가 남부를 휩쓸기 시작합니다.",
             kWeatherNames[game->weather]);
}
static void task_restock(void *context) {
  GameState *game = (GameState *)context;
  const int caravan_stops[3] = {ZONE_LANTERN_WARD, ZONE_VERDANT_ABBEY,
                                ZONE_GLOAM_PORT};
  game->market_potions = 4 + rand() % 4;
  game->port_potions = 3 + rand() % 3;
  game->caravan_zone = caravan_stops[rand() % 3];
  push_event(game, "상인들이 물자를 재입고했습니다. 이동 상단이 %s에 야영지를 엽니다.",
             kZones[game->caravan_zone].name);
}
static void task_regen(void *context) {
  GameState *game = (GameState *)context;
  int heal = 1 + game->player.level / 2;
  if (game->combat.active || game->player.hp >= game->player.max_hp) {
    return;
  }
  if (zone_is_safe(game->player.zone)) {
    heal += 2;
  }
  game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
  push_event(game, "%s이(가) 길이 잠시 잠잠해진 틈에 체력 %d를 회복했습니다.",
             game->player.name, heal);
}
static void task_doom(void *context) {
  GameState *game = (GameState *)context;
  if (game->final_boss_defeated) {
    return;
  }
  game->doom = clamp_int(game->doom + 1, 0, 12);
  if (game->doom >= 9) {
    push_event(game,
               "공허의 왕관이 요동칩니다. 전역의 적이 더 날카롭고, 더 굶주리며, 덜 "
               "필멸적으로 변합니다.");
  } else {
    push_event(game, "묻힌 궁전에서 공포의 파동이 퍼집니다. 현재 파멸도: %d.",
               game->doom);
  }
}
static void task_rumor(void *context) {
  GameState *game = (GameState *)context;
  refresh_rumor(game);
  push_event(game, "새 소문: %s", game->rumor);
}
static bool enqueue_repeating_task(GameState *game, void (*task)(void *),
                                   int start_minutes, int repeat_minutes,
                                   uint8_t priority) {
  return Feather_add_repeating_task(
      &game->feather,
      (FSSchedulerRepeatingTask){
          .task = task,
          .context = game,
          .start_time = game->clock_ms + minutes_to_ms(start_minutes),
          .execute_cycle = minutes_to_ms(repeat_minutes),
          .priority = priority,
          .repeat_mode = FSSchedulerTaskRepeat_FIXEDDELAY});
}
void init_game(GameState *game) {
  memset(game, 0, sizeof(*game));
  snprintf(game->player.name, sizeof(game->player.name), "수호자");
  game->player.player_class = CLASS_WARRIOR;
  game->player.zone = ZONE_EMBERFALL_GATE;
  game->player.level = 1;
  game->player.xp_to_next = 28;
  game->player.max_hp = 30;
  game->player.hp = 30;
  game->player.strength = 6;
  game->player.guard = 3;
  game->player.gold = 24;
  game->player.potions = 2;
  game->player.bombs = 1;
  game->player.discovered[ZONE_EMBERFALL_GATE] = true;
  game->player.discovered[ZONE_BRASS_MARKET] = true;
  game->weather = WEATHER_CLEAR;
  game->market_potions = 5;
  game->port_potions = 4;
  game->caravan_zone = ZONE_LANTERN_WARD;
  game->running = true;
  Feather_init(&game->feather);
  Feather_set_time_source(&game->feather, game_now_ms, game);
  enqueue_repeating_task(game, task_weather_shift, 180, 180,
                         FSScheduler_Priority_BACKGROUND);
  enqueue_repeating_task(game, task_restock, 720, 720,
                         FSScheduler_Priority_BACKGROUND);
  enqueue_repeating_task(game, task_regen, 90, 90,
                         FSScheduler_Priority_UI);
  enqueue_repeating_task(game, task_doom, 1440, 1440,
                         FSScheduler_Priority_INTERACTIVE);
  enqueue_repeating_task(game, task_rumor, 240, 240,
                         FSScheduler_Priority_BACKGROUND);
  refresh_rumor(game);
}
void select_class(GameState *game, PlayerClass cls) {
  game->player.player_class = cls;
  switch (cls) {
  case CLASS_WARRIOR:
    game->player.max_hp = 38;
    game->player.hp = 38;
    game->player.strength = 8;
    game->player.guard = 5;
    game->player.gold = 20;
    game->player.potions = 2;
    game->player.bombs = 1;
    game->player.rune_shards = 0;
    break;
  case CLASS_SCOUT:
    game->player.max_hp = 25;
    game->player.hp = 25;
    game->player.strength = 7;
    game->player.guard = 3;
    game->player.gold = 28;
    game->player.potions = 3;
    game->player.bombs = 2;
    game->player.rune_shards = 0;
    break;
  case CLASS_MAGE:
    game->player.max_hp = 22;
    game->player.hp = 22;
    game->player.strength = 5;
    game->player.guard = 2;
    game->player.gold = 30;
    game->player.potions = 3;
    game->player.bombs = 0;
    game->player.rune_shards = 3;
    break;
  case CLASS_CLERIC:
    game->player.max_hp = 28;
    game->player.hp = 28;
    game->player.strength = 6;
    game->player.guard = 4;
    game->player.gold = 22;
    game->player.potions = 4;
    game->player.bombs = 0;
    game->player.holy_water = 2;
    game->player.rune_shards = 0;
    break;
  }
}
void shutdown_game(GameState *game) {
  Feather_deinit(&game->feather);
}
bool save_game(const GameState *game, const char *path) {
  FILE *file = fopen(path, "wb");
  SaveData data;
  if (file == NULL) {
    return false;
  }
  memset(&data, 0, sizeof(data));
  data.magic = SAVE_MAGIC;
  data.version = SAVE_VERSION;
  data.clock_ms = game->clock_ms;
  data.weather = (int)game->weather;
  data.doom = game->doom;
  data.market_potions = game->market_potions;
  data.port_potions = game->port_potions;
  data.caravan_zone = game->caravan_zone;
  snprintf(data.rumor, sizeof(data.rumor), "%s", game->rumor);
  data.player = game->player;
  data.remedy_quest = (int)game->remedy_quest;
  data.caravan_quest = (int)game->caravan_quest;
  data.fragments_quest = (int)game->fragments_quest;
  data.crown_quest = (int)game->crown_quest;
  data.beacon_quest = (int)game->beacon_quest;
  data.druid_quest = (int)game->druid_quest;
  data.vault_quest = (int)game->vault_quest;
  data.shore_quest = (int)game->shore_quest;
  data.citadel_quest = (int)game->citadel_quest;
  memcpy(data.fragment_found, game->fragment_found, sizeof(data.fragment_found));
  data.bandit_reeve_defeated = game->bandit_reeve_defeated;
  data.dawn_key_forged = game->dawn_key_forged;
  data.basilica_blessing = game->basilica_blessing;
  data.final_boss_defeated = game->final_boss_defeated;
  data.beacon_lit = game->beacon_lit;
  data.citadel_warden_defeated = game->citadel_warden_defeated;
  data.running = game->running;
  if (fwrite(&data, sizeof(data), 1, file) != 1) {
    fclose(file);
    return false;
  }
  fclose(file);
  return true;
}
bool load_game(GameState *game, const char *path) {
  FILE *file = fopen(path, "rb");
  SaveData data;
  if (file == NULL) {
    return false;
  }
  if (fread(&data, sizeof(data), 1, file) != 1) {
    fclose(file);
    return false;
  }
  fclose(file);
  if (data.magic != SAVE_MAGIC || data.version != SAVE_VERSION) {
    return false;
  }
  if (data.weather < 0 || data.weather >= WEATHER_COUNT ||
      data.player.zone < 0 || data.player.zone >= ZONE_COUNT ||
      data.caravan_zone < 0 || data.caravan_zone >= ZONE_COUNT) {
    return false;
  }
  shutdown_game(game);
  memset(game, 0, sizeof(*game));
  game->clock_ms = data.clock_ms;
  game->weather = (WeatherId)data.weather;
  game->doom = clamp_int(data.doom, 0, 12);
  game->market_potions = data.market_potions;
  game->port_potions = data.port_potions;
  game->caravan_zone = data.caravan_zone;
  snprintf(game->rumor, sizeof(game->rumor), "%s", data.rumor);
  game->player = data.player;
  game->remedy_quest = (QuestStage)data.remedy_quest;
  game->caravan_quest = (QuestStage)data.caravan_quest;
  game->fragments_quest = (QuestStage)data.fragments_quest;
  game->crown_quest = (QuestStage)data.crown_quest;
  game->beacon_quest = (QuestStage)data.beacon_quest;
  game->druid_quest = (QuestStage)data.druid_quest;
  game->vault_quest = (QuestStage)data.vault_quest;
  game->shore_quest = (QuestStage)data.shore_quest;
  game->citadel_quest = (QuestStage)data.citadel_quest;
  memcpy(game->fragment_found, data.fragment_found, sizeof(game->fragment_found));
  game->bandit_reeve_defeated = data.bandit_reeve_defeated;
  game->dawn_key_forged = data.dawn_key_forged;
  game->basilica_blessing = data.basilica_blessing;
  game->final_boss_defeated = data.final_boss_defeated;
  game->beacon_lit = data.beacon_lit;
  game->citadel_warden_defeated = data.citadel_warden_defeated;
  game->running = data.running;
  game->event_count = 0;
  game->combat.active = false;
  game->combat.guard_active = false;
  game->combat.enemy_charging = false;
  game->combat.weaken_turns = 0;
  memset(&game->combat.enemy, 0, sizeof(game->combat.enemy));
  Feather_init(&game->feather);
  Feather_set_time_source(&game->feather, game_now_ms, game);
  enqueue_repeating_task(game, task_weather_shift, 180, 180,
                         FSScheduler_Priority_BACKGROUND);
  enqueue_repeating_task(game, task_restock, 720, 720,
                         FSScheduler_Priority_BACKGROUND);
  enqueue_repeating_task(game, task_regen, 90, 90,
                         FSScheduler_Priority_UI);
  enqueue_repeating_task(game, task_doom, 1440, 1440,
                         FSScheduler_Priority_INTERACTIVE);
  enqueue_repeating_task(game, task_rumor, 240, 240,
                         FSScheduler_Priority_BACKGROUND);
  if (is_blank(game->rumor)) {
    refresh_rumor(game);
  }
  return true;
}
