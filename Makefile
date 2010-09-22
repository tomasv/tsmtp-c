CFLAGS=-g

all: server.out client.out

server.out: server.c

client.out: client.c

clean:
	-rm server.out client.out
