cmake_minimum_required(VERSION 3.19)

project(Chip8 C)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_C_STANDARD 17)

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})