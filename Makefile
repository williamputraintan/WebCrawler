CC=gcc
CFLAGS=-I.

crawler: crawler.c
	$(CC) -o crawler crawler.c -I.
