#include <stdio.h>
#include "src/magpie.h"
#include <string.h>
#include "a.h"
#include "b.h"

int main(int argc, char** argv)
{
	size_t size = 1000;
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
	malloc(1000);
	size_t skipped = 0;
	for (size_t i = 0; i < size; i++)
	{
		if (i % 50 == 0)
			continue;
		free(strings[i]);
	}
	free(strings);
	puts("Done");
	printf("remaining count %zu\n", mp_get_count());
	printf("Skipped %zu\n", skipped);
	printf("size %zu\n", mp_get_size());
	mp_terminate();
	return 0;
}