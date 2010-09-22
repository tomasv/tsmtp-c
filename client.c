/*
 ** client.c -- a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}

	// get address info
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(argv[1], PORT, &hints, &servinfo);

	// create socket, connect to it, print some connection info
	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// create epoll set with one listening socket
	int epfd = epoll_create(1);
	struct epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN | EPOLLHUP;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	struct epoll_event* events = calloc(1, sizeof(struct epoll_event));
	for (;;) {
		int nfds = epoll_wait(epfd, events, 1, -1);
		if (( numbytes = recv(events->data.fd, buf, MAXDATASIZE-1, 0) ) == 0)
			break; // connection closed on server side
		buf[numbytes] = '\0';
		printf("client: received '%s'\n", buf);
	}

	close(sockfd);
	return 0;
}
