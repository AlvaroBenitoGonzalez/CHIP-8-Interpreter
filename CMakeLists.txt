cmake_minimum_required(VERSION 3.16)
project(chip8 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Si no se indica otro tipo, compila optimizado
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SDL3 REQUIRED CONFIG)

add_executable(chip8 main.c chip8.c)
target_link_libraries(chip8 PRIVATE SDL3::SDL3)

if(NOT MSVC)
    target_compile_options(chip8 PRIVATE -Wall -Wextra)
endif()