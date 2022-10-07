cmake_minimum_required(VERSION 3.19)

project(Chip8 C)

set(CMAKE_C_STANDARD 17)

add_subdirectory(libs/SDL)

add_subdirectory(src)

target_link_libraries(Chip8
    PRIVATE SDL2::SDL2 SDL2::SDL2main
)