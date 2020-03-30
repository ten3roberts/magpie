#include <stdio.h>
#include "src/magpie.h"
#include <string.h>
#include "a.h"
#include "b.h"

int main()
{
	// free(a);
	char* strings[100];
	for (size_t i = 0; i < 100; i++)
	{
		strings[i] = malloc(512);
	}
	for (size_t i = 0; i < 100; i++)
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