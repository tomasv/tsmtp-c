#include "list.h"

#include <stdlib.h>

void add_to_list(struct list **head, void *data) {
	struct list * node;
	node = *head;
	if (!node) {
		node = (struct list*) malloc(sizeof(struct list));
		node->data = data;
		node->next = NULL;
		*head = node;
	} else {
		while (node->next)
			node = node->next;
		node->next       = (struct list *) malloc(sizeof(struct list));
		node->next->data = data;
	}
}

void* get_from_list(struct list *head, int index) {
	if (index  < 0)
		return NULL;
	while (head && index--)
		head = head->next;

	if (head)
		return head->data;
	else
		return NULL;
}

void free_list(struct list* head) {
	struct list *node = head, *temp;
	while (node) {
		if (node->data)
			free(node->data);
		temp = node;
		node = node->next;
		free(temp);
	}
}
