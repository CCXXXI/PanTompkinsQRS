cmake_minimum_required(VERSION 3.24)
project(PanTompkinsQRS)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_PROFILE --coverage)
endif ()

add_executable(PanTompkinsQRS src/panTompkins.cpp src/panTompkins.h src/main.cpp)
add_executable(PanTompkinsQRS_tests src/panTompkins.cpp src/panTompkins.h tests/test_panTompkins.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(PanTompkinsQRS PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(PanTompkinsQRS_tests PRIVATE nlohmann_json::nlohmann_json)

find_package(Catch2 3 REQUIRED)
target_link_libraries(PanTompkinsQRS_tests PRIVATE Catch2::Catch2WithMain)
