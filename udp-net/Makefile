
CC = gcc
CFLAGS = -Wall
CFLAGS_CLIENT = $(CFLAGS) $(shell sdl2-config --cflags)
CFLAGS_SERVER = $(CFLAGS)
LDFLAGS = -lpthread
LDFLAGS_CLIENT = $(LDFLAGS) $(shell sdl2-config --libs) -lm
LDFLAGS_SERVER = $(LDFLAGS)

all: build/server build/client

build/server: server.c common.h
	@mkdir -p $(@D)
	$(CC) -o $@ $(CFLAGS_SERVER) $< $(LDFLAGS_SERVER)

build/client: client.c common.h
	@mkdir -p $(@D)
	$(CC) -o $@ $(CFLAGS_CLIENT) $< $(LDFLAGS_CLIENT)

clean:
	rm -rdf build

.PHONY: all clean

