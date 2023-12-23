.POSIX:
.SUFFIXES:

CC = gcc -Wall -pedantic
CFLAGS = -O2 -march=native -ggdb3

PROGS =\
	hex_stats\
# 	huff_gen\

all: $(PROGS)

$(PROGS):
	$(CC) -o $@ $(CFLAGS) $(@:=.c)

hex_stats: hex_stats.c
# huff_gen: huff_gen.c

clean:
	rm -rf $(PROGS)

.PHONY: all clean
