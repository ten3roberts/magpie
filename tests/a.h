#include "magpie.h"
// A test function
// Allocates on the same line as B
char* A()
{
	static int i = 0;
	printf("i : %d\n", i++);
	char* s = malloc(100);
	return s;
}