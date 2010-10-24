#include "listener.h"

#define DEFAULT_PORT "3490"

int main(int argc, char** argv)
{
	if (argc == 2)
		listener(argv[1]);
	else
		listener(DEFAULT_PORT);
	return 0;
}
