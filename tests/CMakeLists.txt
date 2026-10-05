add_executable(cppgc_tests doctest.cpp tests.cpp)
target_link_libraries(cppgc_tests PRIVATE cppgc)
target_compile_options(cppgc_tests PRIVATE -Wall -Wextra -Wsign-conversion)

add_test(NAME tests COMMAND cppgc_tests)
