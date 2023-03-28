cmake_minimum_required(VERSION 3.24)
project(PanTompkinsQRS)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

add_executable(PanTompkinsQRS src/panTompkins.cpp src/panTompkins.h src/main.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(PanTompkinsQRS PRIVATE nlohmann_json::nlohmann_json)
