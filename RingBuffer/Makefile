CC = g++
CXXFLAGS = -g -Wall -DDEBUG
LDFLAGS = -lrt
main: main.o

main.o: main.cpp rbuf.h rbuf.hpp

.PHONY: clean

clean:
	rm -f *.o
	rm -f main

.PHONY: all

all: clean main

