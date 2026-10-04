cmake_minimum_required(VERSION 3.21)
project(tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(.. cppgc_build)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE cppgc)
