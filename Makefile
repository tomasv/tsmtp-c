CFLAGS=-g

all: server client

server: server.c

client: client.c

clean:
	-rm server.out client.out
