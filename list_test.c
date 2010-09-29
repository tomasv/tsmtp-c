
#include <stdio.h>
#include <stdlib.h>

#include <vector>

int main(int argc, const char *argv[])
{
	std::vector<int> ints;
	ints.push_back(1);
	int i;
	for (i = 0; i < ints.size(); i++) {
		printf("%d\n", ints[i]);
	}
	return 0;
}
