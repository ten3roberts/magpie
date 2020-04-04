#include "src/magpie.h"

// A test function
// Allocates on the same line as B
char* A()
{
	char* s = calloc(100, 1);
	return s;
}