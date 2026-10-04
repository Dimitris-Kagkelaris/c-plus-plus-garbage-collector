cmake_minimum_required(VERSION 3.20)
project(C++GC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(GC src/collector.cpp)

target_include_directories(GC PUBLIC . include)

target_compile_options(GC PRIVATE -Wall -Wextra -Wsign-conversion)
target_compile_features(GC PUBLIC cxx_std_17)
