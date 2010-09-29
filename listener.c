#include "listener.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>  // sockaddr
#include <netdb.h>       // addrinfo
#include <arpa/inet.h>   // in_addr, inet_ntop()
#include <pthread.h>

#include "session.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int listener(char* service) {
	
	int sockfd, new_fd;
	struct addrinfo hints, *servinfo;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, service, &hints, &servinfo);
	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	int on = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	freeaddrinfo(servinfo);

	listen(sockfd, BACKLOG);
	printf("listener: waiting for connections...\n");

	int epfd = epoll_create(16);
	struct epoll_event listen_event;
	listen_event.data.fd = sockfd;
	listen_event.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &listen_event);
	struct epoll_event* caught_events = calloc(16, sizeof(struct epoll_event));
	for (;;) {
		int nfds = epoll_wait(epfd, caught_events, 1, -1);
		int i;
		for (i = 0; i < nfds; i++) {
			if (caught_events[i].data.fd == sockfd) {
				sin_size = sizeof their_addr;
				new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
				inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
				printf("listener: got connection from %s\n", s);
				pthread_t session_thread;
				struct session_data * data = malloc(sizeof(struct session_data));
				data->sockfd = new_fd;
				data->state = SESSION_NEW;
				pthread_create(&session_thread, NULL, session_worker, data);
				printf("listener: initiated a new session worker\n");
			}
		}
	}
	free(caught_events);
	printf("listener: finished\n");
	return 0;
}
