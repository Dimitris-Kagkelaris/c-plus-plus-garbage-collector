cmake_minimum_required(VERSION 3.21...4.1)
project(cppgc LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cppgc src/collector.cpp)

target_include_directories(cppgc PUBLIC . include)

target_compile_features(cppgc PUBLIC cxx_std_17)
set(CPPGC_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-align)
target_compile_options(cppgc PRIVATE ${CPPGC_WARNINGS})

option(CPPGC_BUILD_TESTS "build the tests" ${PROJECT_IS_TOP_LEVEL})
if(CPPGC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(CPPGC_SANITIZE "add sanitizers" OFF)
if(CPPGC_SANITIZE)
    target_compile_options(cppgc PUBLIC -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(cppgc PUBLIC -fsanitize=address,undefined)
endif()
