# Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
# Subor: Makefile
# Autor: Andrej Beles (xbelesa00)
#
# AI Prompt: "Priprav robustny Makefile pre prekladac imperativneho jazyka IFJ26,
# ktory podporuje inkrementalnu kompilaciu cez objektove subory (.o) a automaticky
# prepoji scanner, parser, tabulku symbolov a generator cieloveho kodu."
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -g
TARGET = compiler

SRCS = main.c scanner.c stack.c symtable.c parser.c expressions.c generator.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Zlinkovanie vsetkych .o suborov do vysledneho programu
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pravidlo pre preklad jedneho .c suboru na jeden .o subor
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o

# Testy (pozri tests/Makefile a tests/run_tests.py)
test: $(TARGET)
	$(MAKE) -C tests test

.PHONY: all clean test