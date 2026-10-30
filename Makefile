# Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
# Autor: Andrej Beles (xbelesa00)

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -g
TARGET = compiler
SRCS = main.c scanner.c stack.c symtable.c

build: $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o