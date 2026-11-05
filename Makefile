CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I. -Igrid_code
LDFLAGS =
LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

.PHONY: all clean

all: main

main: grid_code/grid.c grid_code/grid.h
	$(CC) $(CFLAGS) $(LDFLAGS) grid_code/grid.c -o main $(LDLIBS)

clean:
	rm -f main


