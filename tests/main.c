#include <stdio.h>
#include "src/magpie.h"
#include <string.h>
#include "a.h"
#include "b.h"

int main()
{
// free(a);
#define SIZE 100
	char* strings[SIZE];
	for (size_t i = 0; i < SIZE; i++)
	{
		if (i % 100 == 0)
			printf("i  : %d\n", i);
		strings[i] = malloc(512);
		//free(strings[i]);
	}
	for (size_t i = 0; i < SIZE; i++)
	{
		if (i == 50)
			continue;
		free(strings[i]);
	}
	puts("Done");
	printf("count %zu\n", mp_get_count());
	printf("size %zu\n", mp_get_size());
	mp_terminate();
	return 0;
}