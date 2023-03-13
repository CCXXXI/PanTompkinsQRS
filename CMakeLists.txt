cmake_minimum_required(VERSION 3.24)
project(PanTompkinsQRS)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

add_executable(PanTompkinsQRS src/panTompkins.cpp src/panTompkins.h)
