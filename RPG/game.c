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
static const ZoneData kZones[ZONE_COUNT] = {
    [ZONE_EMBERFALL_GATE] = {
        .name = "엠버폴 관문",
        .short_name = "관문",
        .description =
            "현무암 관문이 옛 왕국 가도를 지키고 있습니다. 보급 화로는 밤새 타오르고, "
            "남쪽으로 향하는 모든 여행자는 잿빛 벌판 너머에서 죽은 궁정이 깨어난다는 "
            "소문을 속삭입니다.",
        .scout_text =
            "새로운 군화 자국, 병참 깃발, 안전한 야영지. 더 깊이 들어가기 전에 "
            "재정비하기 가장 좋은 곳입니다.",
        .npc = "병참장교 아이븐",
        .resource = RESOURCE_NONE,
        .danger = 0,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_BRASS_MARKET] = {
        .name = "황동 시장",
        .short_name = "시장",
        .description =
            "기워 붙인 천막 아래 시장이 간신히 버티고 있습니다. 등불과 흥정이 가득한 "
            "곳으로, 밀수꾼·유물 상인·상단 경비가 동전만큼 빠르게 소문을 주고받습니다.",
        .scout_text =
            "혼잡하고 시끄럽지만 쓸모 있는 곳입니다. 포션이 빠르게 팔려 나가고, 겁먹은 "
            "상인들은 계속 철목 고개 쪽을 가리킵니다.",
        .npc = "상인 살",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = true,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_LANTERN_WARD] = {
        .name = "등불 구역",
        .short_name = "등구",
        .description =
            "줄지은 감시탑과 좁은 골목이 잠들지 않는 구역을 둘러싸고 있습니다. 전령은 "
            "봉인된 명령서를 나르고, 기록관들은 낡은 지도를 따라갈 용감한 이를 찾습니다.",
        .scout_text =
            "구역은 안전하지만 모든 망루가 남쪽 길을 주시하고 있습니다. 전령들은 이동 "
            "상단 야영지와 절벽 쪽의 기이한 날씨를 언급합니다.",
        .npc = "전령 나라",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_WHISPER_LIBRARY] = {
        .name = "속삭임 도서관",
        .short_name = "도관",
        .description =
            "부러진 갈비뼈처럼 기울어진 서가가 차가운 관측 돔을 에워쌉니다. 도서관엔 "
            "여전히 결계 문양의 울림이 남아 있고, 모든 탁자는 침수 기록고·빙설 길·"
            "분화구 가장자리 지도로 덮여 있습니다.",
        .scout_text =
            "안전한 벽, 위태로운 지식. 기록관들은 유물 파편을 해독할 수 있지만, 먼저 "
            "가져와 줄 사람이 필요합니다.",
        .npc = "기록관 센",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
    [ZONE_IRONWOOD_PASS] = {
        .name = "철목 고개",
        .short_name = "철목",
        .description =
            "검은 소나무가 붉은 암석을 가른 가파른 길 주변에 얽혀 있습니다. 고개에는 "
            "수레 바퀴 소리, 도적의 휘파람, 벼락에 갈라진 고목의 파열음이 메아리칩니다.",
        .scout_text =
            "도적은 능선을 장악하고 늑대는 낮은 수풀을 맴돕니다. 도로 절개면에는 풍부한 "
            "광맥이 드러나 있습니다.",
        .npc = "이곳에는 믿을 만한 동맹이 없습니다.",
        .resource = RESOURCE_ORE,
        .danger = 2,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_VERDANT_ABBEY] = {
        .name = "청록 수도원",
        .short_name = "수원",
        .description =
            "반쯤 무너진 수도원에는 약초 담쟁이와 월광화가 무성합니다. 예배당 종은 "
            "사라졌지만 수녀들은 여전히 갑옷을 고치고 상처를 꿰매며 죽어가는 시골의 "
            "기록을 남깁니다.",
        .scout_text =
            "남부에 남은 가장 온화한 장소입니다. 수녀들은 절박한 이들을 치료하며, 이를 "
            "이어가기 위해 늪의 약초가 필요합니다.",
        .npc = "수녀 엘로웬",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = true,
        .forge = false,
        .archive = false,
    },
    [ZONE_MOONFEN] = {
        .name = "월광 늪",
        .short_name = "월늪",
        .description =
            "흘러다니는 등불 버섯 아래로 은빛 갈대와 검은 물이 펼쳐집니다. 멀리서 보면 "
            "평온하지만, 눈이 지나치게 많은 무언가가 수면 아래서 움직이기 전까지만 "
            "그렇습니다.",
        .scout_text =
            "월엽초가 무성하지만 거머리와 늪 마녀도 함께 번성합니다. 용기보다 신중한 "
            "발걸음이 더 오래 살게 해줍니다.",
        .npc = "늪만이 메아리칩니다.",
        .resource = RESOURCE_HERB,
        .danger = 3,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_STORMWATCH_CLIFFS] = {
        .name = "폭풍감시 절벽",
        .short_name = "절벽",
        .description =
            "난삽한 절벽이 회색 바다 위로 솟아 있습니다. 난파선 조각이 파도에 휩쓸리고, "
            "감시 초소 사이로 바람 깃발이 펄럭입니다. 모든 바위턱이 한 번의 돌풍만으로 "
            "무덤이 될 듯합니다.",
        .scout_text =
            "민첩한 약탈자들이 이곳 바위턱에서 기습합니다. 바람이 돌아서면 절벽은 사각지대와 "
            "비명 같은 바람으로 가득한 시련장이 됩니다.",
        .npc = "버려진 봉화만 타닥거립니다.",
        .resource = RESOURCE_NONE,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_ASHEN_QUARRY] = {
        .name = "잿빛 채석장",
        .short_name = "채석",
        .description =
            "채석장 바닥엔 오래된 불빛이 남아 있습니다. 부서진 석판 위로 체인 리프트가 "
            "매달려 있고, 버려진 대장간엔 적절한 광석만 있다면 새 강철을 단련할 열이 "
            "아직 남아 있습니다.",
        .scout_text =
            "좋은 광석, 나쁜 동행. 채석장 망자와 슬래그 괴수가 구덩이를 배회하지만, "
            "숙련된 손이라면 낡은 모루를 다시 쓸 수 있습니다.",
        .npc = "대장간은 아직 사용할 수 있습니다.",
        .resource = RESOURCE_ORE,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = true,
        .archive = false,
    },
    [ZONE_SUNKEN_ARCHIVE] = {
        .name = "침수 기록고",
        .short_name = "기록",
        .description =
            "수십 년 전 도서 지구가 조수 분지로 무너져 내렸습니다. 기울어진 서가 사이 "
            "대리석 계단은 물속으로 사라지고, 익사한 서기관들이 수면 아래 목록 회랑을 "
            "여전히 순찰합니다.",
        .scout_text =
            "수면 아래 오래된 결계가 깜박입니다. 유물 파편이 남아 있다면 아래의 "
            "큐레이터 금고에 있을 가능성이 큽니다.",
        .npc = "조수만이 시간을 셉니다.",
        .resource = RESOURCE_NONE,
        .danger = 5,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
    [ZONE_GLOAM_PORT] = {
        .name = "황혼 항구",
        .short_name = "항구",
        .description =
            "더 이상 배가 떠나지 않지만 부두에는 중개인, 뱃사공, 수집꾼이 등불과 갈고리로 "
            "일을 이어갑니다. 항만장은 말보다 더 많은 것을 알고 있습니다.",
        .scout_text =
            "여기서는 아직 보급품을 살 수 있고, 노선에 익숙한 선장들은 빙첨로로 가는 길을 "
            "어떤 지도보다 정확히 압니다.",
        .npc = "미렐 선장",
        .resource = RESOURCE_NONE,
        .danger = 2,
        .safe = true,
        .merchant = true,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_FROSTSPIRE_TRAIL] = {
        .name = "빙첨로",
        .short_name = "빙첨",
        .description =
            "부서진 석상 아래 길은 얼어붙은 선반 지형으로 좁아집니다. 이곳의 눈은 녹지 "
            "않고, 산의 망자를 피하려다 쓰러진 모든 상단의 뼈 위에 층층이 쌓입니다.",
        .scout_text =
            "이 길의 고요함은 가장 불길한 종류입니다. 망령이 눈 아래 숨어 있고, 추위 "
            "자체가 포식자처럼 물어뜯습니다.",
        .npc = "산바람은 모든 질문에 이빨로 답합니다.",
        .resource = RESOURCE_NONE,
        .danger = 6,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_CINDER_GROVE] = {
        .name = "숯불 수림",
        .short_name = "수림",
        .description =
            "그을린 나무줄기 사이에서도 이상하게 꽃이 피는 숲입니다. 흙은 따뜻하고 공기엔 "
            "삼나무와 재 냄새가 섞여 있으며, 폭풍 뒤마다 희귀 약초가 그을음 사이로 "
            "돋아납니다.",
        .scout_text =
            "이 숲은 신중한 채집에 보답합니다. 날씨가 바뀌면 불사슴과 가시주술사가 연기 "
            "속에 숨어듭니다.",
        .npc = "수피엔 뿔자국만 남아 있습니다.",
        .resource = RESOURCE_HERB,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_OBSIDIAN_CRATER] = {
        .name = "흑요 분화구",
        .short_name = "분화",
        .description =
            "검은 유리질 지면과 붉은 증기가 뒤엉킨 거대한 충돌 분지입니다. 지각 아래 "
            "용암맥이 맥박치고, 광신도들은 여전히 분화구 가장자리를 땅속 무언가를 위한 "
            "성당처럼 떠받듭니다.",
        .scout_text =
            "강한 광석과 더 강한 적이 있는 곳입니다. 유물 파편을 품을 만큼 강한 존재라면 "
            "불꽃과 파편 속에서 이곳에서 깨어날 것입니다.",
        .npc = "분화구가 스스로에게 주문을 외웁니다.",
        .resource = RESOURCE_ORE,
        .danger = 7,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_RUINED_BASILICA] = {
        .name = "폐허 대성당",
        .short_name = "성당",
        .description =
            "한때 황금빛이던 대성당은 이제 지붕 없이 어두운 하늘 아래 서 있습니다. "
            "기도 모자이크는 파편으로만 남았고, 중앙 회랑은 왕관이 깨어나는 매몰 왕좌를 "
            "향해 화살처럼 뻗어 있습니다.",
        .scout_text =
            "대성당은 경계선입니다. 영혼들이 이곳에 모이고, 여기서 얻어낼 수 있는 축복은 "
            "왕좌 앞에서 큰 의미를 가질 수 있습니다.",
        .npc = "메아리가 부서진 전례문을 읊습니다.",
        .resource = RESOURCE_NONE,
        .danger = 6,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_HOLLOW_THRONE] = {
        .name = "공허 왕좌",
        .short_name = "왕좌",
        .description =
            "묻힌 궁전은 돌뿌리와 금 간 대리석으로 된 거대한 성소로 이어집니다. 중심엔 "
            "그림자에 감싼 빈 왕좌가 있고, 왕국의 마지막 굶주림이 자신의 이름을 되찾으려 "
            "버둥거립니다.",
        .scout_text =
            "이곳의 모든 것은 여명의 열쇠를, 당신의 목숨을, 혹은 그 둘 모두를 원합니다.",
        .npc = "이곳엔 필멸의 존재가 기다리지 않습니다.",
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
    "맑음", "비", "안개", "강풍", "화산재", "폭풍"};
static const char *kFragmentNames[FRAGMENT_COUNT] = {
    "조수 파편", "서리 파편", "불씨 파편"};
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
    printf("[세계] %s\n", game->events[index]);
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
  snprintf(game->player.name, sizeof(game->player.name), "수호자");
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
static bool save_game(const GameState *game, const char *path) {
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
  memcpy(data.fragment_found, game->fragment_found, sizeof(data.fragment_found));
  data.bandit_reeve_defeated = game->bandit_reeve_defeated;
  data.dawn_key_forged = game->dawn_key_forged;
  data.basilica_blessing = game->basilica_blessing;
  data.final_boss_defeated = game->final_boss_defeated;
  data.running = game->running;
  if (fwrite(&data, sizeof(data), 1, file) != 1) {
    fclose(file);
    return false;
  }
  fclose(file);
  return true;
}
static bool load_game(GameState *game, const char *path) {
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
  memcpy(game->fragment_found, data.fragment_found, sizeof(game->fragment_found));
  game->bandit_reeve_defeated = data.bandit_reeve_defeated;
  game->dawn_key_forged = data.dawn_key_forged;
  game->basilica_blessing = data.basilica_blessing;
  game->final_boss_defeated = data.final_boss_defeated;
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
static void describe_zone(const GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  printf("\n== %s ==\n", zone->name);
  printf("%s\n", zone->description);
  printf("날씨: %s | %d일차 %02d:%02d (%s)\n", kWeatherNames[game->weather],
         current_day(game), current_hour(game), current_minute(game),
         time_band(game));
  if (!zone->safe && zone->danger > 0) {
    printf("위험도: %d/8\n", zone->danger + game->doom / 3);
  } else {
    printf("위험도: 안전 지대\n");
  }
  printf("%s\n", zone->scout_text);
  if (zone->npc != NULL && zone->npc[0] != '\0') {
    printf("주요 인물/징후: %s\n", zone->npc);
  }
  if (zone_has_merchant(game, game->player.zone)) {
    if (kZones[game->player.zone].merchant) {
      printf("상인이 상점을 열고 있습니다.\n");
    } else {
      printf("이동 상단이 이곳에 임시 상점을 열었습니다.\n");
    }
  }
  if (zone->forge) {
    printf("`forge` 명령으로 대장간을 사용할 수 있습니다.\n");
  }
  show_exits(game->player.zone);
}
static void show_help(void) {
  printf("\n명령어:\n");
  printf("  look [둘러보기], map [지도], stats [능력치], inventory [소지품]\n");
  printf("  quests [퀘스트], rumor [소문], time [시간]\n");
  printf("  north/south/east/west [이동], go [방향], travel [방향]\n");
  printf("  scout [정찰], hunt [사냥], gather [채집], explore [탐사]\n");
  printf("  talk [대화], shop [상점], forge [대장간], rest [휴식]\n");
  printf("  use potion [포션 사용], save [저장], load [불러오기], quit [종료]\n");
  printf("\n전투 명령어:\n");
  printf("  attack [공격], cleave [강공], guard [방어], potion [포션]\n");
  printf("  bomb [폭탄], flee [도주], status [상태]\n");
}
static void show_map(const GameState *game) {
  int row;
  int col;
  printf("\n세계 지도\n");
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
  printf("\n%s, 레벨 %d\n", game->player.name, game->player.level);
  printf("체력 %d/%d | 경험치 %d/%d | 골드 %d | 파멸도 %d\n", game->player.hp,
         game->player.max_hp, game->player.xp, game->player.xp_to_next,
         game->player.gold, game->doom);
  printf("공격력 %d | 방어력 %d | 승리 %d\n", player_attack_value(game),
         player_defense_value(game), game->player.victories);
}
static void show_inventory(const GameState *game) {
  printf("\n인벤토리\n");
  printf("포션: %d | 폭탄: %d | 약초: %d | 광석: %d | 유물 가루: %d\n",
         game->player.potions, game->player.bombs, game->player.herbs,
         game->player.ore, game->player.relic_dust);
  printf("장비: %s, %s, %s\n",
         game->player.steel_edge ? "강철 칼날" : "여행자 검",
         game->player.ward_mail ? "수호 갑옷" : "가죽 방어구",
         game->player.abbey_sigil ? "수도원 인장" : "인장 없음");
  if (game->fragment_found[FRAGMENT_TIDAL] || game->fragment_found[FRAGMENT_FROST] ||
      game->fragment_found[FRAGMENT_EMBER]) {
    printf("파편:\n");
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
    printf("핵심 아이템: 여명의 열쇠\n");
  }
}
static void show_time(const GameState *game) {
  printf("%d일차 %02d:%02d. 지금은 %s이며, 하늘은 %s입니다.\n", current_day(game),
         current_hour(game), current_minute(game), time_band(game),
         kWeatherNames[game->weather]);
}
static void show_quests(const GameState *game) {
  printf("\n퀘스트 기록\n");
  if (game->remedy_quest == QUEST_ACTIVE) {
    printf("  수녀의 치료제: 청록 수도원의 엘로웬 수녀에게 약초 3개 전달 (%d개 보유)\n",
           game->player.herbs);
  } else if (game->remedy_quest == QUEST_COMPLETE) {
    printf("  수녀의 치료제: 완료\n");
  } else {
    printf("  수녀의 치료제: 미수락\n");
  }
  if (game->caravan_quest == QUEST_ACTIVE) {
    printf("  부서진 상단: 철목 고개의 도적 영주를 처치\n");
  } else if (game->caravan_quest == QUEST_COMPLETE) {
    printf("  부서진 상단: 완료\n");
  } else {
    printf("  부서진 상단: 미수락\n");
  }
  if (game->fragments_quest == QUEST_ACTIVE) {
    printf("  여명의 파편: 침수 기록고·빙첨로·흑요 분화구에서 파편 회수\n");
    printf("    침수 기록고: %s\n",
           game->fragment_found[FRAGMENT_TIDAL] ? "회수됨" : "미회수");
    printf("    빙첨로: %s\n",
           game->fragment_found[FRAGMENT_FROST] ? "회수됨" : "미회수");
    printf("    흑요 분화구: %s\n",
           game->fragment_found[FRAGMENT_EMBER] ? "회수됨" : "미회수");
  } else if (game->fragments_quest == QUEST_COMPLETE) {
    printf("  여명의 파편: 완료\n");
  } else {
    printf("  여명의 파편: 잠김\n");
  }
  if (game->crown_quest == QUEST_ACTIVE) {
    printf("  공허의 왕관: 여명의 열쇠를 들고 폐허 대성당을 넘어 왕좌의 갈증을 끝낼 것\n");
  } else if (game->crown_quest == QUEST_COMPLETE) {
    printf("  공허의 왕관: 완료\n");
  } else {
    printf("  공허의 왕관: 잠김\n");
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
               "%s이(가) 레벨 %d에 도달했습니다. 최대 체력, 공격력, 방어력이 상승합니다.",
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
          "도적 영주", 38, 8, 4, 22, 28, 3, ENEMY_ROLE_BOSS, true, true,
          false,
          "상처투성이 약탈대장이 한쪽 팔엔 장부를, 다른 손엔 갈고리 검을 쥐고 길 위로 "
          "내려섭니다.",
          "도적 영주가 신호 호루라기를 불고 무자비한 연격으로 파고듭니다.");
      scale_enemy(game, &enemy, 3);
      return enemy;
    }
    if (roll < 50) {
      enemy = make_enemy(
          "녹빛 늑대", 22, 6, 2, 10, 8, 1, ENEMY_ROLE_SKIRMISHER, false, false,
          false,
          "녹슨 털빛 늑대가 철목 사이를 미끄러지듯 돌며 낮게 몸을 깝니다.",
          "늑대가 목을 향해 뛰어듭니다.");
    } else {
      enemy = make_enemy(
          "통행 도적", 26, 7, 3, 11, 11, 2, ENEMY_ROLE_SKIRMISHER, false,
          true, false,
          "길목 도적이 바위 위에서 내려와 톱날 창끝을 겨눕니다.",
          "도적이 찌르기 돌진으로 파고듭니다.");
    }
    break;
  case ZONE_MOONFEN:
    if (roll < 50) {
      enemy = make_enemy(
          "늪 거머리", 24, 7, 2, 12, 9, 2, ENEMY_ROLE_BRUTE, false, false,
          false,
          "수면이 부풀어 오릅니다. 거대한 거머리가 진흙에서 몸을 떼어냅니다.",
          "거머리가 젖은 무게로 짓눌러 밀고 들어옵니다.");
    } else {
      enemy = make_enemy(
          "늪 마녀", 21, 7, 2, 13, 12, 2, ENEMY_ROLE_CASTER, false, false,
          true,
          "늪 마녀가 뿌리와 갈대 위로 떠오르며 물에게 중얼거립니다.",
          "마녀가 팔다리를 마비시키는 저주를 내뱉습니다.");
    }
    break;
  case ZONE_STORMWATCH_CLIFFS:
    if (roll < 50) {
      enemy = make_enemy(
          "절벽 약탈자", 26, 8, 3, 14, 11, 2, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "약탈자가 바위턱에서 뛰어내리며 칼날을 번뜩입니다.",
          "약탈자가 바람을 타고 잔혹한 일격을 가속합니다.");
    } else {
      enemy = make_enemy(
          "질풍 약탈자", 30, 8, 4, 15, 13, 3, ENEMY_ROLE_SKIRMISHER, false,
          true, false,
          "돛천을 두른 약탈자가 폭풍 가면 아래서 웃습니다.",
          "약탈자가 당신의 방어를 걷어내고 강하게 파고듭니다.");
    }
    break;
  case ZONE_ASHEN_QUARRY:
    if (roll < 50) {
      enemy = make_enemy(
          "채석장 구울", 28, 8, 4, 15, 12, 2, ENEMY_ROLE_BRUTE, false, false,
          false,
          "광맥 틈에서 채석장 시체가 슬래그를 물고 기어 나옵니다.",
          "구울이 채굴 발톱을 내리찍습니다.");
    } else {
      enemy = make_enemy(
          "슬래그 괴수", 33, 9, 5, 17, 15, 3, ENEMY_ROLE_BRUTE, false, false,
          false,
          "슬래그 괴수가 무너진 제련로에서 용해된 눈빛으로 튀어나옵니다.",
          "괴수가 돌진하는 용광로처럼 어깨를 들이받습니다.");
    }
    break;
  case ZONE_SUNKEN_ARCHIVE:
    if (roll < 50) {
      enemy = make_enemy(
          "익사한 서기관", 30, 9, 4, 17, 16, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "깃펜이 손바닥을 관통한 익사한 서기관이 계단을 올라옵니다.",
          "서기관이 주변 물결에 얼어붙는 문양을 그립니다.");
    } else {
      enemy = make_enemy(
          "염수 속박 큐레이터", 34, 10, 5, 18, 17, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "해초와 쇠사슬을 두른 큐레이터가 무너진 서가 사이에서 떠오릅니다.",
          "큐레이터가 집중을 흐리는 목록 저주를 읊습니다.");
    }
    break;
  case ZONE_FROSTSPIRE_TRAIL:
    if (roll < 50) {
      enemy = make_enemy(
          "빙설 사냥개", 32, 10, 4, 18, 15, 3, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "눈보라가 터지며 빙설 사냥개가 길을 가로질러 달려듭니다.",
          "사냥개가 서리 낀 이빨로 뛰어듭니다.");
    } else {
      enemy = make_enemy(
          "서리 망령", 36, 10, 5, 19, 18, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "상단 사슬을 감은 망령이 눈보라 속에서 모습을 드러냅니다.",
          "망령이 파편 섞인 한기를 내뿜습니다.");
    }
    break;
  case ZONE_CINDER_GROVE:
    if (roll < 50) {
      enemy = make_enemy(
          "불사슴", 31, 10, 4, 18, 16, 3, ENEMY_ROLE_BRUTE, false, false,
          false,
          "불사슴이 타오르는 뿔을 낮추고 재 위에 불꽃을 튑니다.",
          "불사슴이 불길을 끌며 돌진합니다.");
    } else {
      enemy = make_enemy(
          "가시주술사", 29, 10, 4, 18, 17, 3, ENEMY_ROLE_CASTER, false, false,
          true,
          "살아 있는 가시에 휘감긴 주술사가 연기 속에서 걸어 나옵니다.",
          "주술사가 베기를 둔화시키는 가시 연기를 던집니다.");
    }
    break;
  case ZONE_OBSIDIAN_CRATER:
    if (roll < 50) {
      enemy = make_enemy(
          "유리 드레이크", 38, 11, 5, 22, 20, 4, ENEMY_ROLE_BRUTE, false, false,
          false,
          "유리 드레이크가 분화구 가장자리를 넘어오며 파편을 흩뿌립니다.",
          "드레이크가 용암 발톱으로 전진하며 내리칩니다.");
    } else {
      enemy = make_enemy(
          "불씨 광신도", 34, 11, 5, 22, 21, 4, ENEMY_ROLE_CASTER, false,
          false, true,
          "검은 유리 가면을 쓴 광신도가 증기 속에서 모습을 드러냅니다.",
          "광신도가 자세를 흔드는 화염 찬가를 뿜어냅니다.");
    }
    break;
  case ZONE_RUINED_BASILICA:
    if (roll < 50) {
      enemy = make_enemy(
          "서약의 그림자", 36, 11, 5, 22, 19, 4, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "서약의 그림자가 깨진 모자이크 위에서 검을 뽑아 듭니다.",
          "그림자가 은빛 궤적을 그리며 방어를 가릅니다.");
    } else {
      enemy = make_enemy(
          "합창 망령", 35, 11, 5, 22, 20, 4, ENEMY_ROLE_CASTER, false, false,
          true,
          "갈라진 턱으로 노래하는 합창 망령이 중앙 회랑에서 내려옵니다.",
          "망령의 음이 집중을 곧게 베어냅니다.");
    }
    break;
  default:
    enemy = make_enemy(
        "길목 잠복자", 20, 6, 2, 8, 7, 1, ENEMY_ROLE_SKIRMISHER, false, false,
        false, "길가 그림자에서 수색자가 모습을 드러냅니다.",
        "잠복자가 빠른 베기로 파고듭니다.");
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
        "익사한 큐레이터", 44, 12, 6, 28, 28, 4, ENEMY_ROLE_BOSS, true, false,
        true,
        "큐레이터 금고 문이 신음하며 열리고, 황동 열쇠 사슬을 양팔에 감은 거대한 "
        "익사한 수호자가 일어섭니다.",
        "큐레이터가 방 전체에 차가운 기록 저주를 퍼뜨립니다.");
    break;
  case FRAGMENT_FROST:
    enemy = make_enemy(
        "서리 거상", 48, 13, 7, 30, 30, 4, ENEMY_ROLE_BOSS, true, false,
        false,
        "얼음과 부서진 상단 목재로 이루어진 거대한 기사가 산 사당에서 몸을 찢고 나옵니다.",
        "거상이 양팔을 들어 산 전체를 무너뜨릴 듯 내리칩니다.");
    break;
  case FRAGMENT_EMBER:
    enemy = make_enemy(
        "유리 웜", 52, 14, 7, 34, 34, 5, ENEMY_ROLE_BOSS, true, false, true,
        "용암이 분화구 바닥을 가르고, 타오르는 목 안에 불씨 파편을 품은 검은 유리 웜이 "
        "허공으로 솟아오릅니다.",
        "웜이 비명을 지르자 분화구가 열파로 응답합니다.");
    break;
  default:
    enemy = make_enemy(
        "유물 수호자", 40, 11, 5, 24, 24, 3, ENEMY_ROLE_BOSS, true, false,
        false, "유물 수호자가 은신처에서 걸어 나옵니다.",
        "수호자가 돌발적으로 강습합니다.");
    break;
  }
  scale_enemy(game, &enemy, 4);
  return enemy;
}
static Enemy build_final_boss(GameState *game) {
  Enemy boss =
      make_enemy("여명 없는 왕", 78, 16, 8, 80, 0, 6, ENEMY_ROLE_BOSS,
                 true, false, true,
                 "왕좌 위의 그림자가 일어섭니다. 왕의 형상을 한 공허가 갑옷과 뿔, "
                 "그리고 끝내 구하지 못한 모든 이로 만든 왕관으로 펼쳐집니다.",
                 "죽은 왕이 공허의 왕관을 들어 올리자 방 전체가 당신 쪽으로 기울어집니다.");
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
      printf("전투의 여파 속에서 쓸 만한 약초 묶음을 찾아냈습니다.\n");
    }
  } else if (zone == ZONE_IRONWOOD_PASS || zone == ZONE_ASHEN_QUARRY ||
             zone == ZONE_OBSIDIAN_CRATER) {
    if (rand() % 100 < 35) {
      game->player.ore++;
      printf("전장에서 가공 가능한 광석 덩이를 떼어냈습니다.\n");
    }
  } else if (zone == ZONE_SUNKEN_ARCHIVE || zone == ZONE_RUINED_BASILICA) {
    if (rand() % 100 < 25) {
      game->player.relic_dust++;
      printf("폐허에서 반짝이는 유물 가루를 모았습니다.\n");
    }
  }
  if (game->weather == WEATHER_CLEAR) {
    bonus_gold++;
  }
  if (bonus_gold > 0) {
    game->player.gold += bonus_gold;
  }
  printf("보상: 경험치 %d, 골드 %d.\n", enemy->xp_reward,
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
    if (!read_command("전투> ", input, sizeof(input))) {
      game->running = false;
      game->combat.active = false;
      return BATTLE_RESULT_DEFEAT;
    }
    canonicalize_input(input, command, sizeof(command));
    if (command[0] == '\0') {
      continue;
    }
    if (strcmp(command, "help") == 0) {
      printf("전투 명령어: attack, cleave, guard, potion, bomb, flee, status\n");
      continue;
    }
    if (strcmp(command, "status") == 0) {
      printf("현재 상태: %s\n",
             game->combat.weaken_turns > 0 ? "적 마법으로 약화됨" : "안정적");
      continue;
    }
    if (strcmp(command, "potion") == 0 ||
        strcmp(command, "use potion") == 0) {
      if (game->player.potions <= 0) {
        printf("포션이 없습니다.\n");
        continue;
      }
      game->player.potions--;
      game->player.hp =
          clamp_int(game->player.hp + 18 + game->player.level * 2, 0,
                    game->player.max_hp);
      printf("포션을 마시고 자세를 가다듬습니다.\n");
      spend_turn = true;
    } else if (strcmp(command, "bomb") == 0) {
      if (game->player.bombs <= 0) {
        printf("폭탄이 남아 있지 않습니다.\n");
        continue;
      }
      game->player.bombs--;
      player_damage = 14 + game->player.level * 3;
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("폭탄이 폭발해 %d 피해를 입혔습니다.\n", player_damage);
      spend_turn = true;
    } else if (strcmp(command, "guard") == 0) {
      game->combat.guard_active = true;
      if (game->player.abbey_sigil) {
        game->player.hp =
            clamp_int(game->player.hp + 2, 0, game->player.max_hp);
        printf("무기를 세워 자세를 잡자 수도원 인장이 빛납니다.\n");
      } else {
        printf("다음 공격에 대비해 방어 태세를 취합니다.\n");
      }
      spend_turn = true;
    } else if (strcmp(command, "cleave") == 0) {
      int attack = player_attack_value(game) + 6;
      if (game->player.level < 3) {
        printf("아직 전력 가르기를 익히지 못했습니다.\n");
        continue;
      }
      if (game->combat.weaken_turns > 0) {
        attack -= 3;
      }
      player_damage = compute_damage(attack, game->combat.enemy.defense);
      if (rand() % 100 < 20) {
        player_damage += 4 + game->player.level;
        printf("가르기가 완벽하게 적중했습니다.\n");
      }
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("%s에게 가르기 피해 %d를 입혔습니다.\n", game->combat.enemy.name,
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
        printf("치명타!\n");
      }
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("%s에게 %d 피해를 입혔습니다.\n", game->combat.enemy.name,
             player_damage);
      spend_turn = true;
    } else if (strcmp(command, "flee") == 0) {
      int flee_chance;
      if (game->combat.enemy.boss) {
        printf("이 전투에서는 도망칠 수 없습니다.\n");
        continue;
      }
      flee_chance = 45 + game->player.level * 5 - kZones[game->player.zone].danger * 3;
      if (game->weather == WEATHER_FOG) {
        flee_chance += 10;
      }
      if (rand() % 100 < flee_chance) {
        printf("틈을 만들어 탈출했습니다.\n");
        advance_time(game, 10);
        flush_events(game);
        game->combat.active = false;
        return BATTLE_RESULT_FLED;
      }
      printf("이탈을 시도했지만 %s이(가) 길을 막습니다.\n",
             game->combat.enemy.name);
      spend_turn = true;
    } else {
      printf("그 명령어는 전투 중 사용할 수 없습니다.\n");
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
      printf("%s이(가) 강타 연계를 준비합니다.\n",
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
      printf("%s이(가) 방어를 뚫고 골드 %d를 훔쳤습니다.\n",
             game->combat.enemy.name, stolen);
    } else {
      enemy_damage =
          compute_damage(game->combat.enemy.attack, player_defense_value(game));
      if (game->combat.enemy.role == ENEMY_ROLE_BRUTE) {
        printf("%s이(가) 압도적인 힘으로 들이받습니다.\n", game->combat.enemy.name);
      } else if (game->combat.enemy.role == ENEMY_ROLE_CASTER) {
        printf("%s이(가) 악의적인 마법을 퍼붓습니다.\n", game->combat.enemy.name);
      } else {
        printf("%s이(가) 재빠르게 공격합니다.\n", game->combat.enemy.name);
      }
    }
    if (game->combat.guard_active) {
      enemy_damage = (enemy_damage + 1) / 2;
      printf("방어로 피해 일부를 흡수했습니다.\n");
    }
    if (enemy_damage > 0) {
      game->player.hp = clamp_int(game->player.hp - enemy_damage, 0,
                                  game->player.max_hp);
      printf("%d 피해를 받았습니다.\n", enemy_damage);
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
    printf("\n%s이(가) 쓰러지고 길은 어둠에 잠깁니다.\n", game->player.name);
    game->running = false;
    return BATTLE_RESULT_DEFEAT;
  }
  printf("\n%s이(가) 쓰러졌습니다.\n", game->combat.enemy.name);
  award_post_battle_loot(game, &game->combat.enemy, game->player.zone);
  return BATTLE_RESULT_VICTORY;
}
static void use_potion_outside_combat(GameState *game) {
  if (game->player.potions <= 0) {
    printf("포션이 없습니다.\n");
    return;
  }
  if (game->player.hp >= game->player.max_hp) {
    printf("이미 체력이 가득합니다.\n");
    return;
  }
  game->player.potions--;
  game->player.hp = clamp_int(game->player.hp + 18 + game->player.level * 2, 0,
                              game->player.max_hp);
  printf("포션을 마셔 체력을 %d/%d로 회복했습니다.\n", game->player.hp,
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
               "회수한 장부에서 남부의 숨겨진 광석 저장 위치를 확인했습니다.");
  }
  refresh_rumor(game);
  printf("부서진 상단 장부를 회수하고 고개의 도적 무리를 흩어놓았습니다. 상인들이 한숨 "
         "돌립니다.\n");
}
static void handle_fragment_victory(GameState *game, FragmentId fragment) {
  game->fragment_found[fragment] = true;
  game->player.relic_dust += 2;
  game->doom = clamp_int(game->doom - 1, 0, 12);
  refresh_rumor(game);
  printf("%s을(를) 확보했습니다.\n", kFragmentNames[fragment]);
}
static void hunt_current_zone(GameState *game) {
  Enemy enemy;
  BattleResult result;
  if (kZones[game->player.zone].safe || kZones[game->player.zone].danger <= 0) {
    printf("이 구역은 너무 안전해 사냥 대상을 찾기 어렵습니다.\n");
    return;
  }
  advance_time(game, 20);
  flush_events(game);
  enemy = build_regular_enemy(game, game->player.zone);
  result = run_battle(game, enemy);
  if (result != BATTLE_RESULT_VICTORY) {
    return;
  }
  if (strcmp(enemy.name, "도적 영주") == 0) {
    handle_bandit_boss_victory(game);
  }
}
static void scout_zone(GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  printf("%s\n", zone->scout_text);
  if (!zone->safe && zone->danger > 0) {
    printf("체감 위험도는 대략 %d 정도입니다.\n",
           zone->danger + game->doom / 3);
  }
  if (zone_has_merchant(game, game->player.zone)) {
    printf("이곳에서는 거래가 가능합니다.\n");
  }
  if (game->player.zone == ZONE_HOLLOW_THRONE && game->dawn_key_forged) {
    printf("왕좌가 깨어 있습니다. 무혈 입장은 불가능합니다.\n");
  }
  advance_time(game, 10);
  flush_events(game);
}
static void gather_resources(GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  int amount = 1;
  if (zone->resource == RESOURCE_NONE) {
    printf("이곳에서는 채집할 만한 것이 보이지 않습니다.\n");
    return;
  }
  if (game->weather == WEATHER_CLEAR || game->weather == WEATHER_RAIN) {
    amount++;
  }
  if (zone->resource == RESOURCE_HERB) {
    game->player.herbs += amount;
    printf("약초 묶음 %d개를 채집했습니다.\n", amount);
  } else if (zone->resource == RESOURCE_ORE) {
    game->player.ore += amount;
    printf("광석 덩이 %d개를 채굴했습니다.\n", amount);
  }
  advance_time(game, 35);
  flush_events(game);
  if (!zone->safe && rand() % 100 < 30) {
    printf("채집 소음이 불청객의 관심을 끌었습니다.\n");
    hunt_current_zone(game);
  }
}
static void explore_special_location(GameState *game) {
  BattleResult result;
  switch (game->player.zone) {
  case ZONE_SUNKEN_ARCHIVE:
    if (game->fragments_quest != QUEST_ACTIVE) {
      printf("기록고를 뒤졌지만 기록관의 메모 없이는 심층 금고를 해독할 수 없습니다.\n");
      advance_time(game, 25);
      flush_events(game);
      return;
    }
    if (game->fragment_found[FRAGMENT_TIDAL]) {
      printf("큐레이터 금고는 이미 열려 있고 비어 있습니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("무너진 서가 아래로 잠수해 큐레이터 금고를 강제로 엽니다.\n");
    advance_time(game, 25);
    flush_events(game);
    result = run_battle(game, build_fragment_guardian(game, FRAGMENT_TIDAL));
    if (result == BATTLE_RESULT_VICTORY) {
      handle_fragment_victory(game, FRAGMENT_TIDAL);
    }
    return;
  case ZONE_FROSTSPIRE_TRAIL:
    if (game->fragments_quest != QUEST_ACTIVE) {
      printf("도서관 경로 메모가 없으면 폭풍 속 얼음 사당은 전부 똑같이 보입니다.\n");
      advance_time(game, 25);
      flush_events(game);
      return;
    }
    if (game->fragment_found[FRAGMENT_FROST]) {
      printf("얼어붙은 사당에서는 이미 유물이 회수되었습니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("반쯤 묻힌 행렬 길을 따라 얼어붙은 사당으로 향합니다.\n");
    advance_time(game, 25);
    flush_events(game);
    result = run_battle(game, build_fragment_guardian(game, FRAGMENT_FROST));
    if (result == BATTLE_RESULT_VICTORY) {
      handle_fragment_victory(game, FRAGMENT_FROST);
    }
    return;
  case ZONE_OBSIDIAN_CRATER:
    if (game->fragments_quest != QUEST_ACTIVE) {
      printf("분화구 분기공이 안전 하강 전에 당신을 밀어냅니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    if (game->fragment_found[FRAGMENT_EMBER]) {
      printf("웜이 둥지를 틀었던 방은 이미 고요합니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("분화구의 용해된 심장부로 하강합니다.\n");
    advance_time(game, 25);
    flush_events(game);
    result = run_battle(game, build_fragment_guardian(game, FRAGMENT_EMBER));
    if (result == BATTLE_RESULT_VICTORY) {
      handle_fragment_victory(game, FRAGMENT_EMBER);
    }
    return;
  case ZONE_RUINED_BASILICA:
    if (!game->dawn_key_forged) {
      printf("대성당이 당신을 거부합니다. 모든 통로가 부서진 문으로 되돌아가는 듯합니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    if (game->basilica_blessing) {
      printf("이미 대성당의 마지막 축복을 받았습니다.\n");
      advance_time(game, 15);
      flush_events(game);
      return;
    }
    printf("무너진 돔 아래 무릎 꿇자 여명의 열쇠가 먼지 사이로 빛납니다.\n");
    game->basilica_blessing = true;
    game->doom = clamp_int(game->doom - 1, 0, 12);
    advance_time(game, 30);
    flush_events(game);
    printf("희미한 결계가 갑옷 위에 내려앉습니다. 왕좌의 끌림이 약해집니다.\n");
    return;
  case ZONE_HOLLOW_THRONE:
    if (!game->dawn_key_forged) {
      printf("봉인된 궁전 문이 접근을 거부합니다.\n");
      return;
    }
    printf("왕좌의 방에 발을 들이자 왕관이 당신을 향합니다.\n");
    result = run_battle(game, build_final_boss(game));
    if (result == BATTLE_RESULT_VICTORY) {
      game->final_boss_defeated = true;
      game->crown_quest = QUEST_COMPLETE;
      game->running = false;
      printf("\n여명의 열쇠가 부서지고 공허의 왕관이 산산이 깨지며, 묻힌 궁전이 마침내 "
             "긴 숨을 내쉽니다.\n");
    }
    return;
  default:
    printf("세밀하게 수색해 오래된 지식 조각과 동전을 찾았지만 급한 단서는 없습니다.\n");
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
      printf("병참장교 아이븐이 추가 보급품을 쥐여주며 값싸게 죽지 말라고 경고합니다.\n");
    } else {
      printf("아이븐이 길을 바라보며 말합니다. \"남부는 한 시간만 지나도 표정이 바뀐다. "
             "날씨와 소문보다 먼저 움직여라.\"\n");
    }
    break;
  case ZONE_BRASS_MARKET:
    if (game->caravan_quest == QUEST_LOCKED) {
      game->caravan_quest = QUEST_ACTIVE;
      refresh_rumor(game);
      printf("상인 살이 부서진 장부를 내려칩니다. \"철목 고개에서 도적 영주가 내 마지막 "
             "상단을 털었어. 장부를 찾아오면 돈 이상으로 갚겠어.\"\n");
    } else if (game->caravan_quest == QUEST_ACTIVE) {
      printf("\"철목 고개.\" 살이 되뇝니다. \"크게 흔들면 그놈은 오래 숨어 있지 못해.\"\n");
    } else {
      printf("살이 웃습니다. \"고개가 다시 숨 쉬기 시작했어. 다른 지역도 그랬으면 좋겠군.\"\n");
    }
    break;
  case ZONE_VERDANT_ABBEY:
    if (game->remedy_quest == QUEST_LOCKED) {
      game->remedy_quest = QUEST_ACTIVE;
      printf("엘로웬 수녀가 빈 주머니 세 개를 놓습니다. \"월광 늪이나 숯불 수림에서 "
             "신선한 약초 묶음 3개를 가져와 주세요. 없으면 부상자들이 이번 주를 못 넘겨요.\"\n");
    } else if (game->remedy_quest == QUEST_ACTIVE && game->player.herbs >= 3) {
      game->player.herbs -= 3;
      game->remedy_quest = QUEST_COMPLETE;
      game->player.abbey_sigil = true;
      game->player.max_hp += 4;
      game->player.hp = game->player.max_hp;
      game->player.potions += 2;
      printf("엘로웬이 밤새 약을 달인 뒤 은빛 인장을 목에 걸어 줍니다. \"우리의 축복을 "
             "어둠 속으로 가져가세요.\"\n");
    } else if (game->remedy_quest == QUEST_ACTIVE) {
      printf("\"약초 묶음이 아직 3개 필요해요.\" 엘로웬이 조용히 말합니다. \"지금 %d개예요.\"\n",
             game->player.herbs);
    } else {
      game->player.hp = game->player.max_hp;
      printf("수녀들이 상처를 치료해 주고 다시 길로 돌려보냅니다.\n");
    }
    break;
  case ZONE_WHISPER_LIBRARY:
    if (game->fragments_quest == QUEST_LOCKED) {
      if (game->player.level < 3 && game->caravan_quest != QUEST_COMPLETE) {
        printf("기록관 센이 지도 더미 너머로 당신을 살핍니다. \"아직 파편 사냥을 맡기엔 "
               "이릅니다. 남부를 더 보고 오세요.\"\n");
      } else {
        game->fragments_quest = QUEST_ACTIVE;
        refresh_rumor(game);
        printf("센이 바스라지는 지도 세 장을 펼칩니다. \"여명의 열쇠는 산산조각 났습니다. "
               "침수 기록고, 빙첨로, 흑요 분화구에서 파편을 회수해 오면 왕좌로 향할 길을 "
               "열 수 있습니다.\"\n");
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
      printf("센이 회수된 파편을 묶어 여명의 열쇠를 완성합니다. 균열 사이로 새벽빛 같은 "
             "광채가 흐릅니다. \"이제 공허 왕좌에 닿을 수 있습니다.\"\n");
    } else if (game->dawn_key_forged) {
      printf("\"대성당은 열쇠에 응답할 겁니다.\" 센이 말합니다. \"넘어가기 전 가능한 "
             "모든 이점을 챙기세요.\"\n");
    } else {
      printf("센이 지도를 하나씩 두드립니다. \"기록고. 빙첨로. 분화구. 왕관이 부순 것을 "
             "되찾아 오세요.\"\n");
    }
    break;
  case ZONE_GLOAM_PORT:
    if (game->dawn_key_forged) {
      printf("미렐 선장이 내륙을 가리킵니다. \"대성당을 넘으면 멈추지 마세요. 왕좌는 "
             "망설임을 먹고 자랍니다.\"\n");
    } else {
      printf("미렐이 오래된 항로도 조수 흔적을 보며 말합니다. \"쓸모 있는 건 묻혀 있지 "
             "않고 숨겨져 있을 뿐이죠. 동쪽과 남쪽으로 계속 밀고 가세요.\"\n");
    }
    break;
  case ZONE_LANTERN_WARD:
    printf("전령 나라가 순찰 기록과 구역 바깥 도로의 최신 경고를 전합니다.\n");
    break;
  default:
    printf("이곳에는 대화할 상대가 없습니다.\n");
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
    printf("지금 이곳에는 거래할 상인이 없습니다.\n");
    return;
  }
  if (game->player.zone == ZONE_GLOAM_PORT) {
    stock = &game->port_potions;
    port_prices = true;
  } else if (game->player.zone == ZONE_BRASS_MARKET) {
    stock = &game->market_potions;
  }
  printf("`buy potion`, `buy bomb`, `leave` 중 하나를 입력하세요.\n");
  while (game->running) {
    int price = potion_price(game, port_prices);
    int bomb_price = port_prices ? 18 : 20;
    printf("재고: 포션 %d | 포션 %dg | 폭탄 %dg\n",
           stock != NULL ? *stock : 2, price, bomb_price);
    if (!read_command("상점> ", input, sizeof(input))) {
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
        printf("골드가 부족합니다.\n");
        continue;
      }
      if (stock != NULL && *stock <= 0) {
        printf("해당 상인의 재고가 소진되었습니다.\n");
        continue;
      }
      game->player.gold -= price;
      game->player.potions++;
      if (stock != NULL) {
        (*stock)--;
      }
      printf("포션을 구매했습니다.\n");
      advance_time(game, 10);
      flush_events(game);
      continue;
    }
    if (strcmp(command, "buy bomb") == 0) {
      if (game->player.gold < bomb_price) {
        printf("골드가 부족합니다.\n");
        continue;
      }
      game->player.gold -= bomb_price;
      game->player.bombs++;
      printf("폭탄을 구매했습니다.\n");
      advance_time(game, 10);
      flush_events(game);
      continue;
    }
    printf("상인이 그 요청을 이해하지 못했습니다.\n");
  }
}
static void forge_here(GameState *game) {
  char input[MAX_INPUT];
  char command[MAX_INPUT];
  if (!kZones[game->player.zone].forge) {
    printf("이곳에는 작동 가능한 대장간이 없습니다.\n");
    return;
  }
  printf("`craft blade`, `craft mail`, `craft bomb`, `leave` 중 하나를 입력하세요.\n");
  while (game->running) {
    printf("광석 %d | 검 4개 | 갑옷 6개 | 폭탄 2개\n", game->player.ore);
    if (!read_command("대장간> ", input, sizeof(input))) {
      game->running = false;
      return;
    }
    canonicalize_input(input, command, sizeof(command));
    if (strcmp(command, "leave") == 0 || strcmp(command, "exit") == 0) {
      return;
    }
    if (strcmp(command, "craft blade") == 0) {
      if (game->player.steel_edge) {
        printf("이미 검을 재단련했습니다.\n");
        continue;
      }
      if (game->player.ore < 4) {
        printf("광석 4개가 필요합니다.\n");
        continue;
      }
      game->player.ore -= 4;
      game->player.steel_edge = true;
      printf("무기를 재단련해 강철 칼날로 만들었습니다.\n");
      advance_time(game, 45);
      flush_events(game);
      continue;
    }
    if (strcmp(command, "craft mail") == 0) {
      if (game->player.ward_mail) {
        printf("이미 갑옷을 강화했습니다.\n");
        continue;
      }
      if (game->player.ore < 6) {
        printf("광석 6개가 필요합니다.\n");
        continue;
      }
      game->player.ore -= 6;
      game->player.ward_mail = true;
      printf("새 철판을 단련해 수호 갑옷 세트를 완성했습니다.\n");
      advance_time(game, 55);
      flush_events(game);
      continue;
    }
    if (strcmp(command, "craft bomb") == 0) {
      if (game->player.ore < 2) {
        printf("광석 2개가 필요합니다.\n");
        continue;
      }
      game->player.ore -= 2;
      game->player.bombs++;
      printf("거칠지만 실용적인 폭탄을 제작했습니다.\n");
      advance_time(game, 20);
      flush_events(game);
      continue;
    }
    printf("대장간이 그 명령에는 반응하지 않습니다.\n");
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
    printf("이곳은 안전하게 휴식할 수 있는 장소가 아닙니다.\n");
    return;
  }
  if (game->player.gold < cost) {
    printf("숙소를 이용할 골드가 부족합니다.\n");
    return;
  }
  game->player.gold -= cost;
  advance_time(game, 480);
  game->player.hp = game->player.max_hp;
  printf("휴식으로 기력을 회복했습니다. 체력이 %d로 회복되었습니다.\n",
         game->player.hp);
  flush_events(game);
}
static bool move_player(GameState *game, const char *direction) {
  int next_zone = zone_from_direction(game->player.zone, direction);
  if (next_zone == ZONE_NONE) {
    printf("그 길은 막혀 있습니다.\n");
    return false;
  }
  if (next_zone == ZONE_HOLLOW_THRONE && !game->dawn_key_forged) {
    printf("봉인된 압력이 발길을 돌립니다. 돌보다 깊은 무언가가 아직 궁전을 가로막고 "
           "있습니다.\n");
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
  printf("Feather RPG: 공허의 왕관의 재\n");
  printf("Feather 협력형 스케줄러로 구동되는 텍스트 RPG입니다.\n");
  printf("시간이 흐르는 동안 세계는 계속 움직입니다: 날씨 변화, 상단 이동, "
         "소문 확산, 파멸도 상승, 그리고 길 위의 회복/위협.\n");
}
int rpg_run(void) {
  GameState game;
  char input[MAX_INPUT];
  char command[MAX_INPUT];
  srand((unsigned int)time(NULL));
  init_game(&game);
  print_intro();
  if (read_command("수호자의 이름을 입력하세요 (빈칸이면 수호자): ", input,
                   sizeof(input)) &&
      !is_blank(input)) {
    snprintf(game.player.name, sizeof(game.player.name), "%s", input);
  }
  printf("\n%s이(가) 남부 순찰대의 불빛이 희미해지는 저녁, 엠버폴 관문에 도착했습니다.\n",
         game.player.name);
  describe_zone(&game);
  show_help();
  while (game.running) {
    if (!read_command("\n명령> ", input, sizeof(input))) {
      break;
    }
    canonicalize_input(input, command, sizeof(command));
    if (command[0] == '\0') {
      continue;
    }
    if (maybe_handle_movement_command(&game, command)) {
      continue;
    }
    if (strcmp(command, "help") == 0 || strcmp(command, "도움말") == 0) {
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
    } else if (strcmp(command, "save") == 0 || strcmp(command, "저장") == 0) {
      if (save_game(&game, SAVE_FILE_PATH)) {
        printf("게임이 %s 파일에 저장되었습니다.\n", SAVE_FILE_PATH);
      } else {
        printf("저장에 실패했습니다.\n");
      }
    } else if (strcmp(command, "load") == 0 || strcmp(command, "불러오기") == 0) {
      if (load_game(&game, SAVE_FILE_PATH)) {
        printf("%s 파일에서 게임을 불러왔습니다.\n", SAVE_FILE_PATH);
        describe_zone(&game);
      } else {
        printf("불러오기에 실패했습니다. 저장 파일이 없거나 형식이 올바르지 않습니다.\n");
      }
    } else if (strcmp(command, "quit") == 0 ||
               strcmp(command, "exit") == 0 || strcmp(command, "종료") == 0) {
      printf("가장 가까운 불빛 쪽으로 물러나 오늘의 원정을 마칩니다.\n");
      break;
    } else {
      printf("알 수 없는 명령어입니다. `help`로 목록을 확인하세요.\n");
    }
  }
  if (game.final_boss_defeated) {
    printf("\n승리했습니다. 길은 여전히 수호자를 필요로 하지만, 더 이상 공허의 왕관에 "
           "지배당하지 않습니다.\n");
  } else if (!game.running && game.player.hp <= 0) {
    printf("게임 오버.\n");
  }
  shutdown_game(&game);
  return 0;
}
