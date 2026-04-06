#define _POSIX_C_SOURCE 200809L
#include "game_shared.h"

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
  /* Class selection */
  printf("\n직업을 선택하세요:\n");
  printf("  1. 전사  - 높은 체력/방어, parry(막기), bash(방패 강타) 특기\n");
  printf("  2. 척후  - 빠른 공격/이탈, backstab(기습), vanish(은신) 특기\n");
  printf("  3. 마법사 - 마력 공격, fireball(화염구), frost(냉기 화살) 특기\n");
  printf("  4. 성직자 - 치유 중심, smite(신성 피해+자가치유), holy_barrier(Lv4) 특기\n");
  if (read_command("선택 (1/2/3/4, 엔터=전사): ", input, sizeof(input))) {
    canonicalize_input(input, command, sizeof(command));
    if (strcmp(command, "2") == 0) {
      select_class(&game, CLASS_SCOUT);
      printf("척후 선택. 빠른 발과 날카로운 눈으로 길을 개척하십시오.\n");
    } else if (strcmp(command, "3") == 0) {
      select_class(&game, CLASS_MAGE);
      printf("마법사 선택. 아는 것이 힘입니다. 룬 파편으로 화염구와 냉기 화살을 사용하세요.\n");
    } else if (strcmp(command, "4") == 0) {
      select_class(&game, CLASS_CLERIC);
      printf("성직자 선택. 신성한 빛이 어둠을 밝힙니다. smite로 피해를 주고 스스로를 치유하세요.\n");
    } else {
      select_class(&game, CLASS_WARRIOR);
      printf("전사 선택. 강철이 말하게 하십시오.\n");
    }
  }
  if (read_command("수호자의 이름을 입력하세요 (빈칸이면 수호자): ", input,
                   sizeof(input)) &&
      !is_blank(input)) {
    snprintf(game.player.name, sizeof(game.player.name), "%s", input);
  }
  printf("\n%s이(가) 남부 순찰대의 불빛이 희미해지는 저녁, 엠버폴 관문에 도착했습니다.\n",
         game.player.name);
  describe_zone(&game);
  show_help(&game);
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
      show_help(&game);
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
    } else if (strcmp(command, "use holy water") == 0 ||
               strcmp(command, "holy water") == 0) {
      use_holy_water_outside_combat(&game);
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
    /* Multiple endings based on doom level and completed quests */
    if (game.doom <= 2) {
      printf("\n★ 찬란한 승리 ★\n");
      printf("공허의 왕관이 산산이 부서지고 어둠이 왕국 땅 위에서 물러납니다. "
             "가장 적은 희생으로 왕국을 지켜낸 수호자여, 당신의 이름은 돌에 새겨질 "
             "것입니다. 생존자들이 다시 길을 걷기 시작합니다.\n");
    } else if (game.doom <= 6) {
      printf("\n◆ 수호자의 승리 ◆\n");
      printf("왕관은 부서졌습니다. 그러나 전투의 흔적은 깊습니다. "
             "왕국은 살아남았지만, 상처를 회복하는 데 오랜 시간이 걸릴 것입니다. "
             "당신의 희생은 잊히지 않습니다.\n");
    } else {
      printf("\n▲ 피루스적 승리 ▲\n");
      printf("왕관은 부서졌지만, 공허의 그림자가 너무 깊이 스며들었습니다. "
             "왕국의 형태는 남아 있지만, 회복에는 세대가 걸릴 것입니다. "
             "그래도, 당신은 해냈습니다.\n");
    }
    /* Bonus epilogue based on completed side content */
    if (game.beacon_lit && game.doom <= 4) {
      printf("\n[에필로그] 고대 봉화의 빛이 다시 한 번 남쪽 하늘을 물들입니다. "
             "생존자들이 빛을 향해 모여듭니다. 새로운 마을이 그 자리에 생겨날 것입니다.\n");
    }
    if (game.druid_quest == QUEST_COMPLETE) {
      printf("\n[에필로그] 심숲 분지의 숲이 다시 깊고 조용한 녹색으로 돌아갑니다. "
             "에이브가 나무 사이에서 웃으며 손을 흔듭니다.\n");
    }
    if (game.shore_quest == QUEST_COMPLETE) {
      printf("\n[에필로그] 메아리 해안의 파도 소리가 맑아졌습니다. "
             "레나가 해안 동굴 입구에 서서 새벽빛 속 수평선을 바라봅니다.\n");
    }
    if (game.citadel_quest == QUEST_COMPLETE) {
      printf("\n[에필로그] 부서진 성채에 다시 불이 켜집니다. "
             "왕국의 마지막 대장간에서 새로운 강철이 단련되기 시작했습니다.\n");
    }
    printf("\n승리했습니다. 길은 여전히 수호자를 필요로 하지만, 더 이상 공허의 왕관에 "
           "지배당하지 않습니다.\n");
  } else if (!game.running && game.player.hp <= 0) {
    printf("게임 오버.\n");
  }
  shutdown_game(&game);
  return 0;
}
