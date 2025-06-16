
include config.mk

all: build/connectors-game

SRCS =\
	main

OBJS = $(patsubst %,build/%.o,$(SRCS))

build/connectors-game: $(OBJS)
	@mkdir -p $(@D)
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) -o $@ -c -MMD $(CXXFLAGS) $<

clean:
	rm -rf build

.PHONY: clean

