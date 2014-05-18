# Compiler to use
CC=g++
CFLAGS=-std=c++11 -O2 -Wall -Wextra -DNDEBUG

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
	#Linux
	CLIBS=../libs/SiLog.lib ../libs/SiConf.lib -lboost_regex
endif
ifeq ($(UNAME), Darwin)
	#OS X
	CLIBS=../libs/SiLog.lib ../libs/SiConf.lib
endif

all: unix.o Movie.o Show.o Vid.o
	$(CC)  $(CFLAGS) $(CLIBS)  main.cpp unix.o Movie.o Show.o Vid.o -o main

test: unix.o Movie.o Show.o Vid.o tests.o
	$(CC)  $(CFLAGS) $(CLIBS) main.cpp tests.o unix.o Movie.o Show.o Vid.o -o test

tests.o: tests.cpp
	$(CC) $(CFLAGS) -c tests.cpp

unix.o: unix.cpp
	$(CC) $(CFLAGS) -c unix.cpp

Movie.o: Movie.cpp
	$(CC) $(CFLAGS) -c Movie.cpp

Show.o: Show.cpp
	$(CC) $(CFLAGS) -c Show.cpp

Vid.o: Vid.hpp
	$(CC) $(CFLAGS) -c Vid.cpp


clean:
	rm -rf *.o main test
