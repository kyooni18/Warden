cmake_minimum_required(VERSION 3.20)

if(APPLE AND (NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL ""))
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures to build for on macOS" FORCE)
endif()

project(Warden VERSION 0.1.0 LANGUAGES C)

add_subdirectory(Feather)

# ncurses (wide-character variant for proper UTF-8 / Korean support)
find_package(Curses REQUIRED)

add_executable(warden
    RPG/main.c
    RPG/game.c
    RPG/game_utils.c
    RPG/game_state.c
    RPG/game_ui.c
    RPG/game_combat.c
    RPG/game_actions.c
    RPG/game_world.c
    RPG/game_tui.c
)

target_include_directories(warden PRIVATE ${CURSES_INCLUDE_DIR})
target_link_libraries(warden PRIVATE Feather::Feather ${CURSES_LIBRARIES})

set_target_properties(warden PROPERTIES
    OUTPUT_NAME Warden
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
