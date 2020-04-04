#ifndef MAGPIE_H
#define MAGPIE_H

#ifdef _STDLIB_H
#error "stdlib.h should not be included before magpie.h"
#endif
#include <stdint.h>
#include <stdlib.h>

#define MP_VALIDATE_OK 0
#define MP_VALIDATE_INVALID -1
#define MP_VALIDATE_OVERFLOW -2

// Sets the message callback function
// Default function is puts if not set
// Set to NULL to quiet
void mp_set_msgcallback(void (*func)(const char* msg));
// Returns the number of blocks allocated
size_t mp_get_count();

// Returns the number of bytes allocated
size_t mp_get_size();

// Prints the locations of all [c,a,re]allocs and how many allocations was performed there
void mp_print_locations();

// Checks if any blocks remain to be freed
// Should only be run at the end of the program execution
// Uses the msg
// Returns how many blocks of memory that weren't freed
// Frees any remaining blocks
// Releases all internal resources
size_t mp_terminate();

// Checks for buffer overruns and pointer life
// Returns MP_VALIDATE_[OK,INVALID,OVERFLOW]
int mp_validate_internal(void* ptr, const char* file, uint32_t line);

void* mp_malloc_internal(size_t size, const char* file, uint32_t line);
void* mp_calloc_internal(size_t num, size_t size, const char* file, uint32_t line);
void* mp_realloc_internal(void* ptr, size_t size, const char* file, uint32_t line);
void mp_free_internal(void* ptr, const char* file, uint32_t line);


#define mp_validate(ptr) mp_validate_internal(ptr, __FILE__, __LINE__)
#define mp_malloc(size)		  mp_malloc_internal(size, __FILE__, __LINE__)
#define mp_calloc(num, size)  mp_calloc_internal(num, size, __FILE__, __LINE__)
#define mp_realloc(ptr, size) mp_realloc_internal(ptr, size, __FILE__, __LINE__)
#define mp_free(ptr)		  mp_free_internal(ptr, __FILE__, __LINE__)

#ifdef MP_REPLACE_STD
#define malloc(size)	   mp_malloc_internal(size, __FILE__, __LINE__);
#define calloc(num, size)  mp_calloc_internal(num, size, __FILE__, __LINE__);
#define realloc(ptr, size) mp_realloc_internal(ptr, size, __FILE__, __LINE__);
#define free(ptr)		   mp_free_internal(ptr, __FILE__, __LINE__);
#endif

#endif