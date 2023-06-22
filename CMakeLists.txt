cmake_minimum_required(VERSION 3.2)
project(HPC C)

set(CMAKE_C_STANDARD 11)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

add_executable(HPC main.c)

target_link_libraries(HPC m OpenMP::OpenMP_C MPI::MPI_C)
