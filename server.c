/*
 ** server.c -- a stream socket server demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;
	struct addrinfo hints, *servinfo;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	getaddrinfo(NULL, PORT, &hints, &servinfo);

	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

	freeaddrinfo(servinfo); // all done with this structure

	listen(sockfd, BACKLOG);

	printf("server: waiting for connections...\n");
	int clients[16], i;
	for (i = 0; i < 16; i++) {
		clients[i] = -1;
	}

	int epfd = epoll_create(16);
	struct epoll_event listen_event;
	listen_event.data.fd = sockfd;
	listen_event.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &listen_event);
	int n_clients = 0;
	while(1) {  // main accept() loop
		struct epoll_event* caught_events = calloc(16, sizeof(struct epoll_event));

		int nfds = epoll_wait(epfd, caught_events, 10, -1);
		printf("%d nfds\n", nfds);

		for (i = 0; i < nfds; i++) {
			if (caught_events[i].data.fd == sockfd) {
				// we are receiving new connections
				printf("server: accepting new connection");
				sin_size = sizeof their_addr;
				new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
				clients[n_clients++] = new_fd;
				inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
				printf("server: got connection from %s\n", s);
				if (send(new_fd, "Hello, world!", 13, 0) == -1)
					perror("send");
				int j;
				for (j = 0; j < 16; j++) {
					if ((clients[j] > -1) && (clients[j] != new_fd)) {
						send(clients[j], "We have another visitor :)\n", 28, 0);
					}
				}
			} else
			{
				//send(caught_events[i].data.fd, "a", 2, 0);
				// we are handling received connections
			}
		}
	}

	return 0;
}
