#include "src/magpie.h"

// B test function
// Allocates on the same line as A
char* B()
{
	char* s = malloc(100);
	return s;
}