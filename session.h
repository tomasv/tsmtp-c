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
	REPLY_SYNTAX_ERROR,
	REPLY_DATA_INFO,
	REPLY_BYE,
	REPLY_OOO
};

struct session_data {
	int sockfd;
	int state;
	int command;
	char* domain;
	struct message_container* message;
};

struct message_container {
	char* mail_from;
	char* rcpt_to;
	char* body;
};

struct request {
	int command;
	char* arguments;
};

int get_session_command(char* buffer);
void* session_worker(void* data);

#endif
