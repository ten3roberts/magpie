# Magpie

Magpie is a small and low overhead library for keeping track of allocations and detecting memory leaks

## What it does
* Tracks allocations and deallocations
* Stores origin of allocations
* Stores how many allocations have been made from the same place
* Detects unfreed blocks at the end of the program
* Prints information on where it was allocated and which number it was if done several times from the same place
* Tracks size of allocations and memory usage
* Statistics like total allocation count
* Buffer overrun protection

## Integrating and building
To use magpie in your program copy the header into your project and include it whilst defining MP_IMPLEMENTATION in one C file to build it

Example:
```
#include <stdio.h>
#define MP_IMPLEMENTATION
#include "magpie.h"

...
```
Magpie can be included several times, but only one C file can define MP_IMPLEMENTATION

## Configuration
Configuring of the library is done at build time by defining zero or more of below macros before the header include in the same file as MP_IMPLEMENTATION

If nothing is defined magpie will keep track of allocations done with mp_[m,c,re]alloc and mp_free and detect memory leaks

define MP_CHECK_FULL to use the full library

* MP_DISABLE to turn off storing and tracking of memory blocks and only keeps track of number of allocations by incremention and decremention
-> This disabled almost the whole library including checks for leaks, pointer validity, overflow and almost all else
-> Use in RELEASE builds
* MP_REPLACE_STD to replace the standard malloc, calloc, realloc, and free
* MP_CHECK_OVERFLOW to be able to validate and detect overflows automatically on free or explicitely
* MP_BUFFER_PAD_LEN (default 5) sets the size of the padding in bytes for detecting overflows
-> Higher values require a bit more memory and checking but catches sparse overflows better
* MP_BUFFER_PAD_VAL (default '#') sets the character or value to fill the padding with if MP_
-> This value should be a character not often used to avoid false negatives since overflow can't be detected if the same character is written
-> DO NOT use '\0' or 0 as it is the most common character to overflow
* MP_FILL_ON_FREE to fill buffer on free with MP_BUFFER_PAD_VAL, this is to avoid reading a pointers data after it has been freed and not overwritten by others

* MP_CHECK_FULL to define MP_REPLACE_STD, MP_CHECK_OVERFLOW, MP_FILL_ON_FREE

## Leak checking
Run mp_terminate at the end of the program, leak checking is enabled by default if not MP_DISABLE is defined

This will print out the information about the remaining blocks such as

* Where they were allocated as file:line, ctrl+click to follow in vscode
* The size of the leaked blocks
* Which number the allocation was from the same place, I.e; if allocation is performed multiple times in a loop, it will print which iteration of the loop leaked
* mp_terminate will also free internal resources and remaining blocks
* Which number of allocations from the same place it was, if allocating in a loop, this will show you which iteration of the loop did not get free
* Total number of leakd blocks
* mp_terminate will also free all remaining blocks and all internal resources, can safely be called if no allocations have happened

## Buffer overflow cheking
Enable by defining MP_CHECK_OVERFLOW (see Configuration)
Will check if more than allocated size has been written to the buffer when it is freed.
This is achieved by adding padding to the end of the buffer and check if it has been altered at free

No overhead when writing to buffer

Buffer overflow can also be checked explcitely with mp_validate without freeing the block

## Examples
```
#include <stdio.h>
#define MP_IMPLEMENTATION
#define MP_CHECK_FULL
#include "magpie.h"
#include <string.h>
#include <time.h>

int main(int argc, char** argv)
{
	srand(time(NULL));    	// Seed random generator
	size_t size = 32;
	char* str = malloc(size);
	// Fill string with random characters
	for (size_t i = 0; i < size; i++)
	{
		str[i] = rand() % ('z' - 'A') + 'A';
	}

	str[size] = '\0';     	// Null terminate string (buffer overflow)
	puts(str);
	free(str);            	// Checks for overflow, memory and tracking is freed
	puts("Done");
	mp_print_locations(); 	// Print where allocations happened and how many happened on each line
	mp_terminate();		// Check for leaked blocks
	return 0;
}
```
Output:
```
l\whdFqu^NsrMXr\F`sy`gvwuYbWNgtQ
Buffer overflow after 32 bytes on pointer 0x55555555a2c0 allocated at tests/overflow.c:12
Done
Allocator at tests/overflow.c:12 made 1 allocations
A total of 0 memory blocks remain to be freed after program execution
```
