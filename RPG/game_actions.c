#define _POSIX_C_SOURCE 200809L
#include "game_shared.h"

void use_potion_outside_combat(GameState *game) {
  if (game->player.potions <= 0) {
    printf("포션이 없습니다.\n");
    return;
  }
  if (game->player.hp >= game->player.max_hp) {
    printf("이미 체력이 가득합니다.\n");
    return;
  }
  {
    int heal = 18 + game->player.level * 2;
    if (game->player.player_class == CLASS_CLERIC) {
      heal += 8;
    }
    if (game->weather == WEATHER_RAIN) {
      heal += 5;
    }
    game->player.potions--;
    game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
    printf("포션을 마셔 체력을 %d/%d로 회복했습니다.\n", game->player.hp,
           game->player.max_hp);
  }
  advance_time(game, 10);
  flush_events(game);
}
void use_holy_water_outside_combat(GameState *game) {
  if (game->player.holy_water <= 0) {
    printf("성수가 없습니다.\n");
    return;
  }
  {
    int heal = 30;
    if (game->player.player_class == CLASS_CLERIC) {
      heal += 10;
    }
    if (game->weather == WEATHER_RAIN) {
      heal += 5;
    }
    game->player.holy_water--;
    game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
    printf("성수를 마셔 체력을 %d/%d로 회복하고 상태이상을 모두 해제했습니다.\n",
           game->player.hp, game->player.max_hp);
  }
  advance_time(game, 10);
  flush_events(game);
}
void use_relic_dust_outside_combat(GameState *game) {
  int heal;
  if (game->player.relic_dust <= 0) {
    printf("유물 가루가 없습니다.\n");
    return;
  }
  game->player.relic_dust--;
  heal = 8 + game->player.level * 2;
  game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
  if (game->doom > 0) {
    game->doom = clamp_int(game->doom - 1, 0, 12);
    printf("유물 가루를 정화 의식에 사용했습니다. 체력 +%d, 파멸도 -1.\n", heal);
  } else {
    printf("유물 가루를 사용해 체력을 +%d 회복했습니다.\n", heal);
  }
  advance_time(game, 8);
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
void hunt_current_zone(GameState *game) {
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
void scout_zone(GameState *game) {
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
void gather_resources(GameState *game) {
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
void explore_special_location(GameState *game) {
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
  case ZONE_DEEPWOOD_HOLLOW:
    if (game->druid_quest != QUEST_ACTIVE) {
      printf("드루이드 에이브가 말합니다. \"이 숲은 오래된 의식을 기다리고 있습니다. "
             "저에게 먼저 말을 걸어주세요.\"\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    if (game->druid_quest == QUEST_ACTIVE && game->player.herbs >= 4) {
      /* Complete druid ritual */
      game->player.herbs -= 4;
      game->druid_quest = QUEST_COMPLETE;
      game->player.spirit_totem = true;
      game->player.max_hp += 15;
      game->player.hp = game->player.max_hp;
      game->doom = clamp_int(game->doom - 1, 0, 12);
      advance_time(game, 40);
      flush_events(game);
      printf("에이브가 약초를 고대 석판 위에 배치하고 의식을 시작합니다. 숲의 기운이 "
             "당신 주위를 감싸며 영혼의 토템이 가슴속에 자리를 잡습니다. "
             "최대 체력 +15.\n");
    } else if (game->druid_quest == QUEST_ACTIVE) {
      printf("에이브가 조용히 말합니다. \"약초 4개가 필요합니다. 현재 %d개입니다.\"\n",
             game->player.herbs);
      advance_time(game, 15);
      flush_events(game);
    }
    return;
  case ZONE_ANCIENT_BEACON:
    if (game->beacon_quest != QUEST_ACTIVE) {
      printf("봉화대 문이 잠겨 있습니다. 봉화지기 오른에게 먼저 말을 걸어 보세요.\n");
      advance_time(game, 15);
      flush_events(game);
      return;
    }
    if (game->beacon_lit) {
      printf("봉화는 이미 밝게 타오르고 있습니다.\n");
      advance_time(game, 10);
      flush_events(game);
      return;
    }
    if (game->player.ore < 2 || game->player.herbs < 1) {
      printf("봉화를 밝히려면 광석 2개와 약초 1개가 필요합니다. "
             "(현재: 광석 %d, 약초 %d)\n",
             game->player.ore, game->player.herbs);
      advance_time(game, 10);
      flush_events(game);
      return;
    }
    game->player.ore -= 2;
    game->player.herbs -= 1;
    game->beacon_lit = true;
    game->beacon_quest = QUEST_COMPLETE;
    game->doom = clamp_int(game->doom - 2, 0, 12);
    advance_time(game, 45);
    flush_events(game);
    printf("광석으로 불씨를 만들고 약초 기름을 태워 고대 봉화를 다시 밝힙니다. "
           "봉화의 빛이 남쪽 하늘을 물들입니다. 파멸도가 2 감소했습니다.\n");
    return;
  case ZONE_SHATTERED_VAULT:
    if (game->vault_quest == QUEST_COMPLETE) {
      printf("금고는 이미 탐사했습니다. 고대의 기록들이 먼지 속에 잠들어 있습니다.\n");
      advance_time(game, 15);
      flush_events(game);
      return;
    }
    printf("먼지 쌓인 금고를 조심스럽게 탐사합니다...\n");
    advance_time(game, 60);
    flush_events(game);
    game->vault_quest = QUEST_COMPLETE;
    {
      int gold_found = roll_range(60, 100);
      game->player.gold += gold_found;
      game->player.relic_dust += 5;
      if (game->player.player_class == CLASS_MAGE) {
        game->player.rune_shards += 3;
      }
      printf("왕국의 마지막 보물 창고에서 발굴했습니다: 골드 %d, "
             "유물 가루 5개",
             gold_found);
      if (game->player.player_class == CLASS_MAGE) {
        printf(", 룬 파편 3개");
      }
      printf(".\n");
      printf("또한 왕국의 진실을 담은 고대 기록을 발견했습니다. "
             "공허의 왕관은 왕국을 보호하기 위해 만들어진 것이었으나, "
             "왕의 절망이 그것을 오염시켰습니다...\n");
    }
    return;
  case ZONE_ECHO_SHORE:
    if (game->shore_quest != QUEST_ACTIVE) {
      printf("해안을 둘러보지만 은자 레나의 안내 없이는 정화 의식을 시작할 수 없습니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    if (game->shore_quest == QUEST_ACTIVE && game->player.herbs >= 3 &&
        game->player.relic_dust >= 1) {
      game->player.herbs -= 3;
      game->player.relic_dust -= 1;
      game->shore_quest = QUEST_COMPLETE;
      game->player.max_hp += 8;
      game->player.hp = game->player.max_hp;
      game->doom = clamp_int(game->doom - 1, 0, 12);
      advance_time(game, 50);
      flush_events(game);
      printf("레나가 약초와 유물 가루를 해안 제단에 올려놓고 정화 의식을 시작합니다. "
             "바다의 기운이 상처를 씻어냅니다. 최대 체력 +8, 파멸도 -1.\n");
      if (game->player.player_class == CLASS_CLERIC) {
        game->player.holy_water += 2;
        printf("성직자로서 신성한 해안에서 성수 2개를 추가로 채집했습니다.\n");
      }
    } else if (game->shore_quest == QUEST_ACTIVE) {
      printf("레나가 말합니다. \"의식에는 약초 3개와 유물 가루 1개가 필요합니다. "
             "(현재: 약초 %d개, 유물 가루 %d개)\"\n",
             game->player.herbs, game->player.relic_dust);
      advance_time(game, 15);
      flush_events(game);
    }
    return;
  case ZONE_BONE_TOMB:
    printf("뼈 무덤 깊숙이 탐사합니다...\n");
    advance_time(game, 30);
    flush_events(game);
    if (rand() % 100 < 60) {
      int dust_found = 1 + rand() % 2;
      printf("오래된 관에서 유물을 발견했습니다.\n");
      int gold_found = roll_range(10, 25);
      game->player.gold += gold_found;
      game->player.relic_dust += dust_found;
      printf("골드 %d, 유물 가루 %d개를 수습했습니다.\n", gold_found, dust_found);
    } else {
      printf("무덤 내부에 아무것도 남아 있지 않습니다.\n");
    }
    return;
  case ZONE_LIGHT_SPIRE:
    if (!game->dawn_key_forged) {
      printf("에반이 고서 더미 너머로 말합니다. \"성채의 어둠을 몰아낼 임무는 "
             "아직 그 자격을 갖춘 자를 기다립니다. 더 깊이 알고 오세요.\"\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    {
      int gold_found = roll_range(15, 30);
      game->player.gold += gold_found;
      if (game->player.player_class == CLASS_CLERIC) {
        game->player.holy_water++;
        printf("첨탑 성소에서 고요히 기도했습니다. 골드 %d를 받고 성수 1개를 "
               "축성했습니다.\n", gold_found);
      } else {
        printf("첨탑 서고에서 오래된 지식을 연구했습니다. 골드 %d 가치의 "
               "기록을 가져옵니다.\n", gold_found);
      }
      advance_time(game, 40);
      flush_events(game);
    }
    return;
  case ZONE_IRON_CITADEL:
    if (game->citadel_warden_defeated) {
      printf("성채는 이미 해방되었습니다. 조용한 복도를 걷습니다.\n");
      advance_time(game, 15);
      flush_events(game);
      return;
    }
    if (game->citadel_quest != QUEST_ACTIVE) {
      printf("성채 깊숙이 진입하려 하지만, 빛의 첨탑의 에반에게서 "
             "임무를 받아야 성채의 마지막 군주와 맞설 수 있습니다.\n");
      advance_time(game, 20);
      flush_events(game);
      return;
    }
    printf("성채 왕좌의 방으로 진격합니다.\n");
    advance_time(game, 30);
    flush_events(game);
    result = run_battle(game, build_citadel_warden(game));
    if (result == BATTLE_RESULT_VICTORY) {
      game->citadel_warden_defeated = true;
      game->citadel_quest = QUEST_COMPLETE;
      game->player.gold += 40;
      game->player.relic_dust += 3;
      game->doom = clamp_int(game->doom - 1, 0, 12);
      refresh_rumor(game);
      printf("성채의 마지막 군주가 쓰러지며 성채가 해방되었습니다. "
             "골드 40, 유물 가루 3개, 파멸도 -1.\n");
      if (!game->player.titan_blade && !game->player.steel_edge) {
        game->player.titan_blade = true;
        printf("성채 무기고에서 전설의 타이탄 검을 발견했습니다. 공격력 +7!\n");
      } else if (game->player.steel_edge && !game->player.titan_blade) {
        game->player.steel_edge = false;
        game->player.titan_blade = true;
        printf("성채 무기고의 불꽃에서 무기를 재단련해 타이탄 검으로 강화했습니다! "
               "공격력 +4 → +7.\n");
      }
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
void interact_here(GameState *game) {
  const ZoneData *zone = &kZones[game->player.zone];
  if (zone_has_merchant(game, game->player.zone)) {
    printf("상인과 거래할 준비를 합니다.\n");
    shop_here(game);
    return;
  }
  if (zone->forge) {
    printf("대장간 설비를 점검합니다.\n");
    forge_here(game);
    return;
  }
  if (zone->healer && game->player.hp < game->player.max_hp) {
    int heal = 10 + game->player.level;
    game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
    printf("치유소에서 응급 처치를 받아 체력 %d를 회복했습니다.\n", heal);
    advance_time(game, 20);
    flush_events(game);
    return;
  }
  if (zone->npc != NULL && zone->npc[0] != '\0') {
    talk_here(game);
    return;
  }
  explore_special_location(game);
}
void talk_here(GameState *game) {
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
  case ZONE_DEEPWOOD_HOLLOW:
    if (game->druid_quest == QUEST_LOCKED) {
      game->druid_quest = QUEST_ACTIVE;
      printf("드루이드 에이브가 고목 그늘에서 당신을 바라봅니다. \"숲이 아파하고 있습니다. "
             "공허의 어둠이 뿌리까지 스며들기 전에 의식을 치러야 합니다. 신선한 약초 4개를 "
             "가져온다면 숲의 기운을 당신과 나누겠습니다.\"\n");
    } else if (game->druid_quest == QUEST_ACTIVE) {
      printf("에이브가 말합니다. \"약초 4개가 필요합니다. 월광 늪이나 숯불 수림, "
             "혹은 이 심숲 분지에서 채집할 수 있습니다. (현재 %d개)\"\n",
             game->player.herbs);
    } else {
      printf("에이브가 고개를 끄덕입니다. \"숲이 당신을 기억할 것입니다.\"\n");
      if (rand() % 100 < 40) {
        game->player.hp = clamp_int(game->player.hp + 8, 0, game->player.max_hp);
        printf("숲의 기운이 상처를 어루만져 체력이 조금 회복되었습니다.\n");
      }
    }
    break;
  case ZONE_ANCIENT_BEACON:
    if (game->beacon_quest == QUEST_LOCKED) {
      game->beacon_quest = QUEST_ACTIVE;
      printf("봉화지기 오른이 꺼진 봉화를 바라봅니다. \"이 봉화는 왕국이 살아있을 때부터 "
             "희망의 빛이었습니다. 광석 2개와 약초 1개면 다시 밝힐 수 있습니다. "
             "봉화 탑에서 '탐사' 명령으로 점화해주세요.\"\n");
    } else if (game->beacon_quest == QUEST_ACTIVE) {
      printf("오른이 말합니다. \"봉화를 밝히려면 광석 2개와 약초 1개가 필요합니다. "
             "(현재: 광석 %d, 약초 %d) '탐사' 명령으로 점화하세요.\"\n",
             game->player.ore, game->player.herbs);
      game->player.hp = game->player.max_hp;
      printf("오른이 상처를 돌봐줍니다. 체력이 완전히 회복되었습니다.\n");
    } else {
      printf("오른이 활활 타오르는 봉화를 바라보며 웃습니다. \"빛이 돌아왔습니다. "
             "왕국이 조금 더 버틸 수 있을 겁니다.\"\n");
      game->player.hp = game->player.max_hp;
      printf("오른이 상처를 돌봐줍니다. 체력이 완전히 회복되었습니다.\n");
    }
    break;
  case ZONE_ECHO_SHORE:
    if (game->shore_quest == QUEST_LOCKED) {
      game->shore_quest = QUEST_ACTIVE;
      refresh_rumor(game);
      printf("은자 레나가 모래 위에 그린 신성한 문양을 가리킵니다. \"공허의 그늘이 "
             "이 해안까지 뻗어왔습니다. 오래된 정화 의식을 다시 치러야 합니다. "
             "신선한 약초 3개와 유물 가루 1개를 가져온다면, 의식을 함께 "
             "집행하겠습니다.\"\n");
    } else if (game->shore_quest == QUEST_ACTIVE) {
      printf("레나가 말합니다. \"해안의 어둠이 짙어지고 있습니다. "
             "약초 3개와 유물 가루 1개를 모으면 정화 의식을 시작하세요. "
             "(현재: 약초 %d개, 유물 가루 %d개)\"\n",
             game->player.herbs, game->player.relic_dust);
    } else {
      printf("레나가 밝게 웃습니다. \"해안이 숨을 쉬기 시작했어요. "
             "고마워요. 오래도록 이 바다를 지킬 수 있을 것 같습니다.\"\n");
      if (rand() % 100 < 40) {
        game->player.hp = clamp_int(game->player.hp + 10, 0, game->player.max_hp);
        printf("레나가 따뜻한 차를 내어줍니다. 체력이 약간 회복되었습니다.\n");
      }
    }
    break;
  case ZONE_LIGHT_SPIRE:
    if (game->citadel_quest == QUEST_LOCKED) {
      if (!game->dawn_key_forged) {
        printf("첨탑 사서 에반이 지도 너머로 당신을 바라봅니다. \"성채의 어둠을 몰아내려면 "
               "여명의 열쇠를 먼저 완성해야 합니다. 파편을 모으고 다시 오세요.\"\n");
      } else {
        game->citadel_quest = QUEST_ACTIVE;
        refresh_rumor(game);
        printf("에반이 고지도를 펼칩니다. \"여명의 열쇠를 완성했군요. 이제 부서진 성채를 "
               "해방시킬 때입니다. 성채의 마지막 군주가 왕국 최후의 대장간을 점거하고 "
               "있습니다. 탐사 명령으로 그것을 물리치세요.\"\n");
      }
    } else if (game->citadel_quest == QUEST_ACTIVE) {
      printf("에반이 말합니다. \"부서진 성채로 가세요. '탐사' 명령으로 성채의 마지막 "
             "군주와 맞서야 합니다.\"\n");
      game->player.hp = game->player.max_hp;
      printf("에반이 상처를 치료해줍니다. 체력이 완전히 회복되었습니다.\n");
    } else {
      printf("에반이 환하게 웃습니다. \"성채가 해방되었군요! 첨탑의 지식도 이제 "
             "더욱 자유롭게 세상에 퍼질 수 있을 것입니다.\"\n");
      game->player.hp = game->player.max_hp;
      printf("에반이 상처를 치료해줍니다. 체력이 완전히 회복되었습니다.\n");
    }
    break;
  case ZONE_IRON_CITADEL:
    printf("성채는 함락되었습니다. 대화할 생존자를 찾을 수 없습니다.\n");
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
bool handle_trade_command(GameState *game, const char *command) {
  int *stock = NULL;
  bool port_prices = false;
  int price;
  int bomb_price;
  int rune_price = 15;
  int holy_water_price = 20;
  if (!zone_has_merchant(game, game->player.zone)) {
    return false;
  }
  if (game->player.zone == ZONE_GLOAM_PORT) {
    stock = &game->port_potions;
    port_prices = true;
  } else if (game->player.zone == ZONE_BRASS_MARKET) {
    stock = &game->market_potions;
  } else {
    stock = NULL;
  }
  price = potion_price(game, port_prices);
  bomb_price = port_prices ? 18 : 20;
  if (strcmp(command, "buy potion") == 0) {
    if (game->player.gold < price) {
      printf("골드가 부족합니다.\n");
      return true;
    }
    if (stock != NULL && *stock <= 0) {
      printf("해당 상인의 재고가 소진되었습니다.\n");
      return true;
    }
    game->player.gold -= price;
    game->player.potions++;
    if (stock != NULL) {
      (*stock)--;
    }
    printf("포션을 구매했습니다.\n");
    advance_time(game, 10);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "buy bomb") == 0) {
    if (game->player.gold < bomb_price) {
      printf("골드가 부족합니다.\n");
      return true;
    }
    game->player.gold -= bomb_price;
    game->player.bombs++;
    printf("폭탄을 구매했습니다.\n");
    advance_time(game, 10);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "buy rune") == 0) {
    if (game->player.gold < rune_price) {
      printf("골드가 부족합니다.\n");
      return true;
    }
    game->player.gold -= rune_price;
    game->player.rune_shards++;
    printf("룬 파편을 구매했습니다.\n");
    advance_time(game, 10);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "buy holy water") == 0) {
    if (game->player.gold < holy_water_price) {
      printf("골드가 부족합니다.\n");
      return true;
    }
    game->player.gold -= holy_water_price;
    game->player.holy_water++;
    printf("성수를 구매했습니다.\n");
    advance_time(game, 10);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "sell herb") == 0) {
    if (game->player.herbs <= 0) {
      printf("판매할 약초가 없습니다.\n");
      return true;
    }
    game->player.herbs--;
    game->player.gold += 3;
    printf("약초를 3골드에 판매했습니다.\n");
    advance_time(game, 5);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "sell ore") == 0) {
    if (game->player.ore <= 0) {
      printf("판매할 광석이 없습니다.\n");
      return true;
    }
    game->player.ore--;
    game->player.gold += 4;
    printf("광석을 4골드에 판매했습니다.\n");
    advance_time(game, 5);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "sell dust") == 0) {
    if (game->player.relic_dust <= 0) {
      printf("판매할 유물 가루가 없습니다.\n");
      return true;
    }
    game->player.relic_dust--;
    game->player.gold += 8;
    printf("유물 가루를 8골드에 판매했습니다.\n");
    advance_time(game, 5);
    flush_events(game);
    return true;
  }
  return false;
}
void shop_here(GameState *game) {
  int *stock = NULL;
  bool port_prices = false;
  int price;
  int bomb_price;
  int rune_price = 15;
  int holy_water_price = 20;
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
  price = potion_price(game, port_prices);
  bomb_price = port_prices ? 18 : 20;
  printf("상점 이용 가능: `buy potion`, `buy bomb`, `buy rune`, `buy holy water`\n");
  printf("판매 가능: `sell herb`, `sell ore`, `sell dust`\n");
  printf("재고: 포션 %d | 포션 %dg | 폭탄 %dg | 룬 %dg | 성수 %dg\n",
         stock != NULL ? *stock : 2, price, bomb_price, rune_price,
         holy_water_price);
  printf("힌트: 거래 명령을 바로 입력하면 즉시 처리됩니다.\n");
}
bool handle_craft_command(GameState *game, const char *command) {
  if (!kZones[game->player.zone].forge) {
    return false;
  }
  if (strcmp(command, "craft blade") == 0) {
    if (game->player.steel_edge) {
      printf("이미 검을 재단련했습니다.\n");
      return true;
    }
    if (game->player.ore < 4) {
      printf("광석 4개가 필요합니다.\n");
      return true;
    }
    game->player.ore -= 4;
    game->player.steel_edge = true;
    printf("무기를 재단련해 강철 칼날로 만들었습니다.\n");
    advance_time(game, 45);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "craft mail") == 0) {
    if (game->player.ward_mail) {
      printf("이미 갑옷을 강화했습니다.\n");
      return true;
    }
    if (game->player.ore < 6) {
      printf("광석 6개가 필요합니다.\n");
      return true;
    }
    game->player.ore -= 6;
    game->player.ward_mail = true;
    printf("새 철판을 단련해 수호 갑옷 세트를 완성했습니다.\n");
    advance_time(game, 55);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "craft bomb") == 0) {
    if (game->player.ore < 2) {
      printf("광석 2개가 필요합니다.\n");
      return true;
    }
    game->player.ore -= 2;
    game->player.bombs++;
    printf("거칠지만 실용적인 폭탄을 제작했습니다.\n");
    advance_time(game, 20);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "craft rune") == 0) {
    if (game->player.ore < 3) {
      printf("광석 3개가 필요합니다.\n");
      return true;
    }
    game->player.ore -= 3;
    game->player.rune_shards += 2;
    printf("광석을 정제해 룬 파편 2개를 만들었습니다.\n");
    advance_time(game, 30);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "craft holy water") == 0) {
    if (game->player.ore < 1 || game->player.herbs < 2) {
      printf("광석 1개와 약초 2개가 필요합니다. (현재: 광석 %d, 약초 %d)\n",
             game->player.ore, game->player.herbs);
      return true;
    }
    game->player.ore -= 1;
    game->player.herbs -= 2;
    game->player.holy_water++;
    printf("광석 열기로 약초를 달여 성수 1개를 만들었습니다.\n");
    advance_time(game, 25);
    flush_events(game);
    return true;
  }
  if (strcmp(command, "craft titan blade") == 0) {
    if (game->player.titan_blade) {
      printf("이미 타이탄 검을 보유하고 있습니다.\n");
      return true;
    }
    if (game->player.ore < 6) {
      printf("광석 6개가 필요합니다. (현재: %d개)\n", game->player.ore);
      return true;
    }
    game->player.ore -= 6;
    game->player.steel_edge = false;
    game->player.titan_blade = true;
    printf("최고의 광석을 용광로에서 단련해 타이탄 검을 완성했습니다. 공격력 +7!\n");
    advance_time(game, 90);
    flush_events(game);
    return true;
  }
  return false;
}
void forge_here(GameState *game) {
  if (!kZones[game->player.zone].forge) {
    printf("이곳에는 작동 가능한 대장간이 없습니다.\n");
    return;
  }
  printf("대장간 제작 가능: `craft blade`, `craft mail`, `craft bomb`, `craft rune`\n");
  printf("고급 제작: `craft holy water`, `craft titan blade`\n");
  printf("현재 재료: 광석 %d | 약초 %d\n", game->player.ore, game->player.herbs);
  printf("힌트: 제작 명령을 바로 입력하면 즉시 처리됩니다.\n");
}
void rest_here(GameState *game) {
  int cost = 0;
  if (game->player.zone == ZONE_EMBERFALL_GATE ||
      game->player.zone == ZONE_VERDANT_ABBEY ||
      game->player.zone == ZONE_LIGHT_SPIRE) {
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
  if (next_zone == ZONE_SHATTERED_VAULT && !game->final_boss_defeated) {
    printf("봉인된 문들이 버티고 있습니다. 공허의 왕관이 부서지기 전까지 이 너머로는 "
           "나아갈 수 없습니다.\n");
    return false;
  }
  game->player.zone = next_zone;
  game->player.discovered[next_zone] = true;
  advance_time(game, 30);
  flush_events(game);
  /* Random travel event (30% chance) */
  if (!kZones[next_zone].safe && rand() % 100 < 30) {
    int roll = rand() % 8;
    switch (roll) {
    case 0: {
      int found = roll_range(3, 12);
      game->player.gold += found;
      printf("[여행] 길가에서 흩어진 동전 꾸러미를 발견했습니다. 골드 +%d.\n", found);
      break;
    }
    case 1:
      game->player.herbs++;
      printf("[여행] 바위틈에서 야생 약초 한 묶음을 발견했습니다.\n");
      break;
    case 2:
      if (kZones[next_zone].danger > 0) {
        printf("[여행] 이동 중 급습을 받았습니다!\n");
        hunt_current_zone(game);
      }
      break;
    case 3: {
      printf("[여행] 길가에 부상당한 여행자가 쓰러져 있습니다.\n");
      if (game->player.potions > 0) {
        game->player.potions--;
        int gratitude = roll_range(10, 20);
        game->player.gold += gratitude;
        printf("       포션을 나눠주자 감사해하며 숨겨둔 동전을 건넵니다. 골드 +%d.\n",
               gratitude);
      } else {
        printf("       나눠줄 포션이 없습니다. 여행자가 간신히 몸을 일으켜 이동합니다.\n");
      }
      break;
    }
    case 4:
      if (game->player.player_class == CLASS_MAGE || rand() % 100 < 30) {
        game->player.rune_shards++;
        printf("[여행] 바위에 새겨진 마법진에서 룬 파편을 수집했습니다.\n");
      } else {
        int found = roll_range(2, 7);
        game->player.gold += found;
        printf("[여행] 황량한 길가에서 낡은 동전을 줍습니다. 골드 +%d.\n", found);
      }
      break;
    case 5: {
      int heal = roll_range(3, 8);
      game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
      printf("[여행] 잠시 멈춰 숨을 고르며 체력 +%d를 회복했습니다.\n", heal);
      break;
    }
    case 6: {
      /* Abandoned cache */
      if (rand() % 100 < 50) {
        game->player.ore++;
        printf("[여행] 버려진 야영지 잔해에서 사용 가능한 광석 덩이를 발견했습니다.\n");
      } else {
        game->player.relic_dust++;
        printf("[여행] 오래된 비석 틈에서 유물 가루 한 주머니를 찾았습니다.\n");
      }
      break;
    }
    case 7: {
      /* Cleric and mage get holy_water/rune; others get an ore or herbs */
      if (game->player.player_class == CLASS_CLERIC) {
        game->player.holy_water++;
        printf("[여행] 신성한 샘에서 성수 1개를 채웠습니다.\n");
      } else if (kZones[next_zone].resource == RESOURCE_HERB) {
        game->player.herbs++;
        printf("[여행] 길가의 수풀에서 약초 한 묶음을 더 발견했습니다.\n");
      } else {
        int found = roll_range(5, 15);
        game->player.gold += found;
        printf("[여행] 버려진 짐 꾸러미에서 금화를 찾았습니다. 골드 +%d.\n", found);
      }
      break;
    }
    default:
      break;
    }
  }
  describe_zone(game);
  return true;
}
bool maybe_handle_movement_command(GameState *game, const char *command) {
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
