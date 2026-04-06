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
  char line[MAX_EVENT_TEXT + 16];
  TuiState *tui = tui_get_global();
  for (index = 0; index < game->event_count; index++) {
    snprintf(line, sizeof(line), "[세계] %s", game->events[index]);
    if (tui && tui->initialized) {
      tui_append_log(tui, line);
    } else {
      fputs(line, stdout);
      fputc('\n', stdout);
    }
  }
  game->event_count = 0;
}
static int total_minutes_elapsed(const GameState *game) {
  return (int)(game->clock_ms / GAME_MINUTE_MS);
}
static int clamp_zone(int zone) {
  if (zone < 0 || zone >= ZONE_COUNT) {
    return ZONE_EMBERFALL_GATE;
  }
  return zone;
}
int npc_favor(const GameState *game, int zone) {
  int z = clamp_zone(zone);
  return game->npc_rel[z].favor;
}
void adjust_npc_favor(GameState *game, int zone, int delta) {
  int z = clamp_zone(zone);
  game->npc_rel[z].favor = clamp_int(game->npc_rel[z].favor + delta, 0, 100);
  game->npc_rel[z].trust = clamp_int(game->npc_rel[z].trust + delta / 2, 0, 100);
  game->npc_rel[z].reputation = clamp_int(game->npc_rel[z].reputation + delta / 3, 0, 100);
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
static int event_power_scale(const GameState *game) {
  return 1 + game->player.level / 3 + game->doom / 4;
}
static const char *event_category_name(EventCategory category) {
  switch (category) {
  case EVENT_CATEGORY_WEATHER:
    return "기상";
  case EVENT_CATEGORY_THREAT:
    return "위협";
  case EVENT_CATEGORY_ECONOMY:
    return "교역";
  case EVENT_CATEGORY_NPC:
    return "인물";
  case EVENT_CATEGORY_MYSTIC:
    return "비전";
  default:
    return "세계";
  }
}
static int active_world_event_index(const GameState *game, EventCategory category,
                                    int zone) {
  int i;
  for (i = 0; i < MAX_WORLD_EVENTS; i++) {
    if (game->world_events[i].active &&
        game->world_events[i].category == category &&
        game->world_events[i].zone == zone) {
      return i;
    }
  }
  return -1;
}
static void clear_expired_world_events(GameState *game) {
  int i;
  int day = current_day(game);
  game->world_event_count = 0;
  for (i = 0; i < MAX_WORLD_EVENTS; i++) {
    if (!game->world_events[i].active) {
      continue;
    }
    if (game->world_events[i].expires_day < day) {
      game->world_events[i].active = false;
      continue;
    }
    game->world_event_count++;
  }
}
static void add_world_event(GameState *game, EventCategory category, int zone,
                            int intensity, int duration_days,
                            const char *text_fmt, ...) {
  int i;
  int slot = -1;
  va_list args;
  char msg[96];
  for (i = 0; i < MAX_WORLD_EVENTS; i++) {
    if (!game->world_events[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1) {
    int oldest = 0;
    for (i = 1; i < MAX_WORLD_EVENTS; i++) {
      if (game->world_events[i].expires_day <
          game->world_events[oldest].expires_day) {
        oldest = i;
      }
    }
    slot = oldest;
  }
  va_start(args, text_fmt);
  vsnprintf(msg, sizeof(msg), text_fmt, args);
  va_end(args);
  game->world_events[slot].active = true;
  game->world_events[slot].category = category;
  game->world_events[slot].zone = clamp_zone(zone);
  game->world_events[slot].intensity = clamp_int(intensity, 1, 9);
  game->world_events[slot].expires_day = current_day(game) + duration_days;
  snprintf(game->world_events[slot].text, sizeof(game->world_events[slot].text),
           "%s", msg);
  clear_expired_world_events(game);
  push_event(game, "[%s] %s", event_category_name(category), msg);
}
int world_event_intensity(const GameState *game, int zone,
                          EventCategory category) {
  int idx = active_world_event_index(game, category, zone);
  if (idx < 0) {
    return 0;
  }
  return game->world_events[idx].intensity;
}
void refresh_rumor(GameState *game) {
  clear_expired_world_events(game);
  if (game->world_event_count > 0) {
    int idx = rand() % MAX_WORLD_EVENTS;
    int safety = 0;
    while (!game->world_events[idx].active && safety < MAX_WORLD_EVENTS) {
      idx = (idx + 1) % MAX_WORLD_EVENTS;
      safety++;
    }
    if (game->world_events[idx].active) {
      fill_rumor(game, game->world_events[idx].text);
      return;
    }
  }
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
static void maybe_spawn_conditional_event(GameState *game) {
  int zone;
  int power = event_power_scale(game);
  if (roll_range(0, 99) >= 35) {
    return;
  }
  zone = roll_range(0, ZONE_COUNT - 1);
  if (!game->player.discovered[zone]) {
    return;
  }
  if (kZones[zone].safe && roll_range(0, 99) < 40) {
    add_world_event(game, EVENT_CATEGORY_ECONOMY, zone, 1 + power / 2, 1,
                    "%s에 순회 장인이 머물며 거래 가격이 개선되었습니다.",
                    kZones[zone].name);
    return;
  }
  if (!kZones[zone].safe) {
    add_world_event(game, EVENT_CATEGORY_THREAT, zone, 1 + power, 1,
                    "%s에 야간 습격대가 출몰해 이동 위험이 급증했습니다.",
                    kZones[zone].name);
  }
}
static void maybe_spawn_miniquest(GameState *game) {
  int slot;
  int zone;
  MiniQuest *mq = NULL;
  if (current_day(game) == game->miniquest_generation_day) {
    return;
  }
  game->miniquest_generation_day = current_day(game);
  if (roll_range(0, 99) < 45) {
    return;
  }
  for (slot = 0; slot < MAX_MINI_QUESTS; slot++) {
    if (!game->mini_quests[slot].active ||
        game->mini_quests[slot].expires_day < current_day(game)) {
      mq = &game->mini_quests[slot];
      break;
    }
  }
  if (mq == NULL) {
    return;
  }
  memset(mq, 0, sizeof(*mq));
  zone = roll_range(0, ZONE_COUNT - 1);
  mq->active = true;
  mq->zone = zone;
  mq->expires_day = current_day(game) + 2;
  mq->reward_gold = 12 + game->player.level * 3;
  mq->reward_xp = 10 + game->player.level * 2;
  switch (roll_range(0, 2)) {
  case 0:
    mq->type = MINIQUEST_HERB;
    mq->target = 2 + game->player.level / 3;
    snprintf(mq->summary, sizeof(mq->summary),
             "긴급 치료제: %s 인근 약초 %d개 전달", kZones[zone].short_name,
             mq->target);
    break;
  case 1:
    mq->type = MINIQUEST_ORE;
    mq->target = 2 + game->player.level / 4;
    snprintf(mq->summary, sizeof(mq->summary),
             "방벽 보수: %s 인근 광석 %d개 조달", kZones[zone].short_name,
             mq->target);
    break;
  default:
    mq->type = MINIQUEST_HUNT;
    mq->target = 1 + game->player.level / 4;
    snprintf(mq->summary, sizeof(mq->summary),
             "현상 수배: %s 위협체 %d회 제압", kZones[zone].short_name,
             mq->target);
    break;
  }
  add_world_event(game, EVENT_CATEGORY_NPC, zone, 1 + game->player.level / 4, 2,
                  "현장 의뢰가 접수됨: %s", mq->summary);
}
void ensure_miniquests(GameState *game) {
  maybe_spawn_miniquest(game);
}
static void resolve_completed_miniquests(GameState *game) {
  int i;
  for (i = 0; i < MAX_MINI_QUESTS; i++) {
    MiniQuest *mq = &game->mini_quests[i];
    if (!mq->active || mq->completed) {
      continue;
    }
    if (mq->progress < mq->target) {
      continue;
    }
    mq->completed = true;
    game->player.gold += mq->reward_gold;
    game->player.xp += mq->reward_xp;
    if (game->player.xp >= game->player.xp_to_next) {
      game->player.xp = game->player.xp_to_next - 1;
    }
    adjust_npc_favor(game, mq->zone, 8);
    push_event(game, "현장 의뢰 완료: %s (보상 골드 %d, 경험치 %d).",
               mq->summary, mq->reward_gold, mq->reward_xp);
  }
}
void progress_miniquests_resource(GameState *game, ResourceId resource, int amount) {
  int i;
  if (amount <= 0) {
    return;
  }
  for (i = 0; i < MAX_MINI_QUESTS; i++) {
    MiniQuest *mq = &game->mini_quests[i];
    if (!mq->active || mq->completed) {
      continue;
    }
    if ((mq->type == MINIQUEST_HERB && resource == RESOURCE_HERB) ||
        (mq->type == MINIQUEST_ORE && resource == RESOURCE_ORE)) {
      mq->progress = clamp_int(mq->progress + amount, 0, mq->target);
    }
  }
  resolve_completed_miniquests(game);
}
void progress_miniquests_hunt(GameState *game, int zone) {
  int i;
  for (i = 0; i < MAX_MINI_QUESTS; i++) {
    MiniQuest *mq = &game->mini_quests[i];
    if (!mq->active || mq->completed || mq->type != MINIQUEST_HUNT) {
      continue;
    }
    if (mq->zone == zone) {
      mq->progress = clamp_int(mq->progress + 1, 0, mq->target);
    }
  }
  resolve_completed_miniquests(game);
}
static void process_ready_tasks(GameState *game) {
  int safety = 0;
  while (safety < 256 && Feather_step(&game->feather)) {
    safety++;
  }
}
void tick_game_tasks(GameState *game) {
  process_ready_tasks(game);
  clear_expired_world_events(game);
  maybe_spawn_conditional_event(game);
  ensure_miniquests(game);
  resolve_completed_miniquests(game);
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
  if (game->weather == WEATHER_STORM || game->weather == WEATHER_ASH) {
    add_world_event(game, EVENT_CATEGORY_WEATHER, game->player.zone,
                    1 + event_power_scale(game), 1,
                    "%s 일대에 %s 경보가 발령되었습니다.",
                    kZones[game->player.zone].name, kWeatherNames[game->weather]);
  }
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
  add_world_event(game, EVENT_CATEGORY_ECONOMY, game->caravan_zone,
                  1 + game->player.level / 4, 1,
                  "%s에서 거래 세율이 일시 완화되었습니다.",
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
static void task_patrol(void *context) {
  GameState *game = (GameState *)context;
  int zone = roll_range(0, ZONE_COUNT - 1);
  if (!game->player.discovered[zone]) {
    return;
  }
  if (kZones[zone].safe) {
    push_event(game, "순찰 보고: %s 경계선은 안정적입니다.", kZones[zone].name);
    return;
  }
  if (roll_range(0, 99) < 45) {
    game->doom = clamp_int(game->doom + 1, 0, 12);
    push_event(game, "순찰 경보: %s에서 공허 기운이 증폭되었습니다. 파멸도 +1.",
               kZones[zone].name);
  } else {
    push_event(game, "순찰 보고: %s에서 적 동향이 포착됐지만 즉시 위협은 아닙니다.",
               kZones[zone].name);
  }
}
static void task_npc_aid(void *context) {
  GameState *game = (GameState *)context;
  if (!kZones[game->player.zone].safe) {
    return;
  }
  if (roll_range(0, 99) < 35) {
    game->player.potions++;
    adjust_npc_favor(game, game->player.zone, 2);
    push_event(game, "현지 지원대가 %s에게 응급 포션 1개를 전달했습니다.",
               game->player.name);
  } else if (roll_range(0, 99) < 50) {
    game->player.relic_dust++;
    adjust_npc_favor(game, game->player.zone, 1);
    push_event(game, "학자 길드가 정제한 유물 가루 1개를 전달했습니다.");
  }
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
  game->miniquest_generation_day = 0;
  adjust_npc_favor(game, ZONE_EMBERFALL_GATE, 15);
  adjust_npc_favor(game, ZONE_BRASS_MARKET, 8);
  adjust_npc_favor(game, ZONE_VERDANT_ABBEY, 10);
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
  enqueue_repeating_task(game, task_patrol, 150, 150,
                         FSScheduler_Priority_INTERACTIVE);
  enqueue_repeating_task(game, task_npc_aid, 210, 210,
                         FSScheduler_Priority_BACKGROUND);
  add_world_event(game, EVENT_CATEGORY_MYSTIC, ZONE_EMBERFALL_GATE, 2, 2,
                  "엠버폴 관문에 임시 지원 거점이 구축되어 보급이 강화되었습니다.");
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
  memcpy(data.world_events, game->world_events, sizeof(data.world_events));
  data.world_event_count = game->world_event_count;
  memcpy(data.npc_rel, game->npc_rel, sizeof(data.npc_rel));
  memcpy(data.mini_quests, game->mini_quests, sizeof(data.mini_quests));
  data.miniquest_generation_day = game->miniquest_generation_day;
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
  if (data.magic != SAVE_MAGIC ||
      data.version < SAVE_VERSION_MIN || data.version > SAVE_VERSION) {
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
  if (data.version >= SAVE_VERSION) {
    memcpy(game->world_events, data.world_events, sizeof(game->world_events));
    game->world_event_count = clamp_int(data.world_event_count, 0, MAX_WORLD_EVENTS);
    memcpy(game->npc_rel, data.npc_rel, sizeof(game->npc_rel));
    memcpy(game->mini_quests, data.mini_quests, sizeof(game->mini_quests));
    game->miniquest_generation_day = data.miniquest_generation_day;
  } else {
    memset(game->world_events, 0, sizeof(game->world_events));
    game->world_event_count = 0;
    memset(game->npc_rel, 0, sizeof(game->npc_rel));
    memset(game->mini_quests, 0, sizeof(game->mini_quests));
    game->miniquest_generation_day = 0;
  }
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
  enqueue_repeating_task(game, task_patrol, 150, 150,
                         FSScheduler_Priority_INTERACTIVE);
  enqueue_repeating_task(game, task_npc_aid, 210, 210,
                         FSScheduler_Priority_BACKGROUND);
  if (is_blank(game->rumor)) {
    refresh_rumor(game);
  }
  clear_expired_world_events(game);
  return true;
}
