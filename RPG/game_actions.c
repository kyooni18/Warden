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
void shop_here(GameState *game) {
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
void forge_here(GameState *game) {
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
void rest_here(GameState *game) {
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
