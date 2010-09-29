#ifndef SESSION_H
#define SESSION_H

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

enum server_replies {
	REPLY_OK,
	REPLY_GREET,
	REPLY_SYNTAX_ERROR
};

struct session_data {
	int sockfd;
	int state;
	int command;
	char* client_domain;
};

struct request_command {
	int command;
	struct list* arguments;
};

int get_session_command(char* buffer);
void* session_worker(void* data);

#endif
