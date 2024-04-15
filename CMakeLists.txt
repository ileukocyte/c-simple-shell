cmake_minimum_required(VERSION 3.28)
project(spaasm2 C)

set(CMAKE_C_STANDARD 17)

add_executable(spaasm2 main.c
        utils.c)
