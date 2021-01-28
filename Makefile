CFLAGS=-Wall -Wextra -pedantic -std=c99

ircmerge: ircmerge.o
	$(CC) ircmerge.o -o ircmerge

ircmerge.o: ircmerge.c
	$(CC) $(CFLAGS) -c ircmerge.c -o ircmerge.o

clean:
	$(RM) *.o ircmerge
