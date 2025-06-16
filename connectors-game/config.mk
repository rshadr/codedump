CXX = g++ -std=c++23
CXXFLAGS = -O2 -ggdb3 -Wall -Wextra -pedantic

LIBS = -lSDL3 -lvulkan
LDFLAGS = $(LIBS)
