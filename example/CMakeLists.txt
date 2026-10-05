cmake_minimum_required(VERSION 3.21...4.1)
project(cplusplusgc_example LANGUAGES CXX)

add_subdirectory(.. cplusplusgc_build)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE cplusplusgc)
