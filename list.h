#ifndef LIST_H
#define LIST_H

struct list {
	void* data;
	struct list* next;
};

void  add_to_list(struct list **head, void *data);
void* get_from_list(struct list *head, int index);
void free_list(struct list* head);

#endif
