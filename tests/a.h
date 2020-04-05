#include "magpie.h"
// A test function
// Allocates on the same line as B
char* A()
{
	char* s = malloc(100);
	return s;
}