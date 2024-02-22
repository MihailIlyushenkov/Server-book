CC = gcc

all: compile

compile: hashtable.o telnumbers.o asyncsrv.o
	$(CC) objects/telnumbers.o objects/hashtable.o objects/asyncsrv.o -o main -luv

telnumbers.o:
	$(CC) -c sources/telnumbers.c -o objects/telnumbers.o

hashtable.o:
	$(CC) -c sources/hashtable.c -o objects/hashtable.o

asyncsrv.o:
	$(CC) -c sources/asyncsrv.c -o objects/asyncsrv.o

clean:
	rm objects/*
