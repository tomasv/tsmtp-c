
#include <stdio.h>
#include <stdlib.h>

#include "session.h"

#include "sender.h"

void* sender_worker(void* data) {
	printf("sender: starting work\n");

	struct message_container* message = (struct message_container*) data;
	printf("sender: sending message\n");
	printf("sender: FROM: %s\n", message->mail_from);
	printf("sender: TO: %s\n", message->rcpt_to);
	printf("sender: BODY:\n");
	if (message->body)
		printf("%s", message->body);
	else
		printf("No body.\n");
	printf("\n");

	// cleanup
	free(message->mail_from);
	free(message->rcpt_to);
	free(message->body);
	free(message);
	printf("sender: finished\n");
	return NULL;
}

