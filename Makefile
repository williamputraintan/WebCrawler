CC=gcc
CFLAGS=-I.

crawler: crawler.o
	$(CC) -o crawler crawler.o -I.
