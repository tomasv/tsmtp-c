#include "list.h"

#include <stdlib.h>

void add_to_list(struct list **head, void *data) {
	if (*head) {
		struct list* node;
		while (node->next)
			node = node->next;
		node->next       = (struct list *) malloc(sizeof(struct list));
		node->next->data = data;
	} else {
		(*head)       = (struct list *) malloc(sizeof(struct list));
		(*head)->data  = data;
	}
}

void* get_from_list(struct list *head, int index) {
	while (head && !index--)
		head = head->next;
	if (head)
		return head->data;
	else
		return NULL;
}
