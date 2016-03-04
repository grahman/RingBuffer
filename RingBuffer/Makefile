CC = g++
LDFLAGS = -lrt

build: clean main

debug: CXXFLAGS = -g -Wall -DDEBUG
debug: clean main

main: main.o

main.o: main.cpp rbuf.h rbuf.hpp

.PHONY: clean

clean:
	rm -f *.o
	rm -f main

.PHONY: all

all: clean main

