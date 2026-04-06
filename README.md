# CLITextGame

이 저장소는 Feather 라이브러리를 연결해 동작하는 독립형 텍스트 RPG 애플리케이션을 포함합니다.

## 구조

- `Feather/`에는 재사용 가능한 스케줄러 라이브러리가 있습니다.
- `RPG/main.c`는 얇은 실행 진입점입니다.
- `RPG/game.c`는 게임 애플리케이션 로직을 담고 있습니다.
- `RPG/game.h`는 게임 실행 인터페이스를 선언합니다.

## 빌드

```sh
make rpg
```

## 실행

```sh
make run-rpg
```

실행 파일은 `build/Warden` 경로에 생성됩니다.

## 참고

- 게임의 핵심 UI 안내/메시지는 한국어로 표시됩니다.
- 게임 내 명령은 저장/불러오기를 지원합니다.
  - `save`: 진행 상황을 `savegame.dat`에 저장
  - `load`: `savegame.dat`에서 진행 상황 복원
