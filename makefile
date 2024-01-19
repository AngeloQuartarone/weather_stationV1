SHELL=/bin/bash
CC=gcc
CFLAGS=-Wall -Wextra -pedantic -g -pthread -lm
VFLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes -s
.SILENT:
.PHONY: run

default: server

server: src/server.o src/lib/zambretti.o src/lib/circularQueue.o
	echo
	echo "- Compiling files..."
	$(CC) $(CFLAGS) $^ -o $@ -lm
	echo "- Compiling done!"
	echo 

run: server
	nohup ./server &

clean: 
	echo
	echo "- Removing files..."
	rm server ./src/lib/*.o ./src/*.o
	echo "- All clean!"
	echo


server.o: src/server.c src/lib/zambretti.h src/lib/circularQueue.h
zambretti.o: src/lib/zambretti.c src/lib/zambretti.h
circularQueue.o: src/lib/circularQueue.c src/lib/circularQueue.h
