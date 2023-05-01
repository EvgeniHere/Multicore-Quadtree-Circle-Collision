cmake_minimum_required(VERSION 3.25)
project(HPC C)

set(CMAKE_C_STANDARD 11)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_library(GLU_LIBRARY GLU)

add_executable(HPC main.c)

target_link_libraries(HPC m)
target_link_libraries(HPC OpenGL::GL)
target_link_libraries(HPC GLUT::GLUT)
target_link_libraries(HPC ${OPENGL_LIBRARIES} ${GLU_LIBRARY})

