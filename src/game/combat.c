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
static void maybe_make_elite(Enemy *enemy) {
  char orig_name[64];
  if (enemy->boss) {
    return;
  }
  snprintf(orig_name, sizeof(orig_name), "%s", enemy->name);
  /* "정예 " prefix is 7 UTF-8 bytes; truncate original so total fits in 64 bytes */
  snprintf(enemy->name, sizeof(enemy->name), "정예 %.54s", orig_name);
  enemy->max_hp = (enemy->max_hp * 3) / 2;
  enemy->hp = enemy->max_hp;
  enemy->attack += 3;
  enemy->defense += 2;
  enemy->xp_reward = (enemy->xp_reward * 7) / 4;
  enemy->gold_reward = (enemy->gold_reward * 7) / 4;
  enemy->is_elite = true;
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
      enemy.burns_player = true;
    } else {
      enemy = make_enemy(
          "가시주술사", 29, 10, 4, 18, 17, 3, ENEMY_ROLE_CASTER, false, false,
          true,
          "살아 있는 가시에 휘감긴 주술사가 연기 속에서 걸어 나옵니다.",
          "주술사가 베기를 둔화시키는 가시 연기를 던집니다.");
      enemy.bleeds_player = true;
    }
    break;
  case ZONE_OBSIDIAN_CRATER:
    if (roll < 50) {
      enemy = make_enemy(
          "유리 드레이크", 38, 11, 5, 22, 20, 4, ENEMY_ROLE_BRUTE, false, false,
          false,
          "유리 드레이크가 분화구 가장자리를 넘어오며 파편을 흩뿌립니다.",
          "드레이크가 용암 발톱으로 전진하며 내리칩니다.");
      enemy.burns_player = true;
    } else {
      enemy = make_enemy(
          "불씨 광신도", 34, 11, 5, 22, 21, 4, ENEMY_ROLE_CASTER, false,
          false, true,
          "검은 유리 가면을 쓴 광신도가 증기 속에서 모습을 드러냅니다.",
          "광신도가 자세를 흔드는 화염 찬가를 뿜어냅니다.");
      enemy.burns_player = true;
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
  case ZONE_DEEPWOOD_HOLLOW:
    if (roll < 50) {
      enemy = make_enemy(
          "숲의 수호령", 34, 10, 5, 20, 18, 3, ENEMY_ROLE_CASTER, false, false,
          true,
          "오래된 나무에서 녹색 불꽃 눈을 가진 수호령이 몸을 드러냅니다.",
          "수호령이 가시 덩굴을 날려 상처를 내고 계속 피를 흘리게 합니다.");
      enemy.bleeds_player = true;
    } else {
      enemy = make_enemy(
          "가시 정령", 38, 11, 6, 22, 19, 4, ENEMY_ROLE_BRUTE, false, false,
          false,
          "살아있는 가시 덩굴이 뭉쳐 거대한 정령의 형상을 이룹니다.",
          "정령이 날카로운 가시를 전방으로 뿜어냅니다.");
      enemy.bleeds_player = true;
    }
    break;
  case ZONE_MAGMA_RIFT:
    if (roll < 50) {
      enemy = make_enemy(
          "용암 골렘", 50, 13, 7, 28, 24, 5, ENEMY_ROLE_BRUTE, false, false,
          false,
          "균열 틈에서 굳은 용암 덩어리가 일어서며 붉은 눈을 뜹니다.",
          "골렘이 이글거리는 주먹으로 땅을 내리쳐 용암 파편을 튀깁니다.");
      enemy.burns_player = true;
    } else {
      enemy = make_enemy(
          "화염 폭군", 44, 14, 6, 30, 26, 5, ENEMY_ROLE_CASTER, false, false,
          true,
          "붉은 갑옷을 두른 폭군이 용암 위를 걸으며 화염의 노래를 부릅니다.",
          "폭군이 화염 폭발을 일으켜 주변 모든 것을 불태웁니다.");
      enemy.burns_player = true;
    }
    break;
  case ZONE_ANCIENT_BEACON:
    if (roll < 50) {
      enemy = make_enemy(
          "길 잃은 병사", 28, 9, 4, 16, 14, 2, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "녹슨 갑옷을 입은 병사가 멍한 눈으로 길을 막아섭니다.",
          "병사가 마지막 힘을 쥐어짜 돌진합니다.");
    } else {
      enemy = make_enemy(
          "망각의 망령", 31, 9, 4, 17, 15, 3, ENEMY_ROLE_CASTER, false, false,
          true,
          "봉화대 주변을 떠도는 망령이 당신의 발걸음 소리에 고개를 돌립니다.",
          "망령이 기억을 지우는 저주를 내뿜습니다.");
    }
    break;
  case ZONE_ECHO_SHORE:
    if (roll < 50) {
      enemy = make_enemy(
          "조류 정령", 32, 10, 5, 19, 17, 3, ENEMY_ROLE_CASTER, false, false,
          false,
          "달빛을 받아 빛나는 파도에서 조류 정령이 솟아오릅니다.",
          "정령이 불타는 해류를 뿌려 화상을 남깁니다.");
      enemy.burns_player = true;
    } else {
      enemy = make_enemy(
          "해안 심연체", 36, 11, 6, 21, 19, 4, ENEMY_ROLE_BRUTE, false, false,
          false,
          "난파선 잔해 아래에서 기어 나온 거대한 심연체가 집게발을 휘두릅니다.",
          "심연체가 집게발로 살을 찢으며 출혈을 일으킵니다.");
      enemy.bleeds_player = true;
    }
    break;
  case ZONE_BONE_TOMB:
    if (roll < 50) {
      enemy = make_enemy(
          "해골 기사", 40, 12, 7, 24, 22, 4, ENEMY_ROLE_BRUTE, false, false,
          true,
          "녹슨 철갑을 두른 해골 기사가 지하 묘지 깊은 곳에서 걸어 나옵니다.",
          "기사가 낡은 전쟁 도끼를 들어 쇠약화 일격을 내립니다.");
    } else {
      enemy = make_enemy(
          "저주받은 군주", 45, 13, 7, 27, 25, 5, ENEMY_ROLE_CASTER, false,
          false, true,
          "화려했던 갑옷의 흔적이 남은 저주받은 군주가 어둠 속에서 왕좌에서 일어섭니다.",
          "군주가 저주의 화염을 발사해 주변을 불태웁니다.");
      enemy.burns_player = true;
    }
    break;
  case ZONE_LIGHT_SPIRE:
    if (roll < 50) {
      enemy = make_enemy(
          "길 잃은 음영", 24, 8, 4, 15, 13, 2, ENEMY_ROLE_SKIRMISHER, false,
          false, false,
          "첨탑 외곽을 떠도는 음영이 빛에 이끌려 다가옵니다.",
          "음영이 빠른 허깨비 일격으로 파고듭니다.");
    } else {
      enemy = make_enemy(
          "부서진 수호령", 28, 9, 4, 17, 15, 3, ENEMY_ROLE_CASTER, false,
          false, true,
          "한때 첨탑을 지켰던 수호령이 공허에 오염되어 공격해 옵니다.",
          "수호령이 잔재 에너지로 집중을 흐트러뜨립니다.");
    }
    break;
  case ZONE_IRON_CITADEL:
    if (roll < 50) {
      enemy = make_enemy(
          "철갑 구울", 42, 12, 8, 25, 23, 4, ENEMY_ROLE_BRUTE, false, false,
          false,
          "성채 복도에서 두꺼운 철갑 조각을 두른 구울이 몸을 일으킵니다.",
          "구울이 강철 발톱으로 짓눌러 파고듭니다.");
    } else {
      enemy = make_enemy(
          "성채 군주", 50, 14, 9, 30, 27, 5, ENEMY_ROLE_BRUTE, false, false,
          false,
          "왕국의 갑주를 착용한 성채 군주가 무너진 왕좌에서 일어섭니다.",
          "군주가 전력으로 철갑 일격을 내려칩니다.");
      enemy.bleeds_player = true;
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
  /* 20% chance (30% at high doom) to encounter an elite variant */
  {
    int elite_threshold = (game->doom >= 6) ? 30 : 20;
    if (!enemy.boss && rand() % 100 < elite_threshold) {
      maybe_make_elite(&enemy);
    }
  }
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
Enemy build_citadel_warden(GameState *game) {
  Enemy boss =
      make_enemy("성채의 마지막 군주", 65, 15, 9, 50, 40, 5, ENEMY_ROLE_BOSS,
                 true, false, true,
                 "성채 내부 깊숙한 왕좌의 방에서 공허에 물든 성채의 마지막 군주가 "
                 "철갑 투구를 올려씁니다.",
                 "군주가 성채 전체를 울리는 고함과 함께 전력의 철갑 일격을 날립니다.");
  scale_enemy(game, &boss, 5);
  return boss;
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
  if (zone == ZONE_MOONFEN || zone == ZONE_CINDER_GROVE ||
      zone == ZONE_DEEPWOOD_HOLLOW || zone == ZONE_ECHO_SHORE) {
    if (rand() % 100 < 35) {
      game->player.herbs++;
      printf("전투의 여파 속에서 쓸 만한 약초 묶음을 찾아냈습니다.\n");
    }
  } else if (zone == ZONE_IRONWOOD_PASS || zone == ZONE_ASHEN_QUARRY ||
             zone == ZONE_OBSIDIAN_CRATER || zone == ZONE_MAGMA_RIFT ||
             zone == ZONE_IRON_CITADEL) {
    if (rand() % 100 < 35) {
      game->player.ore++;
      printf("전장에서 가공 가능한 광석 덩이를 떼어냈습니다.\n");
    }
  } else if (zone == ZONE_SUNKEN_ARCHIVE || zone == ZONE_RUINED_BASILICA ||
             zone == ZONE_SHATTERED_VAULT || zone == ZONE_BONE_TOMB) {
    if (rand() % 100 < 25) {
      game->player.relic_dust++;
      printf("폐허에서 반짝이는 유물 가루를 모았습니다.\n");
    }
  }
  /* Mage has a chance to recover a rune shard */
  if (game->player.player_class == CLASS_MAGE && rand() % 100 < 25) {
    game->player.rune_shards++;
    printf("마력 잔재에서 룬 파편을 회수했습니다.\n");
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
/* ---- Non-blocking combat input ----
 * Waits for a keypress (or typed command + Enter).  Single-key shortcuts
 * map immediately to a command string without needing Enter.
 * Returns false only if the game should terminate (window closed, etc.).
 */
static bool combat_read_command(GameState *game, char *cmd, size_t cmd_size)
{
  TuiState *tui = tui_get_global();
  char buf[MAX_INPUT];
  int  len = 0;
  memset(buf, 0, sizeof(buf));

  /* Update the shared input buffer display */
  if (tui) {
    tui->input_buf[0] = '\0';
    tui->input_len = 0;
  }

  while (1) {
    /* Refresh combat display every iteration */
    if (tui && tui->initialized) {
      tui_draw_combat_overlay(tui, game);
    }

    int ch = getch();
    if (ch == ERR) {
      struct timespec ts = {0, 20000000L}; /* 20 ms */
      nanosleep(&ts, NULL);
      continue;
    }

    /* ---- Single-key shortcuts (only when buffer is empty) ---- */
    if (len == 0) {
      const char *shortcut = NULL;
      switch (ch) {
      case '1': shortcut = "attack"; break;
      case '2': shortcut = "guard";  break;
      case '3':
        /* Class ability */
        switch (game->player.player_class) {
        case CLASS_WARRIOR: shortcut = "parry";    break;
        case CLASS_SCOUT:   shortcut = "backstab"; break;
        case CLASS_MAGE:    shortcut = "fireball"; break;
        case CLASS_CLERIC:  shortcut = "smite";    break;
        }
        break;
      case '4': shortcut = "flee";           break;
      case '5': shortcut = "potion";         break;
      case '6': shortcut = "bomb";           break;
      case '7': shortcut = "use holy water"; break;
      default:  break;
      }
      if (shortcut) {
        snprintf(cmd, cmd_size, "%s", shortcut);
        if (tui) { tui->input_buf[0] = '\0'; tui->input_len = 0; }
        return true;
      }
    }

    /* Backspace */
    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
      if (len > 0) {
        buf[--len] = '\0';
        if (tui && len < (int)sizeof(tui->input_buf)) {
          memcpy(tui->input_buf, buf, (size_t)len + 1);
          tui->input_len = len;
        }
      }
      continue;
    }

    /* Enter: submit */
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (len > 0) {
        canonicalize_input(buf, cmd, cmd_size);
        if (tui) { tui->input_buf[0] = '\0'; tui->input_len = 0; }
        return cmd[0] != '\0';
      }
      continue;
    }

    /* Printable ASCII */
    if (ch >= 32 && ch < 127 && len < (int)sizeof(buf) - 1) {
      buf[len++] = (char)ch;
      buf[len]   = '\0';
      if (tui && len < (int)sizeof(tui->input_buf)) {
        memcpy(tui->input_buf, buf, (size_t)len + 1);
        tui->input_len = len;
      }
    }
  }
}

BattleResult run_battle(GameState *game, Enemy enemy) {
  TuiState *tui = tui_get_global();
  char command[MAX_INPUT];
  game->combat.active = true;
  game->combat.guard_active = false;
  game->combat.parry_active = false;
  game->combat.holy_barrier_active = false;
  game->combat.enemy_charging = false;
  game->combat.weaken_turns = 0;
  game->combat.enemy_burn_turns = 0;
  game->combat.enemy_stun_turns = 0;
  game->combat.player_bleed_turns = 0;
  game->combat.enemy = enemy;
  if (tui) tui->combat_mode = true;
  if (game->combat.enemy.is_elite) {
    printf("★ 정예 적 등장! ★");
  }
  printf("%s", game->combat.enemy.intro);
  while (game->combat.enemy.hp > 0 && game->player.hp > 0) {
    int player_damage = 0;
    int enemy_damage = 0;
    bool spend_turn = false;
    /* Status line written to the log so it's visible in the scrolling history */
    {
      char status[256];
      int slen = 0;
      slen += snprintf(status + slen, sizeof(status) - slen,
                       "%s HP %d/%d", game->player.name,
                       game->player.hp, game->player.max_hp);
      if (game->combat.player_bleed_turns > 0)
        slen += snprintf(status + slen, sizeof(status) - slen,
                         " [출혈%d]", game->combat.player_bleed_turns);
      if (game->combat.weaken_turns > 0)
        slen += snprintf(status + slen, sizeof(status) - slen,
                         " [약화%d]", game->combat.weaken_turns);
      slen += snprintf(status + slen, sizeof(status) - slen,
                       " | %s HP %d/%d", game->combat.enemy.name,
                       game->combat.enemy.hp, game->combat.enemy.max_hp);
      if (game->combat.enemy_burn_turns > 0)
        slen += snprintf(status + slen, sizeof(status) - slen,
                         " [화상%d]", game->combat.enemy_burn_turns);
      if (game->combat.enemy_stun_turns > 0)
        slen += snprintf(status + slen, sizeof(status) - slen,
                         " [기절%d]", game->combat.enemy_stun_turns);
      (void)slen;
      printf("%s", status);
    }
    if (!combat_read_command(game, command, sizeof(command))) {
      game->running = false;
      game->combat.active = false;
      if (tui) tui->combat_mode = false;
      return BATTLE_RESULT_DEFEAT;
    }
    if (command[0] == '\0') {
      continue;
    }
    if (strcmp(command, "help") == 0) {
      printf("공용: attack, cleave(Lv3), devastate(Lv6), guard, potion, bomb, flee, status");
      if (game->player.player_class == CLASS_WARRIOR) {
        printf("전사: parry, bash(Lv5)\n");
      } else if (game->player.player_class == CLASS_SCOUT) {
        printf("척후: backstab, vanish(Lv4)\n");
      } else if (game->player.player_class == CLASS_MAGE) {
        printf("마법사: fireball(룬1), frost(Lv3, 룬1)\n");
      } else if (game->player.player_class == CLASS_CLERIC) {
        printf("성직자: smite(신성 피해+자가치유), holy_barrier(Lv4, 공격차단+치유)\n");
      }
      if (game->player.holy_water > 0) {
        printf("공용 아이템: use holy water (체력 25 회복+DoT 제거, 현재 %d개)\n",
               game->player.holy_water);
      }
      continue;
    }
    if (strcmp(command, "status") == 0) {
      const char *state = game->combat.weaken_turns > 0
                              ? "약화됨"
                              : (game->combat.player_bleed_turns > 0
                                     ? "출혈중"
                                     : "안정적");
      printf("현재 상태: %s\n", state);
      if (game->player.player_class == CLASS_MAGE) {
        printf("룬 파편: %d개\n", game->player.rune_shards);
      }
      if (game->player.holy_water > 0) {
        printf("성수: %d개\n", game->player.holy_water);
      }
      continue;
    }
    /* ---- Universal commands ---- */
    if (strcmp(command, "potion") == 0 ||
        strcmp(command, "use potion") == 0) {
      if (game->player.potions <= 0) {
        printf("포션이 없습니다.\n");
        continue;
      }
      {
        int heal = 18 + game->player.level * 2;
        /* Cleric passive: potions heal +8 extra */
        if (game->player.player_class == CLASS_CLERIC) {
          heal += 8;
        }
        /* Rain weather: potions heal +5 extra */
        if (game->weather == WEATHER_RAIN) {
          heal += 5;
        }
        game->player.potions--;
        game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
        printf("포션을 마시고 자세를 가다듬습니다. (체력 %d/%d)\n",
               game->player.hp, game->player.max_hp);
      }
      spend_turn = true;
    } else if (strcmp(command, "use holy water") == 0 ||
               strcmp(command, "holy water") == 0) {
      if (game->player.holy_water <= 0) {
        printf("성수가 없습니다.\n");
        continue;
      }
      {
        int heal = 25;
        if (game->player.player_class == CLASS_CLERIC) {
          heal += 10;
        }
        if (game->weather == WEATHER_RAIN) {
          heal += 5;
        }
        game->player.holy_water--;
        game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
        game->combat.player_bleed_turns = 0;
        game->combat.weaken_turns = 0;
        printf("성수를 사용해 체력 %d를 회복하고 상태이상을 해제했습니다. (체력 %d/%d)\n",
               heal, game->player.hp, game->player.max_hp);
      }
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
      game->combat.parry_active = false;
      game->combat.holy_barrier_active = false;
      if (game->player.abbey_sigil) {
        game->player.hp =
            clamp_int(game->player.hp + 2, 0, game->player.max_hp);
        printf("무기를 세워 자세를 잡자 수도원 인장이 빛납니다.\n");
      } else {
        printf("다음 공격에 대비해 방어 태세를 취합니다.\n");
      }
      spend_turn = true;
    } else if (strcmp(command, "devastate") == 0) {
      if (game->player.level < 6) {
        printf("아직 궁극 강타를 익히지 못했습니다. (레벨 6 필요)\n");
        continue;
      }
      if (game->player.hp <= 15) {
        printf("체력이 너무 낮아 궁극 강타를 사용할 수 없습니다. (체력 15 초과 필요)\n");
        continue;
      }
      {
        /* Costs 12 HP, ignores 75% of enemy defense */
        int attack = player_attack_value(game) * 2 + 10;
        int quarter_def = game->combat.enemy.defense / 4;
        if (game->combat.weaken_turns > 0) {
          attack -= 5;
        }
        player_damage = compute_damage(attack, quarter_def);
        game->player.hp -= 12;
        game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                          game->combat.enemy.max_hp);
        printf("전력을 불태워 궁극 강타! 자신에게 12 피해를 감수하며 %s에게 %d를 "
               "입혔습니다. (체력 %d/%d)\n",
               game->combat.enemy.name, player_damage,
               game->player.hp, game->player.max_hp);
      }
      spend_turn = true;
    } else if (strcmp(command, "cleave") == 0) {
      int attack = player_attack_value(game) + 6;
      if (game->player.level < 3) {
        printf("아직 전력 가르기를 익히지 못했습니다. (레벨 3 필요)\n");
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
        printf("틈을 만들어 탈출했습니다.");
        advance_time(game, 10);
        flush_events(game);
        game->combat.active = false;
        if (tui) tui->combat_mode = false;
        return BATTLE_RESULT_FLED;
      }
      printf("이탈을 시도했지만 %s이(가) 길을 막습니다.\n",
             game->combat.enemy.name);
      spend_turn = true;
    }
    /* ---- Warrior abilities ---- */
    else if (strcmp(command, "parry") == 0) {
      if (game->player.player_class != CLASS_WARRIOR) {
        printf("전사 전용 기술입니다.\n");
        continue;
      }
      game->combat.parry_active = true;
      game->combat.guard_active = false;
      printf("무기를 세워 막기 자세를 취합니다. 다음 공격을 튕겨내고 반격합니다.\n");
      spend_turn = true;
    } else if (strcmp(command, "bash") == 0) {
      if (game->player.player_class != CLASS_WARRIOR) {
        printf("전사 전용 기술입니다.\n");
        continue;
      }
      if (game->player.level < 5) {
        printf("아직 방패 강타를 익히지 못했습니다. (레벨 5 필요)\n");
        continue;
      }
      player_damage = 8 + game->player.level * 2;
      game->combat.enemy_stun_turns = 1;
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("방패로 %s을(를) 강타해 %d 피해를 주고 기절시켰습니다.\n",
             game->combat.enemy.name, player_damage);
      spend_turn = true;
    }
    /* ---- Scout abilities ---- */
    else if (strcmp(command, "backstab") == 0) {
      if (game->player.player_class != CLASS_SCOUT) {
        printf("척후 전용 기술입니다.\n");
        continue;
      }
      {
        int attack = player_attack_value(game) + 4;
        int half_defense = game->combat.enemy.defense / 2;
        if (game->combat.weaken_turns > 0) {
          attack -= 2;
        }
        player_damage = compute_damage(attack, half_defense);
        if (rand() % 100 < 50) {
          player_damage += 6 + game->player.level;
          printf("기습이 급소를 꿰뚫었습니다!\n");
        }
        game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                          game->combat.enemy.max_hp);
        printf("%s에게 기습 피해 %d를 입혔습니다.\n", game->combat.enemy.name,
               player_damage);
      }
      spend_turn = true;
    } else if (strcmp(command, "vanish") == 0) {
      if (game->player.player_class != CLASS_SCOUT) {
        printf("척후 전용 기술입니다.\n");
        continue;
      }
      if (game->player.level < 4) {
        printf("아직 은신 이탈을 익히지 못했습니다. (레벨 4 필요)\n");
        continue;
      }
      if (game->combat.enemy.boss) {
        printf("이 전투에서는 도망칠 수 없습니다.\n");
        continue;
      }
      {
        int self_dmg = roll_range(2, 6);
        game->player.hp = clamp_int(game->player.hp - self_dmg, 0, game->player.max_hp);
        printf("연기 속에서 사라지며 전투를 이탈합니다. (%d 피해 감수)\n", self_dmg);
      }
      advance_time(game, 5);
      flush_events(game);
      game->combat.active = false;
      if (tui) tui->combat_mode = false;
      return BATTLE_RESULT_FLED;
    }
    /* ---- Mage abilities ---- */
    else if (strcmp(command, "fireball") == 0) {
      if (game->player.player_class != CLASS_MAGE) {
        printf("마법사 전용 기술입니다.\n");
        continue;
      }
      if (game->player.rune_shards <= 0) {
        printf("룬 파편이 없습니다.\n");
        continue;
      }
      game->player.rune_shards--;
      player_damage = 18 + game->player.level * 4 + roll_range(0, 5);
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("화염구가 %s을(를) 강타해 %d 마법 피해를 입혔습니다.\n",
             game->combat.enemy.name, player_damage);
      if (rand() % 100 < 40) {
        game->combat.enemy_burn_turns = (game->combat.enemy_burn_turns > 0)
                                            ? game->combat.enemy_burn_turns + 1
                                            : 2;
        printf("%s이(가) 화염에 휩싸였습니다!\n", game->combat.enemy.name);
      }
      spend_turn = true;
    } else if (strcmp(command, "frost") == 0) {
      if (game->player.player_class != CLASS_MAGE) {
        printf("마법사 전용 기술입니다.\n");
        continue;
      }
      if (game->player.level < 3) {
        printf("아직 냉기 화살을 익히지 못했습니다. (레벨 3 필요)\n");
        continue;
      }
      if (game->player.rune_shards <= 0) {
        printf("룬 파편이 없습니다.\n");
        continue;
      }
      game->player.rune_shards--;
      player_damage = 12 + game->player.level * 3;
      game->combat.enemy_stun_turns = (game->combat.enemy_stun_turns > 0)
                                          ? game->combat.enemy_stun_turns + 1
                                          : 2;
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                        game->combat.enemy.max_hp);
      printf("냉기 화살이 %s을(를) 관통해 %d 피해를 주고 동결시켰습니다!\n",
             game->combat.enemy.name, player_damage);
      spend_turn = true;
    }
    /* ---- Cleric abilities ---- */
    else if (strcmp(command, "smite") == 0) {
      if (game->player.player_class != CLASS_CLERIC) {
        printf("성직자 전용 기술입니다.\n");
        continue;
      }
      {
        int attack = player_attack_value(game) + 3;
        int half_defense = game->combat.enemy.defense / 2;
        if (game->combat.weaken_turns > 0) {
          attack -= 2;
        }
        player_damage = compute_damage(attack, half_defense);
        /* Ash weather: caster enemies deal extra, but smite ignores that */
        game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - player_damage, 0,
                                          game->combat.enemy.max_hp);
        int self_heal = player_damage / 3;
        game->player.hp = clamp_int(game->player.hp + self_heal, 0,
                                    game->player.max_hp);
        printf("신성한 빛이 %s을(를) 강타해 %d 피해를 입히고, "
               "신성력으로 체력 %d를 회복했습니다.\n",
               game->combat.enemy.name, player_damage, self_heal);
      }
      spend_turn = true;
    } else if (strcmp(command, "holy_barrier") == 0 ||
               strcmp(command, "holy barrier") == 0) {
      if (game->player.player_class != CLASS_CLERIC) {
        printf("성직자 전용 기술입니다.\n");
        continue;
      }
      if (game->player.level < 4) {
        printf("아직 신성 방벽을 익히지 못했습니다. (레벨 4 필요)\n");
        continue;
      }
      game->combat.holy_barrier_active = true;
      game->combat.guard_active = false;
      game->combat.parry_active = false;
      printf("신성한 빛의 방벽을 펼칩니다. 다음 공격을 완전히 차단하고 체력을 회복합니다.\n");
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
    /* ---- Process enemy burn ---- */
    if (game->combat.enemy_burn_turns > 0) {
      int burn_dmg = 3;
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - burn_dmg, 0,
                                        game->combat.enemy.max_hp);
      game->combat.enemy_burn_turns--;
      printf("%s이(가) 화염에 타오릅니다. %d 피해.\n", game->combat.enemy.name,
             burn_dmg);
      if (game->combat.enemy.hp <= 0) {
        break;
      }
    }
    /* ---- Process player bleed ---- */
    if (game->combat.player_bleed_turns > 0) {
      int bleed_dmg = 2;
      game->player.hp = clamp_int(game->player.hp - bleed_dmg, 0,
                                  game->player.max_hp);
      game->combat.player_bleed_turns--;
      printf("출혈로 %d 피해를 받았습니다. (체력 %d/%d)\n", bleed_dmg,
             game->player.hp, game->player.max_hp);
      if (game->player.hp <= 0) {
        printf("%s이(가) 쓰러지고 길은 어둠에 잠깁니다.", game->player.name);
        game->running = false;
        game->combat.active = false;
        if (tui) tui->combat_mode = false;
        return BATTLE_RESULT_DEFEAT;
      }
    }
    /* ---- Enemy attack ---- */
    if (game->combat.enemy_stun_turns > 0) {
      game->combat.enemy_stun_turns--;
      printf("%s이(가) 기절해 공격하지 못합니다.\n", game->combat.enemy.name);
      enemy_damage = 0;
    } else if (game->combat.enemy_charging) {
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
    } else if (game->combat.enemy.burns_player && rand() % 100 < 30) {
      enemy_damage = compute_damage(game->combat.enemy.attack,
                                    player_defense_value(game));
      /* Burn causes damage over time; reuse player_bleed_turns to track DoT ticks */
      game->combat.player_bleed_turns = 2;
      printf("%s이(가) 불꽃을 내뿜습니다! 화상으로 계속 피해를 입습니다.\n",
             game->combat.enemy.name);
    } else if (game->combat.enemy.bleeds_player && rand() % 100 < 30) {
      enemy_damage = compute_damage(game->combat.enemy.attack,
                                    player_defense_value(game));
      game->combat.player_bleed_turns = 2;
      printf("%s이(가) 날카로운 일격으로 상처를 냅니다!\n", game->combat.enemy.name);
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
        /* Ash weather: caster enemies deal +2 extra damage */
        if (game->weather == WEATHER_ASH) {
          enemy_damage += 2;
          printf("%s이(가) 재 연기를 틈타 강한 마법을 날립니다.\n",
                 game->combat.enemy.name);
        } else {
          printf("%s이(가) 악의적인 마법을 퍼붓습니다.\n", game->combat.enemy.name);
        }
      } else {
        /* Wind weather: 20% chance of enemy attack missing */
        if (game->weather == WEATHER_WIND && rand() % 100 < 20) {
          enemy_damage = 0;
          printf("강한 돌풍이 %s의 공격을 빗나가게 합니다!\n",
                 game->combat.enemy.name);
        } else {
          printf("%s이(가) 재빠르게 공격합니다.\n", game->combat.enemy.name);
        }
      }
    }
    /* Storm weather: all attacks reduced by 2 (minimum 1) */
    if (game->weather == WEATHER_STORM && enemy_damage > 1) {
      enemy_damage -= 2;
      if (enemy_damage < 1) {
        enemy_damage = 1;
      }
    }
    /* ---- Apply block mechanics (warrior parry / cleric holy_barrier) ---- */
    if (game->combat.parry_active && enemy_damage > 0) {
      int counter = enemy_damage / 4;
      /* Ceiling division: parry reduces incoming damage to ~25% (rounded up),
         ensuring at least 1 damage if the attack was non-zero */
      enemy_damage = (enemy_damage + 3) / 4;
      game->combat.enemy.hp = clamp_int(game->combat.enemy.hp - counter, 0,
                                        game->combat.enemy.max_hp);
      printf("막기 성공! 대부분의 피해를 흡수하고 %s에게 %d를 반격했습니다.\n",
             game->combat.enemy.name, counter);
      game->combat.parry_active = false;
    } else if (game->combat.holy_barrier_active && enemy_damage > 0) {
      int heal = 10;
      enemy_damage = 0;
      game->player.hp = clamp_int(game->player.hp + heal, 0, game->player.max_hp);
      printf("신성 방벽이 공격을 완전히 차단하고 체력 %d를 회복시킵니다.\n", heal);
      game->combat.holy_barrier_active = false;
    } else if (game->combat.guard_active && enemy_damage > 0) {
      enemy_damage = (enemy_damage + 1) / 2;
      printf("방어로 피해 일부를 흡수했습니다.\n");
      game->combat.guard_active = false;
    } else {
      game->combat.parry_active = false;
      game->combat.holy_barrier_active = false;
      game->combat.guard_active = false;
    }
    if (enemy_damage > 0) {
      game->player.hp = clamp_int(game->player.hp - enemy_damage, 0,
                                  game->player.max_hp);
      printf("%d 피해를 받았습니다. (체력 %d/%d)\n", enemy_damage,
             game->player.hp, game->player.max_hp);
    }
    if (game->combat.weaken_turns > 0) {
      game->combat.weaken_turns--;
    }
    advance_time(game, 5);
    flush_events(game);
    /* Brief pause so the player can read the enemy's action result */
    if (tui && tui->initialized) {
      tui_draw_combat_overlay(tui, game);
      struct timespec ts = {0, 400000000L}; /* 400 ms */
      nanosleep(&ts, NULL);
    }
  }
  game->combat.active = false;
  if (tui) tui->combat_mode = false;
  if (game->player.hp <= 0) {
    printf("%s이(가) 쓰러지고 길은 어둠에 잠깁니다.", game->player.name);
    game->running = false;
    return BATTLE_RESULT_DEFEAT;
  }
  printf("%s이(가) 쓰러졌습니다.", game->combat.enemy.name);
  if (game->combat.enemy.is_elite) {
    printf("★ 정예 적 격파! 추가 보상을 받았습니다. ★");
  }
  award_post_battle_loot(game, &game->combat.enemy, game->player.zone);
  return BATTLE_RESULT_VICTORY;
}
