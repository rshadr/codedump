CC = gcc -std=c23
CFLAGS = -O2 -ggdb3 -Wall -Wextra -pedantic
LDFLAGS = -lc

CFLAGS_CLIENT = $(CFLAGS)
CFLAGS_SERVER = $(CFLAGS)
LDFLAGS_CLIENT = $(LDFLAGS) -lSDL3 -lvulkan
LDFLAGS_SERVER = $(LDFLAGS)

