CFLAGS=-g -Wall
LDFLAGS=-lpthread

all: server client

list_test: list.o list_test.c

parser_test: parser.c
	$(CC) $(CFLAGS) -DPARSER_TEST $^ -o $@

server: listener.o sender.o session.o parser.o list.o server.c

listener.o: session.o sender.o

sender.o: sender.c

session.o: parser.o session.c

parser.o: list.o parser.c

list.o: list.c

client: parser.o list.o client.c
