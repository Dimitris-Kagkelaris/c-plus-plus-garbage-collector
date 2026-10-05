cmake_minimum_required(VERSION 3.21...4.1)
project(cplusplusgc LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cplusplusgc src/collector.cpp)

target_include_directories(cplusplusgc PUBLIC . include)

target_compile_features(cplusplusgc PUBLIC cxx_std_17)
set(CPLUSPLUSGC_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-align)
target_compile_options(cplusplusgc PRIVATE ${CPLUSPLUSGC_WARNINGS})

option(CPLUSPLUSGC_BUILD_TESTS "build the tests" ${PROJECT_IS_TOP_LEVEL})
if(CPLUSPLUSGC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(CPLUSPLUSGC_SANITIZE "add sanitizers" OFF)
if(CPLUSPLUSGC_SANITIZE)
    target_compile_options(cplusplusgc PUBLIC -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(cplusplusgc PUBLIC -fsanitize=address,undefined)
endif()
