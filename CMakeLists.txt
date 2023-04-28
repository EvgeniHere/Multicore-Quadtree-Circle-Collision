cmake_minimum_required(VERSION 3.25)
project(HPC C)

set(CMAKE_C_STANDARD 11)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
find_package(MPI REQUIRED)

add_executable(HPC main.c)

target_link_libraries(HPC PUBLIC m MPI::MPI_C)


