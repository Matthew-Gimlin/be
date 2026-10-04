CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O0 -ggdb
SRCS = $(wildcard src/*.c)
EXEC = be

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(EXEC)

clean:
	rm -rf $(EXEC)
