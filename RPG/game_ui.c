#define _POSIX_C_SOURCE 200809L
#include "game_shared.h"

void describe_zone(const GameState *game) {
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
void show_help(void) {
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
void show_map(const GameState *game) {
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
void show_stats(const GameState *game) {
  printf("\n%s, 레벨 %d\n", game->player.name, game->player.level);
  printf("체력 %d/%d | 경험치 %d/%d | 골드 %d | 파멸도 %d\n", game->player.hp,
         game->player.max_hp, game->player.xp, game->player.xp_to_next,
         game->player.gold, game->doom);
  printf("공격력 %d | 방어력 %d | 승리 %d\n", player_attack_value(game),
         player_defense_value(game), game->player.victories);
}
void show_inventory(const GameState *game) {
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
void show_time(const GameState *game) {
  printf("%d일차 %02d:%02d. 지금은 %s이며, 하늘은 %s입니다.\n", current_day(game),
         current_hour(game), current_minute(game), time_band(game),
         kWeatherNames[game->weather]);
}
void show_quests(const GameState *game) {
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
bool read_command(const char *prompt, char *buffer, size_t buffer_size) {
  printf("%s", prompt);
  fflush(stdout);
  if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
    return false;
  }
  buffer[strcspn(buffer, "\n")] = '\0';
  return true;
}
