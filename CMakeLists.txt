cmake_minimum_required(VERSION 3.25)
project(HPC C)

set(CMAKE_C_STANDARD 11)

add_executable(HPC main.c)

target_link_libraries(HPC m)
