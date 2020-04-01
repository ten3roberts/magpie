#include <stdio.h>
#include "src/magpie.h"
#include <string.h>
#include "a.h"
#include "b.h"

int main(int argc, char** argv)
{
#define SIZE 0xffffff

	char** strings = malloc(SIZE * sizeof(char*));
	printf("Allocating %zu strings\n", SIZE);
	for (size_t i = 0; i < SIZE; i++)
	{
		strings[i] = malloc(512);
	}
	size_t skipped = 0;
	for (size_t i = 0; i < SIZE; i++)
	{
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