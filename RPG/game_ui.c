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
void show_help(const GameState *game) {
  printf("\n명령어:\n");
  printf("  look [둘러보기], map [지도], stats [능력치], inventory [소지품]\n");
  printf("  quests [퀘스트], rumor [소문], time [시간]\n");
  printf("  north/south/east/west [이동], go [방향], travel [방향]\n");
  printf("  scout [정찰], hunt [사냥], gather [채집], explore [탐사]\n");
  printf("  talk [대화], shop [상점], forge [대장간], rest [휴식]\n");
  printf("  use potion [포션], use holy water [성수], save [저장], load [불러오기], quit [종료]\n");
  printf("\n전투 명령어 (공용):\n");
  printf("  attack [공격], cleave(Lv3) [가르기], devastate(Lv6) [궁극 강타]\n");
  printf("  guard [방어], potion [포션], use holy water [성수], bomb [폭탄]\n");
  printf("  flee [도주], status [상태]\n");
  if (game != NULL) {
    if (game->player.player_class == CLASS_WARRIOR) {
      printf("\n전사 전용:\n");
      printf("  parry [막기/반격], bash(Lv5) [방패 강타/기절]\n");
    } else if (game->player.player_class == CLASS_SCOUT) {
      printf("\n척후 전용:\n");
      printf("  backstab [기습], vanish(Lv4) [은신 이탈]\n");
    } else if (game->player.player_class == CLASS_MAGE) {
      printf("\n마법사 전용 (룬 파편 소모):\n");
      printf("  fireball [화염구], frost(Lv3) [냉기 화살/동결]\n");
    } else if (game->player.player_class == CLASS_CLERIC) {
      printf("\n성직자 전용:\n");
      printf("  smite [신성 피해+자가치유], holy_barrier(Lv4) [공격차단+치유]\n");
    }
  }
}
void show_map(const GameState *game) {
  int row;
  int col;
  printf("\n세계 지도\n");
  for (row = 0; row < 6; row++) {
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
    if (row < 5) {
      printf("   |        |        |        |\n");
    }
  }
}
void show_stats(const GameState *game) {
  printf("\n%s (%s), 레벨 %d\n", game->player.name,
         class_name(game->player.player_class), game->player.level);
  printf("체력 %d/%d | 경험치 %d/%d | 골드 %d | 파멸도 %d\n", game->player.hp,
         game->player.max_hp, game->player.xp, game->player.xp_to_next,
         game->player.gold, game->doom);
  printf("공격력 %d | 방어력 %d | 승리 %d\n", player_attack_value(game),
         player_defense_value(game), game->player.victories);
  if (game->player.player_class == CLASS_MAGE) {
    printf("룬 파편: %d개\n", game->player.rune_shards);
  }
  if (game->player.holy_water > 0 || game->player.player_class == CLASS_CLERIC) {
    printf("성수: %d개\n", game->player.holy_water);
  }
  if (game->beacon_lit) {
    printf("고대 봉화: 점등됨 (파멸도 억제 효과 활성)\n");
  }
  if (game->player.titan_blade) {
    printf("타이탄 검 장착 중 (공격력 +7)\n");
  }
}
void show_inventory(const GameState *game) {
  printf("\n인벤토리\n");
  printf("포션: %d | 폭탄: %d | 성수: %d | 약초: %d | 광석: %d | 유물 가루: %d",
         game->player.potions, game->player.bombs, game->player.holy_water,
         game->player.herbs, game->player.ore, game->player.relic_dust);
  if (game->player.player_class == CLASS_MAGE || game->player.rune_shards > 0) {
    printf(" | 룬 파편: %d", game->player.rune_shards);
  }
  printf("\n");
  printf("장비: %s, %s, %s",
         game->player.titan_blade ? "타이탄 검" :
             (game->player.steel_edge ? "강철 칼날" : "여행자 검"),
         game->player.ward_mail ? "수호 갑옷" : "가죽 방어구",
         game->player.abbey_sigil ? "수도원 인장" : "인장 없음");
  if (game->player.spirit_totem) {
    printf(", 영혼의 토템");
  }
  printf("\n");
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
  /* New quests */
  if (game->beacon_quest == QUEST_ACTIVE) {
    printf("  고대 봉화 점등: 고대 봉화(탐사)에 광석 2개·약초 1개를 사용해 점화\n");
  } else if (game->beacon_quest == QUEST_COMPLETE) {
    printf("  고대 봉화 점등: 완료\n");
  } else {
    printf("  고대 봉화 점등: 미수락 (고대 봉화의 봉화지기 오른에게 대화)\n");
  }
  if (game->druid_quest == QUEST_ACTIVE) {
    printf("  드루이드의 의식: 심숲 분지의 에이브에게 약초 4개 제공 (%d개 보유)\n",
           game->player.herbs);
  } else if (game->druid_quest == QUEST_COMPLETE) {
    printf("  드루이드의 의식: 완료 (영혼의 토템 보유)\n");
  } else {
    printf("  드루이드의 의식: 미수락 (심숲 분지 에이브에게 대화)\n");
  }
  if (game->vault_quest == QUEST_COMPLETE) {
    printf("  파쇄 금고 탐사: 완료\n");
  } else if (game->final_boss_defeated) {
    printf("  파쇄 금고 탐사: 공허 왕좌 남쪽 파쇄 금고에서 탐사 가능\n");
  } else {
    printf("  파쇄 금고 탐사: 잠김 (여명 없는 왕 격파 후 해제)\n");
  }
  if (game->shore_quest == QUEST_ACTIVE) {
    printf("  해안 정화: 메아리 해안(탐사)에 약초 3개·유물 가루 1개 사용 "
           "(약초 %d개, 유물 가루 %d개 보유)\n",
           game->player.herbs, game->player.relic_dust);
  } else if (game->shore_quest == QUEST_COMPLETE) {
    printf("  해안 정화: 완료\n");
  } else {
    printf("  해안 정화: 미수락 (메아리 해안의 은자 레나에게 대화)\n");
  }
  if (game->citadel_quest == QUEST_ACTIVE) {
    printf("  성채 해방: 부서진 성채(탐사)에서 성채의 마지막 군주를 격파할 것\n");
  } else if (game->citadel_quest == QUEST_COMPLETE) {
    printf("  성채 해방: 완료\n");
  } else {
    printf("  성채 해방: 미수락 (빛의 첨탑의 사서 에반에게 대화)\n");
  }
}
bool read_command(const char *prompt, char *buffer, size_t buffer_size) {
  /* Legacy blocking readline — no longer used by the real-time main loop.
   * Combat uses its own ncurses input routine (see game_combat.c).
   * Retained to satisfy the declaration in game_shared.h. */
  (void)prompt;
  (void)buffer;
  (void)buffer_size;
  return false;
}
