
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I.
LDFLAGS =
LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

.PHONY: all clean

all: main

main: grid.c grid.h
	$(CC) $(CFLAGS) $(LDFLAGS) grid.c -o main $(LDLIBS)

clean:
	rm -f main


