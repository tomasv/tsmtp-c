
#include <stdio.h>

#include "session.h"

#include "sender.h"

void* sender_worker(void* data) {
	struct message_container* message = (struct message_container*) data;
	printf("sender: starting work\n");
	printf("sender: sending message '%s'\n", message->body);
	printf("sender: finished\n");
	return NULL;
}

