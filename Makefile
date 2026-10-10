CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
TARGET = compiler

build: main.c scanner.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c scanner.c

clean:
	rm -f $(TARGET)