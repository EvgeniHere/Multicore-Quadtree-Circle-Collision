cmake_minimum_required(VERSION 3.25)
project(HPC C)

set(CMAKE_C_STANDARD 11)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_library(GLU_LIBRARY GLU)
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
find_package(Threads REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

add_executable(HPC main.c)

target_link_libraries(HPC m OpenGL::GL GLUT::GLUT ${OPENGL_LIBRARIES} ${GLU_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} OpenMP::OpenMP_C MPI::MPI_C)
