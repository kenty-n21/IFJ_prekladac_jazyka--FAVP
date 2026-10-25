CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
TARGET = compiler

build: main.c scanner.c parser.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c scanner.c parser.c

clean:
	rm -f $(TARGET)