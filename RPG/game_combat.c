#define _POSIX_C_SOURCE 200809L
#include "game_shared.h"

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
Enemy build_regular_enemy(GameState *game, int zone) {
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
Enemy build_fragment_guardian(GameState *game, FragmentId fragment) {
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
Enemy build_final_boss(GameState *game) {
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
BattleResult run_battle(GameState *game, Enemy enemy) {
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
