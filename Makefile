CC=gcc
CFLAGS= -Wall -g -I.
DEPS = crawler.h
OBJ = crawler.o crawlerfunc.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

crawler: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) crawler.o
	$(RM) crawlerfunc.o
