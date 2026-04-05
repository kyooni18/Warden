#define _POSIX_C_SOURCE 200809L
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
static const ZoneData kZones[ZONE_COUNT] = {
    [ZONE_EMBERFALL_GATE] = {
        .name = "Emberfall Gate",
        .short_name = "EMBR",
        .description =
            "A basalt gatehouse watches the old kingdom road. Supply braziers "
            "burn all night, and every traveler passing south whispers about "
            "the dead court stirring beyond the ashfields.",
        .scout_text =
            "Fresh boot prints, quartermaster banners, and a safe camp. This "
            "is the best place to regroup before marching deeper.",
        .npc = "Quartermaster Iven",
        .resource = RESOURCE_NONE,
        .danger = 0,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_BRASS_MARKET] = {
        .name = "Brass Market",
        .short_name = "BRAS",
        .description =
            "The market survives under patched canvas awnings, all lantern-glow "
            "and hard bargains. Smugglers, relic buyers, and caravan guards "
            "trade rumors as quickly as coin.",
        .scout_text =
            "Crowded, loud, and useful. Potions move fast here, and frightened "
            "merchants keep pointing toward Ironwood Pass.",
        .npc = "Merchant Sal",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = true,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_LANTERN_WARD] = {
        .name = "Lantern Ward",
        .short_name = "LANT",
        .description =
            "Rows of watchtowers and narrow alleys surround a district that "
            "refuses to sleep. Couriers carry sealed orders while archivists "
            "hunt for anyone brave enough to follow old maps.",
        .scout_text =
            "The ward is secure, though every lookout is watching the southern "
            "roads. Couriers mention a moving caravan camp and strange weather "
            "from the cliffs.",
        .npc = "Courier Nara",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_WHISPER_LIBRARY] = {
        .name = "Whisper Library",
        .short_name = "WHIS",
        .description =
            "Shelves lean like broken ribs around a cold observatory dome. The "
            "library still hums with warding sigils, and every table is buried "
            "under maps of the drowned archive, frost road, and crater rim.",
        .scout_text =
            "Safe walls, fragile lore. The archivists know how to read relic "
            "shards, but they need someone willing to retrieve them first.",
        .npc = "Archivist Sen",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
    [ZONE_IRONWOOD_PASS] = {
        .name = "Ironwood Pass",
        .short_name = "IRON",
        .description =
            "Black pines knot around a steep road cut into red stone. The pass "
            "echoes with wagon wheels, bandit whistles, and the cracking sound "
            "of old trees split by lightning.",
        .scout_text =
            "Bandits favor the ridgeline, wolves keep the lower brush, and rich "
            "veins of ore show through the road cuts.",
        .npc = "No steady ally remains here.",
        .resource = RESOURCE_ORE,
        .danger = 2,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_VERDANT_ABBEY] = {
        .name = "Verdant Abbey",
        .short_name = "ABBY",
        .description =
            "A half-ruined monastery blooms with medicinal ivy and moonflowers. "
            "Its chapel bells are gone, but the resident sisters still mend "
            "armor, stitch wounds, and keep records of the dying countryside.",
        .scout_text =
            "This is the gentlest place left in the south. The sisters heal "
            "the desperate, and they need herbs from the fen to keep doing it.",
        .npc = "Sister Elowen",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = true,
        .forge = false,
        .archive = false,
    },
    [ZONE_MOONFEN] = {
        .name = "Moonfen",
        .short_name = "MOON",
        .description =
            "Silver reeds and black water stretch beneath drifting lantern "
            "fungus. The fen looks serene from a distance, right up until "
            "something with too many eyes moves beneath the surface.",
        .scout_text =
            "Moonleaf herbs grow thick here, but so do leeches and mire witches. "
            "Slow steps keep you alive longer than brave ones.",
        .npc = "Only the marsh answers back.",
        .resource = RESOURCE_HERB,
        .danger = 3,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_STORMWATCH_CLIFFS] = {
        .name = "Stormwatch Cliffs",
        .short_name = "CLIF",
        .description =
            "Jagged cliffs rise above a grey sea where wrecked skiffs turn in "
            "the surf. Gale banners snap between watch posts, and every ledge "
            "feels one bad gust away from becoming a grave.",
        .scout_text =
            "Fast raiders strike from the ledges here. When the wind turns, the "
            "cliffs become a gauntlet of blind corners and shrieking air.",
        .npc = "A deserted signal fire crackles.",
        .resource = RESOURCE_NONE,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_ASHEN_QUARRY] = {
        .name = "Ashen Quarry",
        .short_name = "QUAR",
        .description =
            "The quarry floor glows with old fire. Chain lifts hang over broken "
            "slabs, and abandoned forges still hold enough heat to temper new "
            "steel if someone brings the right ore.",
        .scout_text =
            "Good ore, bad company. Quarry dead and slag brutes roam the pits, "
            "but a skilled hand could still rework the old anvils.",
        .npc = "The forge can still be worked.",
        .resource = RESOURCE_ORE,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = true,
        .archive = false,
    },
    [ZONE_SUNKEN_ARCHIVE] = {
        .name = "Sunken Archive",
        .short_name = "SUNK",
        .description =
            "A library district collapsed into the tidal basin decades ago. "
            "Marble stairs vanish underwater between tilted stacks, and drowned "
            "scribes still patrol the catalog halls below the surface.",
        .scout_text =
            "Old wards flicker beneath the waterline. If a relic fragment "
            "survived anywhere, one likely rests in the curator vaults below.",
        .npc = "Only the tide keeps count.",
        .resource = RESOURCE_NONE,
        .danger = 5,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
    [ZONE_GLOAM_PORT] = {
        .name = "Gloam Port",
        .short_name = "PORT",
        .description =
            "Ships no longer sail from the port, but its piers still support "
            "brokers, ferrymen, and scavengers who work by lantern and hook. "
            "The harbor master knows more than she says.",
        .scout_text =
            "Supplies can still be bought here, and old captains chart the road "
            "to Frostspire better than any surviving map.",
        .npc = "Captain Mirelle",
        .resource = RESOURCE_NONE,
        .danger = 2,
        .safe = true,
        .merchant = true,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_FROSTSPIRE_TRAIL] = {
        .name = "Frostspire Trail",
        .short_name = "FROS",
        .description =
            "The road narrows into a frozen shelf beneath shattered statues. "
            "Snow never melts here; it simply layers over the bones of every "
            "caravan that tried to outrun the mountain dead.",
        .scout_text =
            "The trail is quiet in the worst possible way. Revenants wait under "
            "the snow, and the cold itself bites like a predator.",
        .npc = "The mountain wind answers every question with teeth.",
        .resource = RESOURCE_NONE,
        .danger = 6,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_CINDER_GROVE] = {
        .name = "Cinder Grove",
        .short_name = "GROV",
        .description =
            "Charred trunks stand around a grove that somehow still flowers. "
            "The soil is warm, the air smells of cedar and ash, and rare herbs "
            "push through the soot after every storm.",
        .scout_text =
            "This grove rewards careful gathering. Ember stags and thornhexers "
            "both hide in the smoke when the weather turns.",
        .npc = "Only antler marks scar the bark.",
        .resource = RESOURCE_HERB,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_OBSIDIAN_CRATER] = {
        .name = "Obsidian Crater",
        .short_name = "CRAT",
        .description =
            "A vast impact bowl of black glass and red vapor. Lava veins pulse "
            "beneath the crust, and cultists still treat the crater rim like a "
            "cathedral built for something buried beneath it.",
        .scout_text =
            "Strong ore and stronger enemies. Anything powerful enough to hold "
            "a relic fragment would wake here in fire and broken glass.",
        .npc = "The crater chants to itself.",
        .resource = RESOURCE_ORE,
        .danger = 7,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_RUINED_BASILICA] = {
        .name = "Ruined Basilica",
        .short_name = "BASI",
        .description =
            "A once-golden basilica now stands roofless beneath a dim sky. "
            "Prayer mosaics survive in fragments, and the nave points like an "
            "arrow toward the buried throne where the crown wakes.",
        .scout_text =
            "The basilica is a threshold. Spirits gather here, and any blessing "
            "you can wrest from the place may matter before the throne.",
        .npc = "Echoes recite broken liturgies.",
        .resource = RESOURCE_NONE,
        .danger = 6,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_HOLLOW_THRONE] = {
        .name = "Hollow Throne",
        .short_name = "THRN",
        .description =
            "The buried palace opens into a cathedral of stone roots and cracked "
            "marble. At its center sits an empty throne wrapped in shadow, where "
            "the kingdom's last hunger keeps trying to remember its own name.",
        .scout_text =
            "Everything here wants the Dawn Key, your life, or both.",
        .npc = "Nothing mortal waits here.",
        .resource = RESOURCE_NONE,
        .danger = 8,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
};
static const char *kWeatherNames[WEATHER_COUNT] = {
    "Clear", "Rain", "Fog", "High Wind", "Ashfall", "Storm"};
static const char *kFragmentNames[FRAGMENT_COUNT] = {
    "Tidal Fragment", "Frost Fragment", "Ember Fragment"};
static int clamp_int(int value, int min_value, int max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}
static uint64_t minutes_to_ms(int minutes) {
  return (uint64_t)minutes * GAME_MINUTE_MS;
}
static int roll_range(int min_value, int max_value) {
  return min_value + rand() % (max_value - min_value + 1);
}
static bool is_blank(const char *text) {
  while (*text != '\0') {
    if (!isspace((unsigned char)*text)) {
      return false;
    }
    text++;
  }
  return true;
}
static void canonicalize_input(const char *input, char *output,
                               size_t output_size) {
  size_t out = 0;
  bool in_space = false;
  const char *cursor = input;
  while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
    cursor++;
  }
  while (*cursor != '\0' && out + 1 < output_size) {
    unsigned char ch = (unsigned char)*cursor;
    if (isspace(ch)) {
      if (!in_space) {
        output[out++] = ' ';
        in_space = true;
      }
    } else {
      output[out++] = (char)tolower(ch);
      in_space = false;
    }
    cursor++;
  }
  if (out > 0 && output[out - 1] == ' ') {
    out--;
  }
  output[out] = '\0';
}
static bool starts_with(const char *text, const char *prefix) {
  while (*prefix != '\0') {
    if (*text != *prefix) {
      return false;
    }
    text++;
    prefix++;
  }
  return true;
}
static uint64_t game_now_ms(void *context) {
  const GameState *game = (const GameState *)context;
  return game->clock_ms;
}
static void push_event(GameState *game, const char *fmt, ...) {
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
static void flush_events(GameState *game) {
  int index;
  for (index = 0; index < game->event_count; index++) {
    printf("[World] %s\n", game->events[index]);
  }
  game->event_count = 0;
}
static int total_minutes_elapsed(const GameState *game) {
  return (int)(game->clock_ms / GAME_MINUTE_MS);
}
static int current_day(const GameState *game) {
  return total_minutes_elapsed(game) / 1440 + 1;
}
static int current_hour(const GameState *game) {
  return (total_minutes_elapsed(game) / 60) % 24;
}
static int current_minute(const GameState *game) {
  return total_minutes_elapsed(game) % 60;
}
static const char *time_band(const GameState *game) {
  int hour = current_hour(game);
  if (hour < 5) {
    return "deep night";
  }
  if (hour < 8) {
    return "dawn";
  }
  if (hour < 12) {
    return "morning";
  }
  if (hour < 17) {
    return "afternoon";
  }
  if (hour < 21) {
    return "evening";
  }
  return "late night";
}
static bool zone_is_safe(int zone) {
  return kZones[zone].safe;
}
static bool zone_has_merchant(const GameState *game, int zone) {
  return kZones[zone].merchant || game->caravan_zone == zone;
}
static int zone_north(int zone) {
  return zone >= 4 ? zone - 4 : ZONE_NONE;
}
static int zone_south(int zone) {
  return zone < 12 ? zone + 4 : ZONE_NONE;
}
static int zone_west(int zone) {
  return zone % 4 != 0 ? zone - 1 : ZONE_NONE;
}
static int zone_east(int zone) {
  return zone % 4 != 3 ? zone + 1 : ZONE_NONE;
}
static int zone_from_direction(int zone, const char *direction) {
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
static void show_exits(int zone) {
  printf("Exits:");
  if (zone_north(zone) != ZONE_NONE) {
    printf(" north");
  }
  if (zone_east(zone) != ZONE_NONE) {
    printf(" east");
  }
  if (zone_south(zone) != ZONE_NONE) {
    printf(" south");
  }
  if (zone_west(zone) != ZONE_NONE) {
    printf(" west");
  }
  printf("\n");
}
static int player_attack_value(const GameState *game) {
  int attack = game->player.strength + game->player.level * 2;
  if (game->player.steel_edge) {
    attack += 4;
  }
  if (game->player.abbey_sigil) {
    attack += 1;
  }
  return attack;
}
static int player_defense_value(const GameState *game) {
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
static void refresh_rumor(GameState *game) {
  if (!game->bandit_reeve_defeated) {
    fill_rumor(
        game,
        "Merchants swear the Bandit Reeve has started collecting relic taxes in "
        "Ironwood Pass.");
    return;
  }
  if (game->fragments_quest == QUEST_ACTIVE &&
      !game->fragment_found[FRAGMENT_TIDAL]) {
    fill_rumor(
        game,
        "A drowned curator still guards the deepest shelves of the Sunken "
        "Archive.");
    return;
  }
  if (game->fragments_quest == QUEST_ACTIVE &&
      !game->fragment_found[FRAGMENT_FROST]) {
    fill_rumor(
        game,
        "Frostspire caravans keep vanishing near a shrine buried under fresh "
        "snow.");
    return;
  }
  if (game->fragments_quest == QUEST_ACTIVE &&
      !game->fragment_found[FRAGMENT_EMBER]) {
    fill_rumor(
        game,
        "Cult embers are circling a glowing chamber somewhere inside the "
        "Obsidian Crater.");
    return;
  }
  if (game->dawn_key_forged && !game->final_boss_defeated) {
    fill_rumor(
        game,
        "The Ruined Basilica has begun ringing without bells. The throne is "
        "waking.");
    return;
  }
  fill_rumor(
      game,
      "Watch fires are holding for now, but every patrol says the Hollow Throne "
      "grows louder at night.");
}
static void process_ready_tasks(GameState *game) {
  int safety = 0;
  while (safety < 256 && Feather_step(&game->feather)) {
    safety++;
  }
}
static void advance_time(GameState *game, int minutes) {
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
  push_event(game, "The weather turns: %s sweeps across the south.",
             kWeatherNames[game->weather]);
}
static void task_restock(void *context) {
  GameState *game = (GameState *)context;
  const int caravan_stops[3] = {ZONE_LANTERN_WARD, ZONE_VERDANT_ABBEY,
                                ZONE_GLOAM_PORT};
  game->market_potions = 4 + rand() % 4;
  game->port_potions = 3 + rand() % 3;
  game->caravan_zone = caravan_stops[rand() % 3];
  push_event(game, "Merchants restock their shelves. A caravan camp opens at %s.",
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
  push_event(game, "%s recovers %d HP while the road briefly quiets.",
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
               "The Hollow Crown surges. Enemies across the realm feel sharper, "
               "hungrier, and less mortal.");
  } else {
    push_event(game, "A wave of dread rolls out from the buried palace. Doom is now %d.",
               game->doom);
  }
}
static void task_rumor(void *context) {
  GameState *game = (GameState *)context;
  refresh_rumor(game);
  push_event(game, "New rumor: %s", game->rumor);
}
static bool enqueue_repeating_task(GameState *game, void (*task)(void *),
                                   int start_minutes, int repeat_minutes,
                                   int8_t priority) {
  return Feather_add_task(
      &game->feather,
      (FSTask){.task = task,
               .context = game,
               .start_time = game->clock_ms + minutes_to_ms(start_minutes),
               .regular = true,
               .execute_cycle = minutes_to_ms(repeat_minutes),
               .priority = priority,
               .repeat_mode = FSTask_Repeat_FIXED_DELAY});
}
static void init_game(GameState *game) {
  memset(game, 0, sizeof(*game));
  snprintf(game->player.name, sizeof(game->player.name), "Warden");
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
static void shutdown_game(GameState *game) {
  Feather_deinit(&game->feather);
}
static void describe_zone(const GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  printf("\n== %s ==\n", zone->name);
  printf("%s\n", zone->description);
  printf("Weather: %s | Day %d, %02d:%02d (%s)\n", kWeatherNames[game->weather],
         current_day(game), current_hour(game), current_minute(game),
         time_band(game));
  if (!zone->safe && zone->danger > 0) {
    printf("Danger: %d/8\n", zone->danger + game->doom / 3);
  } else {
    printf("Danger: Safe ground\n");
  }
  printf("%s\n", zone->scout_text);
  if (zone->npc != NULL && zone->npc[0] != '\0') {
    printf("Notable presence: %s\n", zone->npc);
  }
  if (zone_has_merchant(game, game->player.zone)) {
    if (kZones[game->player.zone].merchant) {
      printf("A resident merchant is open for trade.\n");
    } else {
      printf("A traveling caravan has set up shop here.\n");
    }
  }
  if (zone->forge) {
    printf("The dormant forge can be used with `forge`.\n");
  }
  show_exits(game->player.zone);
}
static void show_help(void) {
  printf("\nCommands:\n");
  printf("  look, map, stats, inventory, quests, rumor, time\n");
  printf("  north/south/east/west, go <direction>, travel <direction>\n");
  printf("  scout, hunt, gather, explore, talk, shop, forge, rest\n");
  printf("  use potion, quit\n");
  printf("\nCombat commands:\n");
  printf("  attack, cleave, guard, potion, bomb, flee, status\n");
}
static void show_map(const GameState *game) {
  int row;
  int col;
  printf("\nWorld Map\n");
  for (row = 0; row < 4; row++) {
    for (col = 0; col < 4; col++) {
      int zone = row * 4 + col;
      const char *label =
          game->player.discovered[zone] ? kZones[zone].short_name : "????";
      if (zone == game->player.zone) {
        printf("[*%s*]", label);
      } else {
        printf("[ %s ]", label);
      }
      if (col < 3) {
        printf("--");
      }
    }
    printf("\n");
    if (row < 3) {
      printf("   |        |        |        |\n");
    }
  }
}
static void show_stats(const GameState *game) {
  printf("\n%s, level %d\n", game->player.name, game->player.level);
  printf("HP %d/%d | XP %d/%d | Gold %d | Doom %d\n", game->player.hp,
         game->player.max_hp, game->player.xp, game->player.xp_to_next,
         game->player.gold, game->doom);
  printf("Attack %d | Defense %d | Victories %d\n", player_attack_value(game),
         player_defense_value(game), game->player.victories);
}
static void show_inventory(const GameState *game) {
  printf("\nInventory\n");
  printf("Potions: %d | Bombs: %d | Herbs: %d | Ore: %d | Relic Dust: %d\n",
         game->player.potions, game->player.bombs, game->player.herbs,
         game->player.ore, game->player.relic_dust);
  printf("Gear: %s, %s, %s\n",
         game->player.steel_edge ? "Steel Edge" : "Traveler's Blade",
         game->player.ward_mail ? "Ward Mail" : "Road Leathers",
         game->player.abbey_sigil ? "Abbey Sigil" : "No Sigil");
  if (game->fragment_found[FRAGMENT_TIDAL] || game->fragment_found[FRAGMENT_FROST] ||
      game->fragment_found[FRAGMENT_EMBER]) {
    printf("Fragments:\n");
    if (game->fragment_found[FRAGMENT_TIDAL]) {
      printf("  %s\n", kFragmentNames[FRAGMENT_TIDAL]);
    }
    if (game->fragment_found[FRAGMENT_FROST]) {
      printf("  %s\n", kFragmentNames[FRAGMENT_FROST]);
    }
    if (game->fragment_found[FRAGMENT_EMBER]) {
      printf("  %s\n", kFragmentNames[FRAGMENT_EMBER]);
    }
  }
  if (game->dawn_key_forged) {
    printf("Key Item: Dawn Key\n");
  }
}
static void show_time(const GameState *game) {
  printf("Day %d, %02d:%02d. It is %s, and the sky is %s.\n", current_day(game),
         current_hour(game), current_minute(game), time_band(game),
         kWeatherNames[game->weather]);
}
static void show_quests(const GameState *game) {
  printf("\nQuest Log\n");
  if (game->remedy_quest == QUEST_ACTIVE) {
    printf("  Sister's Remedy: Bring 3 herbs to Sister Elowen at Verdant Abbey. "
           "(You have %d)\n",
           game->player.herbs);
  } else if (game->remedy_quest == QUEST_COMPLETE) {
    printf("  Sister's Remedy: Completed.\n");
  } else {
    printf("  Sister's Remedy: Not yet accepted.\n");
  }
  if (game->caravan_quest == QUEST_ACTIVE) {
    printf("  Broken Caravan: Hunt the Bandit Reeve in Ironwood Pass.\n");
  } else if (game->caravan_quest == QUEST_COMPLETE) {
    printf("  Broken Caravan: Completed.\n");
  } else {
    printf("  Broken Caravan: Not yet accepted.\n");
  }
  if (game->fragments_quest == QUEST_ACTIVE) {
    printf("  Dawn Fragments: Recover relics from Sunken Archive, Frostspire "
           "Trail, and Obsidian Crater.\n");
    printf("    Sunken Archive: %s\n",
           game->fragment_found[FRAGMENT_TIDAL] ? "Recovered" : "Missing");
    printf("    Frostspire Trail: %s\n",
           game->fragment_found[FRAGMENT_FROST] ? "Recovered" : "Missing");
    printf("    Obsidian Crater: %s\n",
           game->fragment_found[FRAGMENT_EMBER] ? "Recovered" : "Missing");
  } else if (game->fragments_quest == QUEST_COMPLETE) {
    printf("  Dawn Fragments: Completed.\n");
  } else {
    printf("  Dawn Fragments: Locked.\n");
  }
  if (game->crown_quest == QUEST_ACTIVE) {
    printf("  Hollow Crown: Carry the Dawn Key through the Ruined Basilica and "
           "end the throne's hunger.\n");
  } else if (game->crown_quest == QUEST_COMPLETE) {
    printf("  Hollow Crown: Completed.\n");
  } else {
    printf("  Hollow Crown: Locked.\n");
  }
}
static bool read_command(const char *prompt, char *buffer, size_t buffer_size) {
  printf("%s", prompt);
  fflush(stdout);
  if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
    return false;
  }
  buffer[strcspn(buffer, "\n")] = '\0';
  return true;
}
static void grant_xp(GameState *game, int amount) {
  game->player.xp += amount;
  while (game->player.xp >= game->player.xp_to_next) {
    game->player.xp -= game->player.xp_to_next;
    game->player.xp_to_next += 18;
    game->player.level++;
    game->player.max_hp += 7;
    game->player.hp = game->player.max_hp;
    game->player.strength += 2;
    game->player.guard += 1;
    push_event(game,
               "%s rises to level %d. Max HP, attack, and defense all climb.",
               game->player.name, game->player.level);
  }
}
static Enemy make_enemy(const char *name, int hp, int attack, int defense,
                        int xp_reward, int gold_reward, int tier,
                        EnemyRole role, bool boss, bool steals_gold,
                        bool inflicts_weakness, const char *intro,
                        const char *special) {
  Enemy enemy;
  memset(&enemy, 0, sizeof(enemy));
  snprintf(enemy.name, sizeof(enemy.name), "%s", name);
  snprintf(enemy.intro, sizeof(enemy.intro), "%s", intro);
  snprintf(enemy.special, sizeof(enemy.special), "%s", special);
  enemy.max_hp = hp;
  enemy.hp = hp;
  enemy.attack = attack;
  enemy.defense = defense;
  enemy.xp_reward = xp_reward;
  enemy.gold_reward = gold_reward;
  enemy.tier = tier;
  enemy.role = role;
  enemy.boss = boss;
  enemy.steals_gold = steals_gold;
  enemy.inflicts_weakness = inflicts_weakness;
  return enemy;
}
static void scale_enemy(GameState *game, Enemy *enemy, int danger) {
  int world_bonus = game->player.level / 2 + game->doom / 3;
  enemy->max_hp += danger * 2 + world_bonus * 2;
  enemy->hp = enemy->max_hp;
  enemy->attack += danger / 2 + world_bonus;
  enemy->defense += world_bonus / 2;
  enemy->xp_reward += danger * 2 + world_bonus;
  enemy->gold_reward += danger + world_bonus;
}
static Enemy build_regular_enemy(GameState *game, int zone) {
  Enemy enemy;
  int roll = rand() % 100;
  switch (zone) {
  case ZONE_IRONWOOD_PASS:
    if (game->caravan_quest == QUEST_ACTIVE && !game->bandit_reeve_defeated) {
      enemy = make_enemy(
          "Bandit Reeve", 38, 8, 4, 22, 28, 3, ENEMY_ROLE_BOSS, true, true,
          false,
          "A scarred raider captain steps onto the road with a ledger tucked "
          "under one arm and a hooked saber in the other.",
          "The Reeve snaps a signal whistle and dives forward in a ruthless "
          "chain of cuts.");
      scale_enemy(game, &enemy, 3);
      return enemy;
    }
    if (roll < 50) {
      enemy = make_enemy(
          "Rust Wolf", 22, 6, 2, 10, 8, 1, ENEMY_ROLE_SKIRMISHER, false, false,
          false,
          "A rust-furred wolf slips between the ironwood trunks, circling low.",
          "The wolf lunges for the throat.");
    } else {
      enemy = make_enemy(
          "Toll Bandit", 26, 7, 3, 11, 11, 2, ENEMY_ROLE_SKIRMISHER, false,
          true, false,
          "A road bandit drops from the rocks and levels a sawed spear at you.",
          "The bandit darts in with a stabbing rush.");
    }
    break;
  case ZONE_MOONFEN:
    if (roll < 50) {
      enemy = make_enemy(
          "Fen Leech", 24, 7, 2, 12, 9, 2, ENEMY_ROLE_BRUTE, false, false,
          false,
          "The water bulges. A giant leech peels itself from the mud.",
          "The leech crushes forward in a wet, heavy slam.");
    } else {
      enemy = make_enemy(
          "Mire Witch", 21, 7, 2, 13, 12, 2, ENEMY_ROLE_CASTER, false, false,
          true,
          "A mire witch rises on roots and reeds, muttering to the water.",
          "The witch spits a curse that numbs your limbs.");
    }
    break;
  case ZONE_STORMWATCH_CLIFFS:
    if (roll < 50) {
      enemy = make_enemy(
          "Cliff Harrier", 26, 8, 3, 14, 11, 2, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "A harrier vaults from a ledge, blades flashing in the sea spray.",
          "The raider uses the wind to accelerate a vicious strike.");
    } else {
      enemy = make_enemy(
          "Gale Marauder", 30, 8, 4, 15, 13, 3, ENEMY_ROLE_SKIRMISHER, false,
          true, false,
          "A marauder wrapped in sailcloth grins from beneath a storm mask.",
          "The marauder hooks your guard aside and drives in hard.");
    }
    break;
  case ZONE_ASHEN_QUARRY:
    if (roll < 50) {
      enemy = make_enemy(
          "Quarry Ghoul", 28, 8, 4, 15, 12, 2, ENEMY_ROLE_BRUTE, false, false,
          false,
          "A quarry corpse drags itself from an ore seam, jaw full of slag.",
          "The ghoul hammers downward with mining claws.");
    } else {
      enemy = make_enemy(
          "Slag Brute", 33, 9, 5, 17, 15, 3, ENEMY_ROLE_BRUTE, false, false,
          false,
          "A slag brute tears free from a collapsed smelter with molten eyes.",
          "The brute shoulders into you like a charging furnace.");
    }
    break;
  case ZONE_SUNKEN_ARCHIVE:
    if (roll < 50) {
      enemy = make_enemy(
          "Drowned Scribe", 30, 9, 4, 17, 16, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "A drowned scribe climbs the stairs, quills still pinned through its "
          "palms.",
          "The scribe traces a freezing glyph through the water around you.");
    } else {
      enemy = make_enemy(
          "Saltbound Curator", 34, 10, 5, 18, 17, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "A curator draped in algae and chains rises from the collapsed stacks.",
          "The curator speaks a catalog curse that blurs your focus.");
    }
    break;
  case ZONE_FROSTSPIRE_TRAIL:
    if (roll < 50) {
      enemy = make_enemy(
          "Ice Hound", 32, 10, 4, 18, 15, 3, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "Snow erupts and an ice hound streaks across the trail toward you.",
          "The hound springs with a frost-rimed bite.");
    } else {
      enemy = make_enemy(
          "Frost Revenant", 36, 10, 5, 19, 18, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "A revenant in caravan chains emerges from the blowing snow.",
          "The revenant exhales a shard-laced gale into your face.");
    }
    break;
  case ZONE_CINDER_GROVE:
    if (roll < 50) {
      enemy = make_enemy(
          "Ember Stag", 31, 10, 4, 18, 16, 3, ENEMY_ROLE_BRUTE, false, false,
          false,
          "An ember stag lowers a rack of burning antlers and paws sparks into "
          "the ash.",
          "The stag charges, trailing a ribbon of fire.");
    } else {
      enemy = make_enemy(
          "Thornhexer", 29, 10, 4, 18, 17, 3, ENEMY_ROLE_CASTER, false, false,
          true,
          "A thornhexer steps out from the smoke wrapped in living briars.",
          "The hexer hurls barbed smoke that deadens your swing.");
    }
    break;
  case ZONE_OBSIDIAN_CRATER:
    if (roll < 50) {
      enemy = make_enemy(
          "Glass Drake", 38, 11, 5, 22, 20, 4, ENEMY_ROLE_BRUTE, false, false,
          false,
          "A glass drake climbs over the crater rim, shedding shards as it moves.",
          "The drake slams forward on molten talons.");
    } else {
      enemy = make_enemy(
          "Cinder Cultist", 34, 11, 5, 22, 21, 4, ENEMY_ROLE_CASTER, false,
          false, true,
          "A cinder cultist emerges from the vapor with a mask of black glass.",
          "The cultist spits a hymn of flame that shakes your stance.");
    }
    break;
  case ZONE_RUINED_BASILICA:
    if (roll < 50) {
      enemy = make_enemy(
          "Oathbound Shade", 36, 11, 5, 22, 19, 4, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "An oathbound shade steps from a shattered mosaic with sword drawn.",
          "The shade glides through your guard in a silver arc.");
    } else {
      enemy = make_enemy(
          "Choir Wraith", 35, 11, 5, 22, 20, 4, ENEMY_ROLE_CASTER, false, false,
          true,
          "A choir wraith descends from the nave, singing through a split jaw.",
          "The wraith's note cuts straight into your concentration.");
    }
    break;
  default:
    enemy = make_enemy(
        "Road Lurker", 20, 6, 2, 8, 7, 1, ENEMY_ROLE_SKIRMISHER, false, false,
        false, "A scavenger emerges from the roadside shadows.",
        "The lurker darts in with a fast slash.");
    break;
  }
  scale_enemy(game, &enemy, kZones[zone].danger);
  return enemy;
}
static Enemy build_fragment_guardian(GameState *game, FragmentId fragment) {
  Enemy enemy;
  switch (fragment) {
  case FRAGMENT_TIDAL:
    enemy = make_enemy(
        "Drowned Curator", 44, 12, 6, 28, 28, 4, ENEMY_ROLE_BOSS, true, false,
        true,
        "The curator vault door groans open and a towering drowned keeper rises "
        "with chains of brass keys wrapped around both arms.",
        "The curator floods the chamber with a cold archival curse.");
    break;
  case FRAGMENT_FROST:
    enemy = make_enemy(
        "Rime Colossus", 48, 13, 7, 30, 30, 4, ENEMY_ROLE_BOSS, true, false,
        false,
        "A colossal knight of ice and broken caravan wood tears free from the "
        "mountain shrine.",
        "The colossus lifts both arms and brings the whole mountain down.");
    break;
  case FRAGMENT_EMBER:
    enemy = make_enemy(
        "Glass Wyrm", 52, 14, 7, 34, 34, 5, ENEMY_ROLE_BOSS, true, false, true,
        "Lava tears open the crater floor and a wyrm of black glass coils into "
        "the air, carrying an ember shard in its burning throat.",
        "The wyrm screams and the crater answers with a wave of heat.");
    break;
  default:
    enemy = make_enemy(
        "Relic Guardian", 40, 11, 5, 24, 24, 3, ENEMY_ROLE_BOSS, true, false,
        false, "A relic guardian steps from hiding.",
        "The guardian attacks with sudden force.");
    break;
  }
  scale_enemy(game, &enemy, 4);
  return enemy;
}
static Enemy build_final_boss(GameState *game) {
  Enemy boss =
      make_enemy("King Without Dawn", 78, 16, 8, 80, 0, 6, ENEMY_ROLE_BOSS,
                 true, false, true,
                 "The shadow on the throne stands. A king-shaped absence unfolds "
                 "into armor, antlers, and a crown made from everyone it ever "
                 "failed to save.",
                 "The dead king lifts the Hollow Crown and the entire chamber "
                 "leans toward you.");
  scale_enemy(game, &boss, 5);
  if (game->basilica_blessing) {
    boss.max_hp -= 6;
    boss.hp = boss.max_hp;
    boss.attack -= 1;
  }
  return boss;
}
static int compute_damage(int attack_value, int defense_value) {
  int damage = attack_value - defense_value / 2 + roll_range(0, 3);
  return damage < 1 ? 1 : damage;
}
static void award_post_battle_loot(GameState *game, const Enemy *enemy, int zone) {
  int bonus_gold = 0;
  game->player.gold += enemy->gold_reward;
  grant_xp(game, enemy->xp_reward);
  game->player.victories++;
  if (zone == ZONE_MOONFEN || zone == ZONE_CINDER_GROVE) {
    if (rand() % 100 < 35) {
      game->player.herbs++;
      printf("You recover a usable herb bundle from the aftermath.\n");
    }
  } else if (zone == ZONE_IRONWOOD_PASS || zone == ZONE_ASHEN_QUARRY ||
             zone == ZONE_OBSIDIAN_CRATER) {
    if (rand() % 100 < 35) {
      game->player.ore++;
      printf("You pry a chunk of workable ore from the battlefield.\n");
    }
  } else if (zone == ZONE_SUNKEN_ARCHIVE || zone == ZONE_RUINED_BASILICA) {
    if (rand() % 100 < 25) {
      game->player.relic_dust++;
      printf("You collect shimmering relic dust from the ruins.\n");
    }
  }
  if (game->weather == WEATHER_CLEAR) {
    bonus_gold++;
  }
  if (bonus_gold > 0) {
    game->player.gold += bonus_gold;
  }
  printf("Rewards: %d XP, %d gold.\n", enemy->xp_reward,
         enemy->gold_reward + bonus_gold);
}
static BattleResult run_battle(GameState *game, Enemy enemy) {
  char input[MAX_INPUT];
  char command[MAX_INPUT];
  game->combat.active = true;
  game->combat.guard_active = false;
  game->combat.enemy_charging = false;
  game->combat.weaken_turns = 0;
  game->combat.enemy = enemy;
  printf("\n%s\n", game->combat.enemy.intro);
  while (game->combat.enemy.hp > 0 && game->player.hp > 0) {
    int player_damage = 0;
    int enemy_damage = 0;
    bool spend_turn = false;
    printf("\n%s HP %d/%d | %s HP %d/%d\n", game->player.name, game->player.hp,
           game->player.max_hp, game->combat.enemy.name, game->combat.enemy.hp,
           game->combat.enemy.max_hp);
    if (!read_command("Combat> ", input, sizeof(input))) {
      game->running = false;
      game->combat.active = false;
      return BATTLE_RESULT_DEFEAT;
    }
    canonicalize_input(input, command, sizeof(command));
    if (command[0] == '\0') {
      continue;
    }
    if (strcmp(command, "help") == 0) {
      printf("Combat commands: attack, cleave, guard, potion, bomb, flee, "
             "status\n");
      continue;
    }
    if (strcmp(command, "status") == 0) {
      printf("You are %s.\n",
             game->combat.weaken_turns > 0 ? "weakened by hostile magic"
                                           : "holding steady");
      continue;
    }
    if (strcmp(command, "potion") == 0 ||
        strcmp(command, "use potion") == 0) {
      if (game->player.potions <= 0) {
        printf("You are out of potions.\n");
        continue;
      }
      game->player.potions--;
      game->player.hp =
          clamp_int(game->player.hp + 18 + game->player.level * 2, 0,
                    game->player.max_hp);
      printf("You drink a potion and steady yourself.\n");
      spend_turn = true;
    } else if (strcmp(command, "bomb") == 0) {
      if (game->player.bombs <= 0) {
        printf("You have no bombs left.\n");
        continue;
      }
      game->player.bombs--;
      player_damage = 14 + game->player.level * 3;
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("Your bomb detonates for %d damage.\n", player_damage);
      spend_turn = true;
    } else if (strcmp(command, "guard") == 0) {
      game->combat.guard_active = true;
      if (game->player.abbey_sigil) {
        game->player.hp =
            clamp_int(game->player.hp + 2, 0, game->player.max_hp);
        printf("You brace behind your weapon and the abbey sigil flares.\n");
      } else {
        printf("You brace for the next strike.\n");
      }
      spend_turn = true;
    } else if (strcmp(command, "cleave") == 0) {
      int attack = player_attack_value(game) + 6;
      if (game->player.level < 3) {
        printf("You have not yet learned how to commit to a full cleave.\n");
        continue;
      }
      if (game->combat.weaken_turns > 0) {
        attack -= 3;
      }
      player_damage = compute_damage(attack, game->combat.enemy.defense);
      if (rand() % 100 < 20) {
        player_damage += 4 + game->player.level;
        printf("The cleave lands perfectly.\n");
      }
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("You cleave %s for %d damage.\n", game->combat.enemy.name,
             player_damage);
      spend_turn = true;
    } else if (strcmp(command, "attack") == 0) {
      int attack = player_attack_value(game);
      if (game->combat.weaken_turns > 0) {
        attack -= 3;
      }
      player_damage = compute_damage(attack, game->combat.enemy.defense);
      if (rand() % 100 < 12) {
        player_damage += 3 + game->player.level;
        printf("Critical hit.\n");
      }
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("You hit %s for %d damage.\n", game->combat.enemy.name,
             player_damage);
      spend_turn = true;
    } else if (strcmp(command, "flee") == 0) {
      int flee_chance;
      if (game->combat.enemy.boss) {
        printf("There is nowhere to run from this fight.\n");
        continue;
      }
      flee_chance = 45 + game->player.level * 5 - kZones[game->player.zone].danger * 3;
      if (game->weather == WEATHER_FOG) {
        flee_chance += 10;
      }
      if (rand() % 100 < flee_chance) {
        printf("You break away and escape.\n");
        advance_time(game, 10);
        flush_events(game);
        game->combat.active = false;
        return BATTLE_RESULT_FLED;
      }
      printf("You try to disengage, but %s cuts you off.\n",
             game->combat.enemy.name);
      spend_turn = true;
    } else {
      printf("That command does not work in combat.\n");
      continue;
    }
    if (!spend_turn) {
      continue;
    }
    if (game->combat.enemy.hp <= 0) {
      break;
    }
    if (game->combat.enemy_charging) {
      enemy_damage =
          compute_damage(game->combat.enemy.attack + 6, player_defense_value(game));
      printf("%s %s\n", game->combat.enemy.name, game->combat.enemy.special);
      game->combat.enemy_charging = false;
    } else if (game->combat.enemy.boss && rand() % 100 < 20) {
      game->combat.enemy_charging = true;
      printf("%s gathers itself for a crushing follow-up.\n",
             game->combat.enemy.name);
      enemy_damage = 0;
    } else if (game->combat.enemy.inflicts_weakness && rand() % 100 < 25) {
      enemy_damage = compute_damage(game->combat.enemy.attack + 1,
                                    player_defense_value(game));
      game->combat.weaken_turns = 2;
      printf("%s %s\n", game->combat.enemy.name, game->combat.enemy.special);
    } else if (game->combat.enemy.steals_gold && rand() % 100 < 25 &&
               game->player.gold > 0) {
      int stolen = clamp_int(2 + rand() % 5, 1, game->player.gold);
      enemy_damage =
          compute_damage(game->combat.enemy.attack, player_defense_value(game));
      game->player.gold -= stolen;
      printf("%s slashes past your guard and steals %d gold.\n",
             game->combat.enemy.name, stolen);
    } else {
      enemy_damage =
          compute_damage(game->combat.enemy.attack, player_defense_value(game));
      if (game->combat.enemy.role == ENEMY_ROLE_BRUTE) {
        printf("%s crashes into you with brute force.\n", game->combat.enemy.name);
      } else if (game->combat.enemy.role == ENEMY_ROLE_CASTER) {
        printf("%s lashes out with spiteful magic.\n", game->combat.enemy.name);
      } else {
        printf("%s strikes fast.\n", game->combat.enemy.name);
      }
    }
    if (game->combat.guard_active) {
      enemy_damage = (enemy_damage + 1) / 2;
      printf("Your guard absorbs part of the blow.\n");
    }
    if (enemy_damage > 0) {
      game->player.hp = clamp_int(game->player.hp - enemy_damage, 0,
                                  game->player.max_hp);
      printf("You take %d damage.\n", enemy_damage);
    }
    game->combat.guard_active = false;
    if (game->combat.weaken_turns > 0) {
      game->combat.weaken_turns--;
    }
    advance_time(game, 5);
    flush_events(game);
  }
  game->combat.active = false;
  if (game->player.hp <= 0) {
    printf("\n%s falls, and the road goes dark.\n", game->player.name);
    game->running = false;
    return BATTLE_RESULT_DEFEAT;
  }
  printf("\n%s falls.\n", game->combat.enemy.name);
  award_post_battle_loot(game, &game->combat.enemy, game->player.zone);
  return BATTLE_RESULT_VICTORY;
}
static void use_potion_outside_combat(GameState *game) {
  if (game->player.potions <= 0) {
    printf("You do not have any potions.\n");
    return;
  }
  if (game->player.hp >= game->player.max_hp) {
    printf("You are already at full health.\n");
    return;
  }
  game->player.potions--;
  game->player.hp = clamp_int(game->player.hp + 18 + game->player.level * 2, 0,
                              game->player.max_hp);
  printf("You drink a potion and recover to %d/%d HP.\n", game->player.hp,
         game->player.max_hp);
  advance_time(game, 10);
  flush_events(game);
}
static void handle_bandit_boss_victory(GameState *game) {
  game->bandit_reeve_defeated = true;
  game->caravan_quest = QUEST_COMPLETE;
  game->player.gold += 18;
  game->player.relic_dust += 1;
  if (!game->player.steel_edge && game->player.ore >= 4) {
    push_event(game,
               "The recovered ledger identifies hidden ore caches in the south.");
  }
  refresh_rumor(game);
  printf("You recover the broken caravan ledger and scatter the pass gang. "
         "Merchants across the realm breathe easier.\n");
}
static void handle_fragment_victory(GameState *game, FragmentId fragment) {
  game->fragment_found[fragment] = true;
  game->player.relic_dust += 2;
  game->doom = clamp_int(game->doom - 1, 0, 12);
  refresh_rumor(game);
  printf("You secure the %s.\n", kFragmentNames[fragment]);
}
static void hunt_current_zone(GameState *game) {
  Enemy enemy;
  BattleResult result;
  if (kZones[game->player.zone].safe || kZones[game->player.zone].danger <= 0) {
    printf("This area is too secure for open hunting.\n");
    return;
  }
  advance_time(game, 20);
  flush_events(game);
  enemy = build_regular_enemy(game, game->player.zone);
  result = run_battle(game, enemy);
  if (result != BATTLE_RESULT_VICTORY) {
    return;
  }
  if (strcmp(enemy.name, "Bandit Reeve") == 0) {
    handle_bandit_boss_victory(game);
  }
}
static void scout_zone(GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  printf("%s\n", zone->scout_text);
  if (!zone->safe && zone->danger > 0) {
    printf("You judge the effective danger here at roughly %d.\n",
           zone->danger + game->doom / 3);
  }
  if (zone_has_merchant(game, game->player.zone)) {
    printf("Trade is available here.\n");
  }
  if (game->player.zone == ZONE_HOLLOW_THRONE && game->dawn_key_forged) {
    printf("The throne is awake. There will be no soft entrance.\n");
  }
  advance_time(game, 10);
  flush_events(game);
}
static void gather_resources(GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  int amount = 1;
  if (zone->resource == RESOURCE_NONE) {
    printf("There is nothing obvious to gather here.\n");
    return;
  }
  if (game->weather == WEATHER_CLEAR || game->weather == WEATHER_RAIN) {
    amount++;
  }
  if (zone->resource == RESOURCE_HERB) {
    game->player.herbs += amount;
    printf("You gather %d herb bundle%s.\n", amount, amount == 1 ? "" : "s");
  } else if (zone->resource == RESOURCE_ORE) {
    game->player.ore += amount;
    printf("You haul away %d ore chunk%s.\n", amount, amount == 1 ? "" : "s");
  }
  advance_time(game, 35);
  flush_events(game);
  if (!zone->safe && rand() % 100 < 30) {
    printf("Your work draws unwanted attention.\n");
    hunt_current_zone(game);
  }
}
static void explore_special_location(GameState *game) {
  BattleResult result;
  switch (game->player.zone) {
  case ZONE_SUNKEN_ARCHIVE:
    if (game->fragments_quest != QUEST_ACTIVE) {
      printf("You search the archive but without the archivist's notes the "
             "deep vaults remain impossible to read.\n");
      advance_time(game, 25);
      flush_events(game);
      return;
    }
    if (game->fragment_found[FRAGMENT_TIDAL]) {
      printf("The curator vault lies open and empty now.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("You dive below the collapsed stacks and force open the curator vault.\n");
    advance_time(game, 25);
    flush_events(game);
    result = run_battle(game, build_fragment_guardian(game, FRAGMENT_TIDAL));
    if (result == BATTLE_RESULT_VICTORY) {
      handle_fragment_victory(game, FRAGMENT_TIDAL);
    }
    return;
  case ZONE_FROSTSPIRE_TRAIL:
    if (game->fragments_quest != QUEST_ACTIVE) {
      printf("Without the library's route notes, the ice shrines all look the "
             "same through the storm.\n");
      advance_time(game, 25);
      flush_events(game);
      return;
    }
    if (game->fragment_found[FRAGMENT_FROST]) {
      printf("The frozen shrine has already yielded its relic.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("You follow a half-buried processional path toward a frozen shrine.\n");
    advance_time(game, 25);
    flush_events(game);
    result = run_battle(game, build_fragment_guardian(game, FRAGMENT_FROST));
    if (result == BATTLE_RESULT_VICTORY) {
      handle_fragment_victory(game, FRAGMENT_FROST);
    }
    return;
  case ZONE_OBSIDIAN_CRATER:
    if (game->fragments_quest != QUEST_ACTIVE) {
      printf("The crater's vents force you back before you can safely descend.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    if (game->fragment_found[FRAGMENT_EMBER]) {
      printf("The chamber where the wyrm nested has gone quiet.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("You descend toward the crater's molten heart.\n");
    advance_time(game, 25);
    flush_events(game);
    result = run_battle(game, build_fragment_guardian(game, FRAGMENT_EMBER));
    if (result == BATTLE_RESULT_VICTORY) {
      handle_fragment_victory(game, FRAGMENT_EMBER);
    }
    return;
  case ZONE_RUINED_BASILICA:
    if (!game->dawn_key_forged) {
      printf("The basilica rejects you. Every aisle seems to loop back toward the "
             "broken doors.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    if (game->basilica_blessing) {
      printf("You have already taken the basilica's last blessing.\n");
      advance_time(game, 15);
      flush_events(game);
      return;
    }
    printf("You kneel beneath the collapsed dome while the Dawn Key shines "
           "through the dust.\n");
    game->basilica_blessing = true;
    game->doom = clamp_int(game->doom - 1, 0, 12);
    advance_time(game, 30);
    flush_events(game);
    printf("A pale ward settles onto your armor. The throne's pull weakens.\n");
    return;
  case ZONE_HOLLOW_THRONE:
    if (!game->dawn_key_forged) {
      printf("The sealed palace doors reject your approach.\n");
      return;
    }
    printf("You step into the throne chamber and the crown turns toward you.\n");
    result = run_battle(game, build_final_boss(game));
    if (result == BATTLE_RESULT_VICTORY) {
      game->final_boss_defeated = true;
      game->crown_quest = QUEST_COMPLETE;
      game->running = false;
      printf("\nThe Dawn Key breaks, the Hollow Crown shatters, and the buried "
             "palace finally exhales.\n");
    }
    return;
  default:
    printf("You search carefully and turn up scraps of lore, old coins, and "
           "nothing urgent.\n");
    game->player.gold += 3 + rand() % 4;
    if (rand() % 100 < 30) {
      game->player.relic_dust++;
    }
    advance_time(game, 25);
    flush_events(game);
    return;
  }
}
static void talk_here(GameState *game) {
  switch (game->player.zone) {
  case ZONE_EMBERFALL_GATE:
    if (!game->player.claimed_gate_supplies) {
      game->player.claimed_gate_supplies = true;
      game->player.potions++;
      game->player.bombs++;
      printf("Quartermaster Iven presses extra supplies into your hands and "
             "orders you not to die cheaply.\n");
    } else {
      printf("Iven studies the road. \"The south changes every time you spend "
             "an hour in it. Stay ahead of the weather and the rumors.\"\n");
    }
    break;
  case ZONE_BRASS_MARKET:
    if (game->caravan_quest == QUEST_LOCKED) {
      game->caravan_quest = QUEST_ACTIVE;
      refresh_rumor(game);
      printf("Merchant Sal slams a broken ledger onto the counter. \"Bandit Reeve "
             "hit my last caravan in Ironwood Pass. Bring me the ledger and I'll "
             "owe you more than coin.\"\n");
    } else if (game->caravan_quest == QUEST_ACTIVE) {
      printf("\"Ironwood Pass,\" Sal repeats. \"The Reeve never stays hidden for "
             "long if you hunt loud enough.\"\n");
    } else {
      printf("Sal grins. \"The pass is breathing again. If only the rest of the "
             "realm would copy it.\"\n");
    }
    break;
  case ZONE_VERDANT_ABBEY:
    if (game->remedy_quest == QUEST_LOCKED) {
      game->remedy_quest = QUEST_ACTIVE;
      printf("Sister Elowen places three empty satchels on the table. \"Bring "
             "me 3 fresh herb bundles from Moonfen or Cinder Grove. The wounded "
             "won't survive the week without them.\"\n");
    } else if (game->remedy_quest == QUEST_ACTIVE && game->player.herbs >= 3) {
      game->player.herbs -= 3;
      game->remedy_quest = QUEST_COMPLETE;
      game->player.abbey_sigil = true;
      game->player.max_hp += 4;
      game->player.hp = game->player.max_hp;
      game->player.potions += 2;
      printf("Elowen brews through the night, then hangs a silver sigil around "
             "your neck. \"Carry our blessing into the dark.\"\n");
    } else if (game->remedy_quest == QUEST_ACTIVE) {
      printf("\"I still need 3 herb bundles,\" Elowen says gently. \"You have %d.\"\n",
             game->player.herbs);
    } else {
      game->player.hp = game->player.max_hp;
      printf("The sisters patch your wounds and send you back to the road "
             "restored.\n");
    }
    break;
  case ZONE_WHISPER_LIBRARY:
    if (game->fragments_quest == QUEST_LOCKED) {
      if (game->player.level < 3 && game->caravan_quest != QUEST_COMPLETE) {
        printf("Archivist Sen studies you over a tower of maps. \"You are not yet "
               "ready for the fragment hunt. See more of the south first.\"\n");
      } else {
        game->fragments_quest = QUEST_ACTIVE;
        refresh_rumor(game);
        printf("Sen unfolds three brittle charts. \"The Dawn Key was shattered. "
               "Recover its fragments from the Sunken Archive, Frostspire Trail, "
               "and Obsidian Crater, and I can forge a way into the throne.\"\n");
      }
    } else if (game->fragments_quest == QUEST_ACTIVE &&
               game->fragment_found[FRAGMENT_TIDAL] &&
               game->fragment_found[FRAGMENT_FROST] &&
               game->fragment_found[FRAGMENT_EMBER]) {
      game->fragments_quest = QUEST_COMPLETE;
      game->crown_quest = QUEST_ACTIVE;
      game->dawn_key_forged = true;
      game->doom = clamp_int(game->doom - 2, 0, 12);
      refresh_rumor(game);
      printf("Sen binds the recovered shards into the Dawn Key. Light runs "
             "through the cracks like a held sunrise. \"Now you can reach the "
             "Hollow Throne.\"\n");
    } else if (game->dawn_key_forged) {
      printf("\"The basilica will answer the key,\" Sen says. \"Take every edge "
             "you can before you step through.\"\n");
    } else {
      printf("Sen taps the maps one by one. \"Archive. Frostspire. Crater. "
             "Bring back what the crown broke.\"\n");
    }
    break;
  case ZONE_GLOAM_PORT:
    if (game->dawn_key_forged) {
      printf("Captain Mirelle points inland. \"Once you're through the basilica, "
             "don't stop. The throne feeds on hesitation.\"\n");
    } else {
      printf("Mirelle studies the tide marks on an old harbor chart. \"Nothing "
             "useful stays buried around here, only hidden. Keep pushing east "
             "and south.\"\n");
    }
    break;
  case ZONE_LANTERN_WARD:
    printf("Courier Nara passes along patrol notes and fresh warnings about the "
           "roads beyond the ward.\n");
    break;
  default:
    printf("There is no one here inclined to talk.\n");
    return;
  }
  advance_time(game, 20);
  flush_events(game);
}
static int potion_price(const GameState *game, bool port_prices) {
  int price = port_prices ? 13 : 12;
  if (game->caravan_quest == QUEST_COMPLETE) {
    price -= 2;
  }
  return price;
}
static void shop_here(GameState *game) {
  char input[MAX_INPUT];
  char command[MAX_INPUT];
  int *stock = NULL;
  bool port_prices = false;
  if (!zone_has_merchant(game, game->player.zone)) {
    printf("There is no one trading here right now.\n");
    return;
  }
  if (game->player.zone == ZONE_GLOAM_PORT) {
    stock = &game->port_potions;
    port_prices = true;
  } else if (game->player.zone == ZONE_BRASS_MARKET) {
    stock = &game->market_potions;
  }
  printf("Type `buy potion`, `buy bomb`, or `leave`.\n");
  while (game->running) {
    int price = potion_price(game, port_prices);
    int bomb_price = port_prices ? 18 : 20;
    printf("Stock: potions %d | potion %dg | bomb %dg\n",
           stock != NULL ? *stock : 2, price, bomb_price);
    if (!read_command("Shop> ", input, sizeof(input))) {
      game->running = false;
      return;
    }
    canonicalize_input(input, command, sizeof(command));
    if (command[0] == '\0') {
      continue;
    }
    if (strcmp(command, "leave") == 0 || strcmp(command, "exit") == 0) {
      return;
    }
    if (strcmp(command, "buy potion") == 0) {
      if (game->player.gold < price) {
        printf("You do not have enough gold.\n");
        continue;
      }
      if (stock != NULL && *stock <= 0) {
        printf("That merchant is sold out.\n");
        continue;
      }
      game->player.gold -= price;
      game->player.potions++;
      if (stock != NULL) {
        (*stock)--;
      }
      printf("Potion purchased.\n");
      advance_time(game, 10);
      flush_events(game);
      continue;
    }
    if (strcmp(command, "buy bomb") == 0) {
      if (game->player.gold < bomb_price) {
        printf("You do not have enough gold.\n");
        continue;
      }
      game->player.gold -= bomb_price;
      game->player.bombs++;
      printf("Bomb purchased.\n");
      advance_time(game, 10);
      flush_events(game);
      continue;
    }
    printf("The merchant does not understand that request.\n");
  }
}
static void forge_here(GameState *game) {
  char input[MAX_INPUT];
  char command[MAX_INPUT];
  if (!kZones[game->player.zone].forge) {
    printf("There is no working forge here.\n");
    return;
  }
  printf("Type `craft blade`, `craft mail`, `craft bomb`, or `leave`.\n");
  while (game->running) {
    printf("Ore %d | Blade 4 ore | Mail 6 ore | Bomb 2 ore\n", game->player.ore);
    if (!read_command("Forge> ", input, sizeof(input))) {
      game->running = false;
      return;
    }
    canonicalize_input(input, command, sizeof(command));
    if (strcmp(command, "leave") == 0 || strcmp(command, "exit") == 0) {
      return;
    }
    if (strcmp(command, "craft blade") == 0) {
      if (game->player.steel_edge) {
        printf("Your blade is already reforged.\n");
        continue;
      }
      if (game->player.ore < 4) {
        printf("You need 4 ore.\n");
        continue;
      }
      game->player.ore -= 4;
      game->player.steel_edge = true;
      printf("You reforge your weapon into the Steel Edge.\n");
      advance_time(game, 45);
      flush_events(game);
      continue;
    }
    if (strcmp(command, "craft mail") == 0) {
      if (game->player.ward_mail) {
        printf("Your armor is already reinforced.\n");
        continue;
      }
      if (game->player.ore < 6) {
        printf("You need 6 ore.\n");
        continue;
      }
      game->player.ore -= 6;
      game->player.ward_mail = true;
      printf("You temper fresh plates into your armor and finish a set of ward "
             "mail.\n");
      advance_time(game, 55);
      flush_events(game);
      continue;
    }
    if (strcmp(command, "craft bomb") == 0) {
      if (game->player.ore < 2) {
        printf("You need 2 ore.\n");
        continue;
      }
      game->player.ore -= 2;
      game->player.bombs++;
      printf("You shape a rough but effective bomb.\n");
      advance_time(game, 20);
      flush_events(game);
      continue;
    }
    printf("The forge does not respond to that command.\n");
  }
}
static void rest_here(GameState *game) {
  int cost = 0;
  if (game->player.zone == ZONE_EMBERFALL_GATE ||
      game->player.zone == ZONE_VERDANT_ABBEY) {
    cost = 0;
  } else if (game->player.zone == ZONE_BRASS_MARKET ||
             game->player.zone == ZONE_GLOAM_PORT ||
             game->player.zone == ZONE_LANTERN_WARD) {
    cost = 8;
  } else {
    printf("This is not a place where you can safely rest.\n");
    return;
  }
  if (game->player.gold < cost) {
    printf("You cannot afford a proper room.\n");
    return;
  }
  game->player.gold -= cost;
  advance_time(game, 480);
  game->player.hp = game->player.max_hp;
  printf("You rest until your strength returns. HP restored to %d.\n",
         game->player.hp);
  flush_events(game);
}
static bool move_player(GameState *game, const char *direction) {
  int next_zone = zone_from_direction(game->player.zone, direction);
  if (next_zone == ZONE_NONE) {
    printf("That path is closed.\n");
    return false;
  }
  if (next_zone == ZONE_HOLLOW_THRONE && !game->dawn_key_forged) {
    printf("A sealed pressure in the earth turns you back. Something deeper "
           "than stone still bars the palace.\n");
    return false;
  }
  game->player.zone = next_zone;
  game->player.discovered[next_zone] = true;
  advance_time(game, 30);
  flush_events(game);
  describe_zone(game);
  return true;
}
static bool maybe_handle_movement_command(GameState *game, const char *command) {
  const char *direction = NULL;
  if (strcmp(command, "north") == 0 || strcmp(command, "n") == 0 ||
      strcmp(command, "south") == 0 || strcmp(command, "s") == 0 ||
      strcmp(command, "east") == 0 || strcmp(command, "e") == 0 ||
      strcmp(command, "west") == 0 || strcmp(command, "w") == 0) {
    direction = command;
  } else if (starts_with(command, "go ")) {
    direction = command + 3;
  } else if (starts_with(command, "travel ")) {
    direction = command + 7;
  }
  if (direction == NULL) {
    return false;
  }
  move_player(game, direction);
  return true;
}
static void print_intro(void) {
  printf("Feather RPG: Ashes of the Hollow Crown\n");
  printf("A large-volume text RPG powered by Feather's cooperative scheduler.\n");
  printf("As you spend time, the world keeps moving: weather shifts, caravans "
         "travel, rumors spread, doom rises, and the road heals or harms you.\n");
}
int main(void) {
  GameState game;
  char input[MAX_INPUT];
  char command[MAX_INPUT];
  srand((unsigned int)time(NULL));
  init_game(&game);
  print_intro();
  if (read_command("Name your warden (blank for Warden): ", input,
                   sizeof(input)) &&
      !is_blank(input)) {
    snprintf(game.player.name, sizeof(game.player.name), "%s", input);
  }
  printf("\n%s arrives at Emberfall Gate as the southern patrol fires begin to "
         "fade.\n",
         game.player.name);
  describe_zone(&game);
  show_help();
  while (game.running) {
    if (!read_command("\nCommand> ", input, sizeof(input))) {
      break;
    }
    canonicalize_input(input, command, sizeof(command));
    if (command[0] == '\0') {
      continue;
    }
    if (maybe_handle_movement_command(&game, command)) {
      continue;
    }
    if (strcmp(command, "help") == 0) {
      show_help();
    } else if (strcmp(command, "look") == 0 || strcmp(command, "l") == 0) {
      describe_zone(&game);
    } else if (strcmp(command, "map") == 0) {
      show_map(&game);
    } else if (strcmp(command, "stats") == 0 || strcmp(command, "status") == 0) {
      show_stats(&game);
    } else if (strcmp(command, "inventory") == 0 ||
               strcmp(command, "inv") == 0) {
      show_inventory(&game);
    } else if (strcmp(command, "quests") == 0 ||
               strcmp(command, "journal") == 0) {
      show_quests(&game);
    } else if (strcmp(command, "time") == 0) {
      show_time(&game);
    } else if (strcmp(command, "rumor") == 0 ||
               strcmp(command, "rumours") == 0 ||
               strcmp(command, "rumors") == 0) {
      printf("%s\n", game.rumor);
    } else if (strcmp(command, "scout") == 0) {
      scout_zone(&game);
    } else if (strcmp(command, "hunt") == 0 ||
               strcmp(command, "fight") == 0) {
      hunt_current_zone(&game);
    } else if (strcmp(command, "gather") == 0) {
      gather_resources(&game);
    } else if (strcmp(command, "explore") == 0 ||
               strcmp(command, "search") == 0) {
      explore_special_location(&game);
    } else if (strcmp(command, "talk") == 0) {
      talk_here(&game);
    } else if (strcmp(command, "shop") == 0) {
      shop_here(&game);
    } else if (strcmp(command, "forge") == 0) {
      forge_here(&game);
    } else if (strcmp(command, "rest") == 0) {
      rest_here(&game);
    } else if (strcmp(command, "use potion") == 0 ||
               strcmp(command, "potion") == 0) {
      use_potion_outside_combat(&game);
    } else if (strcmp(command, "quit") == 0 ||
               strcmp(command, "exit") == 0) {
      printf("You turn back toward the nearest fire and leave the south for "
             "another day.\n");
      break;
    } else {
      printf("Unknown command. Type `help` for the command list.\n");
    }
  }
  if (game.final_boss_defeated) {
    printf("\nVictory. The roads will still need wardens, but they will no "
           "longer answer to the Hollow Crown.\n");
  } else if (!game.running && game.player.hp <= 0) {
    printf("Game over.\n");
  }
  shutdown_game(&game);
  return 0;
}