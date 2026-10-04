cmake_minimum_required(VERSION 3.21)
project(cppgc LANGUAGES CXX)

add_library(cppgc src/collector.cpp)

target_include_directories(cppgc PUBLIC . include)

target_compile_features(cppgc PUBLIC cxx_std_17)
target_compile_options(cppgc PRIVATE -Wall -Wextra -Wsign-conversion)

option(BUILD_TESTS "build the tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
