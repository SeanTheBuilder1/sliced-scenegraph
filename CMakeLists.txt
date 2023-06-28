cmake_minimum_required(VERSION "3.0")
set(CMAKE_CXX_FLAGS "-O3 -std=c++20")

project("scene_graph")
set(EXECUTABLE_OUTPUT_PATH ./)


include_directories(${CMAKE_SOURCE_DIR})

add_executable(
    a
    implementation.cpp
)

add_executable(
    b
    implementation2.cpp
)


