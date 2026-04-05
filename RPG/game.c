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
  if (read_command("선택 (1/2/3, 엔터=전사): ", input, sizeof(input))) {
    canonicalize_input(input, command, sizeof(command));
    if (strcmp(command, "2") == 0) {
      select_class(&game, CLASS_SCOUT);
      printf("척후 선택. 빠른 발과 날카로운 눈으로 길을 개척하십시오.\n");
    } else if (strcmp(command, "3") == 0) {
      select_class(&game, CLASS_MAGE);
      printf("마법사 선택. 아는 것이 힘입니다. 룬 파편으로 화염구와 냉기 화살을 사용하세요.\n");
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
