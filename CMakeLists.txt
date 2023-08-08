cmake_minimum_required(VERSION 3.12)
project(babchess VERSION 0.1.0)

set(SRCS
    src/bitboard.cpp
    src/engine.cpp
    src/main.cpp
    src/move.cpp
    src/perft.cpp
    src/position.cpp
    src/test.cpp
    src/uci_option.cpp
    src/uci.cpp
    )

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CXX_STANDARD_REQUIRED 17)


add_compile_options(-Wall -fno-exceptions -mbmi -mbmi2 -mpopcnt)
add_link_options(-Wall -fno-exceptions -mbmi -mbmi2 -mpopcnt)

#add_compile_options(-pg -no-pie)
#add_link_options(-pg -no-pie)

add_executable(babchess ${SRCS})