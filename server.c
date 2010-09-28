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
//#include <signal.h>
#include <pthread.h>

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

enum session_state {
	SESSION_NEW,
	SESSION_GREET,
	SESSION_MAIL,
	SESSION_RCPT,
	SESSION_DATA,
	SESSION_QUIT
};

enum session_command {
	CMD_HELO,
	CMD_MAIL,
	CMD_RCPT,
	CMD_DATA,
	CMD_CRLF,
	CMD_QUIT
};

struct session_data {
	int sockfd;
	int state;
	int command;
	char* client_domain;
};

void* sender_worker(void* data) {
	char* message = (char*) data;
	printf("sender: starting work\n");
	printf("sender: sending message '%s'\n", message);
	printf("sender: finished\n");
}

int get_session_command(char* buffer) {
	if (strncmp(buffer, "HELO", 4) == 0)
		return CMD_HELO;
	if (strncmp(buffer, "MAIL", 4) == 0)
		return CMD_MAIL;
	if (strncmp(buffer, "RCPT", 4) == 0)
		return CMD_RCPT;
	if (strncmp(buffer, "DATA", 4) == 0)
		return CMD_DATA;
	if (strncmp(buffer, "CRLF", 4) == 0)
		return CMD_CRLF;
	if (strncmp(buffer, "QUIT", 4) == 0)
		return CMD_QUIT;
}

void* session_worker(void* data) {
	int capacity = 1;
	int timeout = 15000;
	struct session_data session = *(struct session_data*) data;
	int sockfd = session.sockfd;
	session.command = -1;

	printf("session %d: starting work, state %d\n", sockfd, session.state);

	char* buffer = calloc(1024, sizeof(char));

	int epoll_set = epoll_create(capacity);
	struct epoll_event listen_event;
	listen_event.data.fd = sockfd;
	listen_event.events = EPOLLIN;
	epoll_ctl(epoll_set, EPOLL_CTL_ADD, sockfd, &listen_event);
	struct epoll_event* caught_events = calloc(capacity, sizeof(struct epoll_event));

	char* hello = "im waiting for a hello!";
	send(sockfd, (void*) hello, strlen(hello), 0);

	for (;;) {
		// we wait for some messages
		int nfds = epoll_wait(epoll_set, caught_events, 1, timeout);
		if (nfds == 0) {
			close(sockfd);
			printf("session %d: timeout, work is over\n", sockfd);
			break;
		}

		// we parse our messages
		int i;
		if (nfds == 1) {
			int received_bytes = recv(sockfd, buffer, (sizeof(buffer)) - 1, 0);
			if (received_bytes < 1)
				break;
			if (buffer[received_bytes-1] == '\n')
				buffer[received_bytes-1] = '\0';
			buffer[received_bytes] = '\0';

			if (strlen(buffer) < 4) // dont parse nonsense
				continue;

			session.command = get_session_command(buffer);

			switch(session.command) {
				case CMD_HELO:
					printf("HELO\n");
					session.state = SESSION_GREET;
					//session_reset(session);
					break;
				case CMD_MAIL:
					if (session.state == SESSION_GREET) {
						session.state = SESSION_MAIL;
						printf("MAIL\n");
					}
					break;
				case CMD_RCPT:
					if (session.state == SESSION_MAIL || session.state == SESSION_RCPT) {
						printf("RCPT\n");
						session.state = SESSION_RCPT;
					}
					break;
				case CMD_DATA:
					if (session.state == SESSION_RCPT) {
						session.state = SESSION_DATA;
						printf("DATA\n");
					}
					break;
				case CMD_CRLF:
					printf("CRLF\n");
					//submit_session(session);
					break;
				case CMD_QUIT:
					printf("QUIT\n");
					break;
				default:
					break;
			}

			printf("session %d: received message '%s'\n", sockfd, buffer);
			printf("session %d: handing off message to sender\n", sockfd);
			pthread_t sender;
			pthread_create(&sender, NULL, sender_worker, buffer);
		}
	}
	free(data);
	printf("session %d: finished\n", sockfd);
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
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, PORT, &hints, &servinfo);
	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	freeaddrinfo(servinfo);

	listen(sockfd, BACKLOG);
	printf("server: waiting for connections...\n");

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
				printf("server: got connection from %s\n", s);
				pthread_t session_thread;
				struct session_data * data = malloc(sizeof(struct session_data));
				data->sockfd = new_fd;
				data->state = SESSION_NEW;
				pthread_create(&session_thread, NULL, session_worker, data);
				printf("server: initiated a new session worker\n");
			}
		}
	}
	free(caught_events);

	return 0;
}
