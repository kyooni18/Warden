cmake_minimum_required(VERSION 3.20)

if(APPLE AND (NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL ""))
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures to build for on macOS" FORCE)
endif()

project(CLITextGame VERSION 0.1.0 LANGUAGES C)

add_subdirectory(Feather)

add_executable(cli_text_rpg
    RPG/main.c
)

target_link_libraries(cli_text_rpg PRIVATE Feather::Feather)

set_target_properties(cli_text_rpg PROPERTIES
    OUTPUT_NAME CLITextRPG
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
