
cmake_minimum_required(VERSION 3.21)

project(PacmanTerminal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pacman
    src/main.cpp
    src/Game.cpp
    src/Terminal.cpp
)

target_include_directories(pacman PRIVATE include)

target_compile_options(pacman PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
