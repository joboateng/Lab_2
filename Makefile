CC = gcc
CFLAGS = -Wall -Wextra -Wunused-parameter -Wunused-variable

all: oss worker

oss: oss.o
	$(CC) $(CFLAGS) -o oss oss.o 2>/dev/null

worker: worker.o
	$(CC) $(CFLAGS) -o worker worker.o 2>/dev/null

oss.o: oss.c
	$(CC) $(CFLAGS) -c oss.c 2>/dev/null

worker.o: worker.c
	$(CC) $(CFLAGS) -c worker.c 2>/dev/null

clean:
	rm -f oss worker *.o
