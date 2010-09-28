#include "sender.h"

#include <stdio.h>

void* sender_worker(void* data) {
	char* message = (char*) data;
	printf("sender: starting work\n");
	printf("sender: sending message '%s'\n", message);
	printf("sender: finished\n");
	return NULL;
}

