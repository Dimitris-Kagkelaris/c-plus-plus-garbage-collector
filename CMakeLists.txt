cmake_minimum_required(VERSION 3.21...4.1)
project(cppgc LANGUAGES CXX)

add_library(cppgc src/collector.cpp)

target_include_directories(cppgc PUBLIC . include)

target_compile_features(cppgc PUBLIC cxx_std_17)
target_compile_options(cppgc PRIVATE -Wall -Wextra -Wsign-conversion)

option(OPTIMIZE "enable optimizations" ON)
if(OPTIMIZE)
    target_compile_options(cppgc PRIVATE -O3)
endif()

option(TESTS "build the tests" OFF)
if(TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(SANITIZE "add sanitizers" OFF)
if(SANITIZE)
    target_compile_options(cppgc PUBLIC -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(cppgc PUBLIC -fsanitize=address,undefined)
endif()
