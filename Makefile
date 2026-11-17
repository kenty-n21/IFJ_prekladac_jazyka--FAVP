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
# Po dokonceni parsera (a vypnuti TEST_SCANNER v main.c) pridat: parser.c
# Neskor pridat aj: expressions.c, generator.c, hashtable.c
SRCS = main.c scanner.c stack.c symtable.c parser.c expressions.c generator.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# 6. Zlinkovanie všetkých .o súborov do finálneho programu
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 7. Pravidlo pre preklad jedného .c súboru na jeden .o súbor
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
