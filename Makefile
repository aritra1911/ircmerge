CFLAGS=-Wall -Wextra -pedantic -g -O0 -std=c99

ircmerge: main.o mergedir.o mergelog.o
	$(CC) main.o mergedir.o mergelog.o -o ircmerge

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

mergedir.o: mergedir.c
	$(CC) $(CFLAGS) -c mergedir.c -o mergedir.o

mergelog.o: mergelog.c
	$(CC) $(CFLAGS) -c mergelog.c -o mergelog.o

clean:
	$(RM) *.o ircmerge
