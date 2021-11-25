CC=gcc
CFLAGS=-fsanitize=address -Wall -Werror -std=gnu11 -g -lm

tests: tests.c structure.c virtual_alloc.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf *.dSYM
	rm -f tests
