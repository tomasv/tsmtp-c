#ifndef PARSER_H
#define PARSER_H 

struct request * parse_request(char * message);
int get_session_response(char* message);
int check_domain(char * domain);
char * get_email(char * argument);

#endif
