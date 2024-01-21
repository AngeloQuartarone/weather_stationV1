SHELL=/bin/bash
CC=gcc
CFLAGS=-Wall -Wextra -pedantic -g -pthread -lm
VFLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes -s
.SILENT:
.PHONY: run

default: server

server: ./src/server.o ./lib/zambretti.o ./lib/circularQueue.o
	echo
	echo "- Compiling files..."
	$(CC) $(CFLAGS) $^ -o $@ -lm
	echo "- Compiling done!"
	echo 

run: server
	nohup ./server &
	nohup python3 client.py &

test: server
	./server

clean: 
	echo
	echo "- Removing files..."
	rm server ./lib/*.o ./src/*.o ./nohup.out ./log/*.log
	echo "- All clean!"
	echo

stop:
	pkill --signal 9 server && pkill --signal 9 -f client.py


server.o: src/server.c src/lib/zambretti.h src/lib/circularQueue.h
zambretti.o: src/lib/zambretti.c src/lib/zambretti.h
circularQueue.o: src/lib/circularQueue.c src/lib/circularQueue.h
