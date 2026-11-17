# Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
# Subor: Makefile
# Autor: Andrej Beles (xbelesa00)
#
# AI Prompt: "Pridaj do Makefile komentar, ako po dokonceni parsera pridat
# parser.c do zoznamu zdrojovych suborov, a oznac ciele build a clean ako .PHONY."

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -g
TARGET = compiler
# Po dokonceni parsera (a vypnuti TEST_SCANNER v main.c) pridat: parser.c
# Neskor pridat aj: expressions.c, generator.c, hashtable.c
SRCS = main.c scanner.c stack.c symtable.c parser.c expressions.c

build: $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o

.PHONY: build clean
