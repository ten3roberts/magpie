#include <stdio.h>
#include "src/magpie.h"
#include <string.h>
#include "a.h"
#include "b.h"

int main()
{
	char* a = A();
	char* b = B();
	free(a);
	//free(a);
	a = A();
	char* c = malloc(120);
	free(a);
	//b = calloc(1, 120);

	for (size_t i = 0; i < 1000; i++)
	{
		// free(strings);
	}
	puts("Done");
	free(a);
	free(b);
	free(c);
	return 0;
}