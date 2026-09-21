CXX = g++-16
CXXFLAGS = -Wall -Wextra

.PHONY: all

debug: CXXFLAGS += -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
debug: all

all: collector

collector: collector.o root.o
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf *.o collector