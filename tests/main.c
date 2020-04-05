#include <stdio.h>
#define MP_IMPLEMENTATION
#define MP_CHECK_FULL
#include "magpie.h"
#include "a.h"
#include <string.h>


int main(int argc, char** argv)
{
	size_t size = 100000;
	if (argc > 1)
	{
		size = atoi(argv[1]);
	}

	char** strings = malloc(size * sizeof(char*));
	printf("Allocating %zu strings\n", size);
	for (size_t i = 0; i < size; i++)
	{
		strings[i] = malloc(512);
	}
	char* s = A();
	size_t skipped = 0;
	for (size_t i = 0; i < size; i++)
	{
		if (rand() % 10000 == 0)
		{
			skipped++;
			continue;
		}
		free(strings[i]);
	}
	puts("Done");
	printf("remaining count %zu\n", mp_get_count());
	printf("Skipped %zu\n", skipped);
	printf("size %zu\n", mp_get_size());
	free(s);
	mp_print_locations();
	(void)mp_terminate();
	return 0;
}