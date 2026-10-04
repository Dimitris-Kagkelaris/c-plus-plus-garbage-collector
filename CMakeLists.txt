cmake_minimum_required(VERSION 3.20)
project(cppgc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cppgc src/collector.cpp)

target_include_directories(cppgc PUBLIC . include)

target_compile_options(cppgc PRIVATE -Wall -Wextra -Wsign-conversion)
target_compile_features(cppgc PUBLIC cxx_std_17)
