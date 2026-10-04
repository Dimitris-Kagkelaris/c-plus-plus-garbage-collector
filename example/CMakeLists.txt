cmake_minimum_required(VERSION 3.20)
project(C++GC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(.. GC)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE GC)
