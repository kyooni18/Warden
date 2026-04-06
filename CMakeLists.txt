cmake_minimum_required(VERSION 3.20)

if(APPLE AND (NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL ""))
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures to build for on macOS" FORCE)
endif()

project(Warden VERSION 0.1.0 LANGUAGES C)

# ---------------------------------------------------------------------------
# Feather submodule (required for the game executable, not for engine tests)
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/Feather/CMakeLists.txt")
    add_subdirectory(Feather)
    set(FEATHER_AVAILABLE TRUE)
else()
    message(WARNING
        "Feather submodule not found — run "
        "'git submodule update --init --recursive' to enable the full game build. "
        "Engine library and tests will still be built.")
    set(FEATHER_AVAILABLE FALSE)
endif()

# ---------------------------------------------------------------------------
# Engine library — pure utilities, no game state, no ncurses, no Feather.
# Compiled separately so the test target can link it without those deps.
# ---------------------------------------------------------------------------
add_library(warden_engine STATIC
    src/engine/utils.c
    src/engine/core_loop.c
    src/engine/audio.c
    src/engine/resource.c
)
target_include_directories(warden_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(warden_engine PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

# ---------------------------------------------------------------------------
# Main game executable (requires Feather submodule + ncurses)
# ---------------------------------------------------------------------------
if(FEATHER_AVAILABLE)
    find_package(Curses REQUIRED)

    add_executable(warden
        src/main.c
        src/game/game.c
        src/game/state.c
        src/game/ui.c
        src/game/combat.c
        src/game/actions.c
        src/game/world.c
        src/game/game_tui.c
    )

    target_include_directories(warden PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/game
        ${CURSES_INCLUDE_DIR}
    )
    target_link_libraries(warden PRIVATE warden_engine Feather::Feather ${CURSES_LIBRARIES})

    set_target_properties(warden PROPERTIES
        OUTPUT_NAME Warden
        C_STANDARD 99
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
else()
    message(STATUS "Skipping 'warden' target (Feather not available).")
endif()

# ---------------------------------------------------------------------------
# Unit tests — compile without Feather or ncurses
# ---------------------------------------------------------------------------
enable_testing()

add_executable(test_utils tests/test_utils.c)
target_link_libraries(test_utils PRIVATE warden_engine)
set_target_properties(test_utils PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
add_test(NAME test_utils COMMAND test_utils)
