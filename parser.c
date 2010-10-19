
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netdb.h>

#include "session.h"
#include "list.h"

#include "parser.h"

#define BUFFER_SIZE 1024

static int command_lengths[] = { 4, 10, 8, 4, 5, 4 };
static char* command_names[] = {
	"HELO",
	"MAIL FROM:",
	"RCPT TO:",
	"DATA",
	"ENDOFMESSAGE",
	"QUIT"
};

int get_session_command(char* buffer) {
	if (strncmp(buffer, "HELO", 4) == 0)
		return CMD_HELO;
	if (strncmp(buffer, "MAIL FROM:", 10) == 0)
		return CMD_MAIL;
	if (strncmp(buffer, "RCPT TO:", 8) == 0)
		return CMD_RCPT;
	if (strncmp(buffer, "DATA", 4) == 0)
		return CMD_DATA;
	if (strncmp(buffer, ".\r\n", 3) == 0)
		return CMD_CRLF;
	if (strncmp(buffer+strlen(buffer)-5, "\r\n.\r\n", 5) == 0)
		return CMD_CRLF;
	if (strncmp(buffer, "QUIT", 4) == 0)
		return CMD_QUIT;
	return -1;
}

int get_session_response(char* buffer) {
	if (strncmp(buffer, "250", 3) == 0)
		return REPLY_OK;
	if (strncmp(buffer, "220", 3) == 0)
		return REPLY_GREET;
	if (strncmp(buffer, "501", 3) == 0)
		return REPLY_SYNTAX_ERROR;
	if (strncmp(buffer, "354", 3) == 0)
		return REPLY_DATA_INFO;
	if (strncmp(buffer, "221", 3) == 0)
		return REPLY_BYE;
	if (strncmp(buffer, "503", 3) == 0)
		return REPLY_OOO;
	return -1;
}

char * get_argument_string_start(char * message, enum session_command cmd) {
	return (message + command_lengths[cmd]);
}

char * get_email(char * argument) {
	char * start = strpbrk(argument, "<");
	char * end = strpbrk(argument, ">");
	if (start == NULL || end == NULL || start >= end)
		return NULL;
	start++;
	int len = end - start;
	char * email = malloc(len * sizeof(char) + 1);
	strncpy(email, start, len);
	email[strlen(email)] = '\0';
	return email;
}

int check_domain(char * domain)
{
	if (!gethostbyname(domain))
		return 0;
	return 1;
}

struct request * parse_request(char * message)
{
	struct request * req = malloc(sizeof(struct request));
	req->command = get_session_command(message);
	if (req->command == -1) {
		return req;
	}
	req->arguments = NULL;

	char * buffer = strdup(message);
	char * arg_start = get_argument_string_start(buffer, req->command);

	char* delim = " \t\r";
	char* word;

	for (word = strtok(arg_start, delim); word; word = strtok(NULL, delim)) {
		// avoid empty arguments (newlines, carry returns, etc.)
		if (!strncmp(word, "\n\0", 2))
			break;
		if (!strncmp(word, "\r\0", 2))
			break;
		add_to_list(&(req->arguments), strdup(word));
	}

	switch(req->command) {
		case CMD_HELO:
		case CMD_MAIL:
		case CMD_RCPT:
			// if no arguments, these commands are invalid
			if (!req->arguments)
				req->command = -1;
			break;
		default:
			break;
	}

	free(buffer);
	return req;
}

#ifdef PARSER_TEST
int main(int argc, const char *argv[])
{
	char * msg = calloc(BUFFER_SIZE, sizeof(char));

	msg = "HELO\n";
	parse_request(msg);
	msg = "MAIL FROM: b <c>";
	parse_request(msg);
	msg = "RCPT TO: google.com";
	parse_request(msg);
	msg = "DATA asda <super@email.com>";
	parse_request(msg);
	msg = ".\r\n";
	parse_request(msg);
	msg = "QUIT";
	parse_request(msg);
	

	return 0;
}
#endif
