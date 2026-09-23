CXX = g++-16
CXXFLAGS = -Wall -Wextra -Wsign-conversion

.PHONY: all debug

debug: CXXFLAGS += -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
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

doctest.o: doctest.cpp
	$(CXX) -Wall -Wextra -O1 -c $< -o $@

clean:
	rm -f *.o main tests