cmake_minimum_required(VERSION 2.8)
project(HPC C)

set(CMAKE_C_STANDARD 11)

find_package(Scorep OPTIONAL_COMPONENTS)

if(Scorep_FOUND)
    set(CMAKE_C_COMPILER ${Scorep_C_COMPILER})
    set(CMAKE_CXX_COMPILER ${Scorep_CXX_COMPILER})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${Scorep_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Scorep_CXX_FLAGS}")
endif()

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

add_executable(HPC main.c)

target_link_libraries(HPC m OpenMP::OpenMP_C MPI::MPI_C)
