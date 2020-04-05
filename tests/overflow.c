#include <stdio.h>
#define MP_IMPLEMENTATION
#define MP_CHECK_FULL
#include "magpie.h"
#include <string.h>
#include <time.h>
#include "b.h"
#include "a.h"

int main(int argc, char** argv)
{
	srand(time(NULL));
	size_t size = 100;
	char* str = malloc(size);
	str = realloc(str, size+10);
	for(size_t i = 0; i < size; i++)
	{
		str[i] = rand() % ('z' - '0') + '0';
	}
	for(int i = 0; i < 100; i++)
	{
		free(A());
	}
	puts(str);
	free(str);
	puts("Done");
	mp_print_locations();
	(void)mp_terminate();
	return 0;
}