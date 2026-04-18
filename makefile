.PHONY : example clean
CC = gcc
CFLAGS = -Wall -Wextra -Werror
example: example
	$(CC) $(CFLAGS) -o example example.c
	./example

clean:
	rm -f example
valgrind: example
	$(CC) $(CFLAGS) -o example example.c
	valgrind -s --track-origins=yes --leak-check=full --log-file=valgrind.log ./example 