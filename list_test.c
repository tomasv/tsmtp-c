
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"

int list_data_is(struct list* head, int index, void* data) {
	return (get_from_list(head, index) == data);
}

int list_value_is(struct list* head, int index, int value) {
	return (*(int*) get_from_list(head, index) == value);
}

void test_adding_to_list() {
	struct list* head = NULL;

	int one = 1, two = 2, three = 3;

	assert(list_data_is(head, -1, NULL));
	assert(list_data_is(head, 0, NULL));
	assert(list_data_is(head, 1, NULL));
	assert(list_data_is(head, 2, NULL));

	add_to_list(&head, &one);
	assert(list_data_is(head, -1, NULL));
	assert(list_value_is(head, 0, 1));

	add_to_list(&head, &two);
	assert(list_value_is(head, 0, 1));
	assert(list_value_is(head, 1, 2));
	assert(list_data_is(head, 2, NULL));

	add_to_list(&head, &three);
	assert(list_value_is(head, 0, 1));
	assert(list_value_is(head, 1, 2));
	assert(list_value_is(head, 2, 3));
	assert(list_data_is(head, 3, NULL));
}

int main(int argc, const char *argv[])
{
	test_adding_to_list();
	return 0;
}
