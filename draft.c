


struct rcpt_list {
	char* rcpt;
	struct rcpt_list* next;
};

struct data_t {
	char* headers;
	char* text;
}

struct message {
	char* mail_from;
	struct rcpt_list* rcpt_list;
	struct data_t data;
};

struct message_list {
	struct message* message;
	struct message_list* next;
};

int             message_list_push(struct message* new_message);  // add to end of message queue
struct message* message_list_shift();                            // get first in queue


