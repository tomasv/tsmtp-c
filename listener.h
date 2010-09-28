#ifndef LISTENER_H
#define LISTENER_H

#define BACKLOG 10     // how many pending connections queue will hold

#include <netinet/in.h>

void *get_in_addr(struct sockaddr *sa);
int listener(char* service);

#endif
