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

#include "session.h"
#include "parser.h"

#define PORT "3490" // the port client will be connecting to 

#define BUFFER_SIZE 1024 // max number of bytes we can get at once 

#define RECV_RESPONSE(sockfd, buffer) numbytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0); \
		if (numbytes == 0) { \
			printf("Server closed connection.\n"); \
			break; \
		} \
		if (numbytes < 0) { \
			printf("Error.\n"); \
			break; \
		} \
		buffer[BUFFER_SIZE] = '\0';

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
	char buffer[BUFFER_SIZE];
	struct addrinfo hints, *servinfo;
	char s[INET6_ADDRSTRLEN];

	if (argc != 5) {
		fprintf(stderr,"usage: client hostname from to message\n");
		exit(1);
	}

	char* from = argv[2];
	char* to = argv[3];
	char* message = argv[4];

	printf("FROM: %s, TO: %s, MESSAGE: %s\n", from, to, message);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(argv[1], PORT, &hints, &servinfo);
	if (!servinfo) {
		printf("Cannot resolve remote address.\n");
	}

	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
		printf("Cannot create socket.\n");
		exit(1);
	}
	if ((connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) < 0) {
		printf("Cannot connect to socket.\n");
		exit(1);
	}
	if ((inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), s, sizeof s)) < 0) {
		printf("Cannot convert to text.\n");
		exit(1);
	}
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo);

	int epfd = epoll_create(1);
	struct epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	struct epoll_event* events = calloc(1, sizeof(struct epoll_event));
	for (;;) {
		epoll_wait(epfd, events, 1, -1);
		char* sendbuffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);

		// get greeting
		RECV_RESPONSE(sockfd, buffer);
		int response;
		if ((response = get_session_response(buffer)) != REPLY_GREET) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		// send helo
		strcpy(sendbuffer,"HELO localhost");
		send(sockfd, sendbuffer, strlen(sendbuffer), 0);

		// get approval
		RECV_RESPONSE(sockfd, buffer);
		if ((response = get_session_response(buffer)) != REPLY_OK) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		// send from address
		strcpy(sendbuffer, "MAIL FROM:");
		strncat(sendbuffer, from, strlen(from));
		send(sockfd, sendbuffer, strlen(sendbuffer), 0);

		// get approval
		RECV_RESPONSE(sockfd, buffer);
		if ((response = get_session_response(buffer)) != REPLY_OK) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		// send to address
		strcpy(sendbuffer, "RCPT TO:");
		strncat(sendbuffer, to, strlen(to));
		send(sockfd, sendbuffer, strlen(sendbuffer), 0);

		// get approval
		RECV_RESPONSE(sockfd, buffer);
		if ((response = get_session_response(buffer)) != REPLY_OK) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		// send data transaction command
		strcpy(sendbuffer, "DATA");
		send(sockfd, sendbuffer, strlen(sendbuffer), 0);

		// get approval
		RECV_RESPONSE(sockfd, buffer);
		if ((response = get_session_response(buffer)) != REPLY_DATA_INFO) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		// send data transaction command
		send(sockfd, message, strlen(message), 0);
		strcpy(sendbuffer, ".\r\n");
		send(sockfd, sendbuffer, strlen(sendbuffer), 0);

		// get confirmation
		RECV_RESPONSE(sockfd, buffer);
		if ((response = get_session_response(buffer)) != REPLY_OK) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		// end session
		strcpy(sendbuffer, "QUIT");
		send(sockfd, sendbuffer, strlen(sendbuffer), 0);

		// get bye from server
		RECV_RESPONSE(sockfd, buffer);
		if ((response = get_session_response(buffer)) != REPLY_BYE) {
			printf("Unexpected reply: %d\n", response);
			break;
		}

		printf("Message sent successfully!\n");
	}
	// close connection
	close(sockfd);

	printf("Done.\n");
	return 0;
}
