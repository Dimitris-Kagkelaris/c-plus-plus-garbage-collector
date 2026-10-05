add_executable(cplusplusgc_tests doctest.cpp tests.cpp)
target_link_libraries(cplusplusgc_tests PRIVATE cplusplusgc)
target_compile_options(cplusplusgc_tests PRIVATE ${CPLUSPLUSGC_WARNINGS})

add_test(NAME tests COMMAND cplusplusgc_tests)
