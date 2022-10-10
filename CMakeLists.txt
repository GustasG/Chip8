cmake_minimum_required(VERSION 3.19)

project(Chip8
    VERSION 1.0
    DESCRIPTION "CHIP-8 emulator"
    HOMEPAGE_URL "https://github.com/GustasG/Chip8"
    LANGUAGES C
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_C_STANDARD 17)

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})