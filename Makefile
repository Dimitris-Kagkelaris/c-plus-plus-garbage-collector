CXX = g++-16
CXXFLAGS = -Wall -Wextra -Wsign-conversion -std=c++17

.PHONY: all debug leakcheck clean

debug: CXXFLAGS += -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
debug: all

all: tests main

leakcheck: CXXFLAGS += -g -O0
leakcheck: clean all
	MallocStackLogging=1 leaks --atExit -- ./tests

tests: doctest.o tests.o collector.o root.o
	$(CXX) $(CXXFLAGS) $^ -o $@

main: main.o collector.o root.o
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tests.o collector.o root.o main.o: collector.hpp types.hpp
tests.o collector.o root.o main.o: root.hpp

doctest.o: doctest.cpp
	$(CXX) -Wall -Wextra -std=c++17 -O1 -c $< -o $@

clean:
	rm -f *.o main tests