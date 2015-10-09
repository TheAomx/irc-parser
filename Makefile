CC?=gcc

CFLAGS += -Wall -Wextra -pedantic --std=c99 
CFLAGS_DEBUG = $(CFLAGS) -O0 -g
CFLAGS_FAST = $(CFLAGS) -O3
CFLAGS_LIB = $(CFLAGS_FAST) -fPIC

default: irc_parser.o

irc_parser.o:
	$(CC) $(CFLAGS_FAST) -c irc_parser.c -o $@

irc_parser_g.o:
	$(CC) $(CFLAGS_DEBUG) -c irc_parser.c -o $@

test: irc_parser_g.o
	$(CC) $(CFLAGS_DEBUG) test.c irc_parser_g.o -o $@
	./test

tags: irc_parser.c irc_parser.h test.c test.h
	etags $^

clean:
	rm -f *.o *.a test



.PHONY: clean test
