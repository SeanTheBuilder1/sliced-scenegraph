cmake_minimum_required(VERSION "3.0")
set(CMAKE_CXX_FLAGS "-O3 -std=c++20")

project("scene_graph")
set(EXECUTABLE_OUTPUT_PATH ./)
add_executable(
    a
    implementation.cpp
)


