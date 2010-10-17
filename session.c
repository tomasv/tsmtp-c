
#include "sender.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>     // for close();
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "list.h"
#include "parser.h"
#include "session.h"

#define REPLY_TO_CLIENT(sock, reply) send(sock, (void*) server_replies[reply], strlen(server_replies[reply]), 0) 

static char* server_replies[] = {
	"250 Ok\n",
	"220 SMTP tsmtp\n",
	"501 Syntax error\n",
	"354 End data with <CR><LF>.<CR><LF>\n",
	"221 Bye\n",
	"503 Commands out of order: HELO -> (MAIL -> RCPT (+) -> DATA -> text -> CRLF.CRLF)(*) -> QUIT\n"
};

void session_reset(struct session_data * session) {
	session->state = SESSION_NEW;
	session->command = -1;
	session->domain = NULL;
}

void session_submit(struct session_data * session) {
	printf("session %d: received message '%s'\n", session->sockfd, session->message->body);
	printf("session %d: handing off message to sender\n", session->sockfd);
	pthread_t sender;
	pthread_create(&sender, NULL, sender_worker, session->message);
}

void* session_worker(void* data) {
	int capacity = 1;
	int timeout = 30000; // 30 seconds

	// setup session data container
	struct session_data * session = (struct session_data*) data;
	int sockfd = session->sockfd;
	session_reset(session);
	session->message = (struct message_container*) malloc(sizeof(struct message_container));
	session->message->mail_from = NULL;
	session->message->rcpt_to = NULL;
	session->message->body = NULL;

	printf("session %d: starting work, state %d\n", sockfd, session->state);

	REPLY_TO_CLIENT(sockfd, REPLY_GREET);

	// buffer for receiving messages
	int buffer_size = 1024;
	char* buffer = calloc(buffer_size, sizeof(char));

	// epoll setup
	int epoll_set = epoll_create(capacity);
	struct epoll_event listen_event;
	listen_event.data.fd = sockfd;
	listen_event.events = EPOLLIN;
	epoll_ctl(epoll_set, EPOLL_CTL_ADD, sockfd, &listen_event);
	struct epoll_event* caught_events = calloc(capacity, sizeof(struct epoll_event));

	while (session->state != SESSION_QUIT) {
		// we wait for some messages
		int nfds = epoll_wait(epoll_set, caught_events, 1, timeout);
		if (nfds == 0) {
			// timeout
			close(sockfd);
			printf("session %d: timeout, work is over\n", sockfd);
			break;
		}

		// we parse our messages, we assume only one event happens
		if (nfds == 1) {
			int received_bytes = recv(sockfd, buffer, buffer_size - 1, 0);

			// connection closed
			if (received_bytes == 0) {
				printf("session %d: client closed connection\n", sockfd);
				break;
			}

			// errors
			if (received_bytes < 0) {
				printf("session %d: recv error\n", sockfd);
				break;
			}

			// make it null terminated in case it isn't already so
			buffer[received_bytes] = '\0';

			// form a request structure using the received message
			struct request* req = parse_request(buffer);

			// dont interpret text if we're in DATA state
			if (session->state == SESSION_DATA && req->command != CMD_CRLF) {
				// add line to body
				if (!session->message->body) {
					session->message->body = strdup(buffer);
				} else {
					// gotta love C
					int size = strlen(buffer) + strlen(session->message->body) + 1;
					char * extended_message = (char*) malloc(sizeof(char) * size);
					strncpy(extended_message, session->message->body, strlen(session->message->body));
					strncat(extended_message, buffer, strlen(buffer));
					extended_message[size] = '\0';
					free(session->message->body);
					session->message->body = extended_message;
				}
				if (req->arguments) {
					free_list(req->arguments);	
				}
				free(req);
				continue;
			}

			// fill session data according to request
			switch(req->command) {
				case CMD_HELO:
					// expect a helo at start; a helo resets session; helo expects 1 argument: domain
					session_reset(session);
					if (session->state == SESSION_NEW) {
						REPLY_TO_CLIENT(sockfd, REPLY_OK);
					}
					session->state = SESSION_GREET;
					session->domain = strdup((char*) (req->arguments->data));
					printf("session %d: state changed to greeted\n", sockfd);
					break;
				case CMD_MAIL:
					// mail from expects an argument: sender address;
					// can be after a helo or transaction completion
					if (session->state == SESSION_GREET) {
						session->state = SESSION_MAIL;
						session->message->mail_from = strdup((char*) (req->arguments->data));
						REPLY_TO_CLIENT(sockfd, REPLY_OK);
						printf("session %d: got mail from\n", sockfd);
					} else {
						REPLY_TO_CLIENT(sockfd, REPLY_OOO);
					}
					break;
				case CMD_RCPT:
					// rcpt to expects an argument: receipient address;
					// can be after a mail from or another rcpt to command
					if (session->state == SESSION_MAIL || session->state == SESSION_RCPT) {
						session->state = SESSION_RCPT;
						session->message->rcpt_to = strdup((char*) (req->arguments->data));
						REPLY_TO_CLIENT(sockfd, REPLY_OK);
						printf("session %d: state changed to got receipients\n", sockfd);
					} else {
						REPLY_TO_CLIENT(sockfd, REPLY_OOO);
					}
					break;
				case CMD_DATA:
					// no arguments; must be after a rcpt to command
					if (session->state == SESSION_RCPT) {
						session->state = SESSION_DATA;
						REPLY_TO_CLIENT(sockfd, REPLY_DATA_INFO);
						printf("session %d: state changed to data receival\n", sockfd);
					} else {
						REPLY_TO_CLIENT(sockfd, REPLY_OOO);
					}
					break;
				case CMD_CRLF:
					// signifies an end of a message's body;
					// only after data command was invoked
					if (session->state == SESSION_DATA) {
						session_submit(session);
						session->state = SESSION_GREET;
						REPLY_TO_CLIENT(sockfd, REPLY_OK);
						printf("session %d: data transaction over, state changed to greeted\n", sockfd);
					} else {
						REPLY_TO_CLIENT(sockfd, REPLY_OOO);
					}
					break;
				case CMD_QUIT:
					// closes session
					session->state = SESSION_QUIT;
					REPLY_TO_CLIENT(sockfd, REPLY_BYE);
					printf("session %d: state changed to quit\n", sockfd);
					break;
				default:
					// send syntax error
					REPLY_TO_CLIENT(sockfd, REPLY_SYNTAX_ERROR);
					printf("session %d: syntax error\n", sockfd);
					break;
			}
			if (req->arguments) {
				free_list(req->arguments);	
			}
			free(req);
		}
	}
	free(buffer);
	free(data);
	printf("session %d: finished\n", sockfd);
	close(sockfd);

	return NULL;
}
