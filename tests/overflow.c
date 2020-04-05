#include <stdio.h>
#define MP_IMPLEMENTATION
#define MP_CHECK_FULL
#include "magpie.h"
#include <string.h>
#include "time.h"

int main(int argc, char** argv)
{
	srand(time(NULL));
	size_t size = 100;
	char* str = malloc(size+1);
	for(size_t i = 0; i < size+1; i++)
	{
		str[i] = rand() % ('z' - '0') + '0';
	}
	str[size+1] = '\0';
	free(str);
	puts(str);
	puts("Done");
	mp_print_locations();
	(void)mp_terminate();
	return 0;
}