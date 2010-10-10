CFLAGS=-g -Wall
LDFLAGS=-lpthread

all: server client

list_test: list.o list_test.c

server: listener.o sender.o session.o server.c

listener.o: session.o sender.o

sender.o: sender.c

session.o: session.c

client: client.c
