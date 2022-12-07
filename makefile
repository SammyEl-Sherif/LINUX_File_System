# Simple makefile for linux style file system
CC= g++

CFLAGS = -g -Wall # setup for system

all: filesystem

filesystem:	main.cpp
		$(CC) $(CFLAGS) main.cpp -o filesystem

disk: make-disk
	./make-disk mydisk0

clean:
	rm filesystem
