
#include "sender.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>     // for close();
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "session.h"
#include "list.h"

int get_session_command(char* buffer) {
	if (strncmp(buffer, "HELO", 4) == 0)
		return CMD_HELO;
	if (strncmp(buffer, "MAIL FROM:", 10) == 0)
		return CMD_MAIL;
	if (strncmp(buffer, "RCPT TO:", 8) == 0)
		return CMD_RCPT;
	if (strncmp(buffer, "DATA", 4) == 0)
		return CMD_DATA;
	if (strncmp(buffer, "\r\n.\r\n", 5) == 0)
		return CMD_CRLF;
	if (strncmp(buffer, "QUIT", 4) == 0)
		return CMD_QUIT;
	return -1;
}

struct request_command* parse_request(char* request) {
	struct request_command* request_command = malloc(sizeof(struct request_command));
	char* delim = " \t";
	char* word;

	if ((request_command.command = get_session_command(request)) == -1); {
		free(request_command);
		return NULL;
	}

	for (word = strtok(request, delim); word; word = strtok(NULL, delim)) {
		printf("request word is: %s\n", word);
	}
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
		if (nfds == 1) {
			int received_bytes = recv(sockfd, buffer, (sizeof(buffer)) - 1, 0);
			if (received_bytes < 1)
				break;
			if (buffer[received_bytes-1] == '\n')
				buffer[received_bytes-1] = '\0';
			buffer[received_bytes] = '\0';

			if (strlen(buffer) < 4) // dont parse nonsense
				continue;

			struct request_command* req = parse_request(buffer);
			if (req == NULL) {
				// send syntax error
				break;
			}

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

	return NULL;
}
