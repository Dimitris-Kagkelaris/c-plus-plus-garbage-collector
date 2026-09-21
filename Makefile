CC = gcc
CFLAGS = -Wall -Wextra

TARGETS = frontend dispatcher worker

.PHONY: all release debug sleep-debug run run-rlwrap clean

debug: CFLAGS += -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -DDEBUG
debug: all

all: gc


gc: gc.o root.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf %.o