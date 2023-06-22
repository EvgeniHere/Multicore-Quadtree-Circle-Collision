cmake_minimum_required(VERSION 2.8)
project(HPC C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_PREFIX_PATH "/opt/sw/rev/21.12/haswell/gcc-9.3.0/scorep-7.0-5z46kv" ${CMAKE_PREFIX_PATH})
set(Scorep_INCLUDE_DIRS "/opt/sw/rev/21.12/haswell/gcc-9.3.0/scorep-7.0-5z46kv/include")
set(Scorep_LIBRARIES "/opt/sw/rev/21.12/haswell/gcc-9.3.0/scorep-7.0-5z46kv/lib")
include_directories(${Scorep_INCLUDE_DIRS})

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

target_link_libraries(HPC m OpenMP::OpenMP_C MPI::MPI_C ${Scorep_LIBRARIES})
