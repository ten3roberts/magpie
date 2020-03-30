#ifndef MAGPIE_H
#define MAGPIE_H
#include <stdint.h>
#include <stdlib.h>

#define MP_STATUS 0
#define MP_ERROR  1


// Returns the number of blocks allocated
size_t mp_get_count();

// Returns the number of bytes allocated
size_t mp_get_size();

void* mp_malloc(size_t size, const char* file, uint32_t line);
void* mp_calloc(size_t num, size_t size, const char* file, uint32_t line);
void* mp_realloc(void* ptr, size_t size, const char* file, uint32_t line);

void mp_free(void* ptr, const char* file, uint32_t line);

#ifdef MP_REPLACE_STD
#define malloc(size)	   mp_malloc(size, __FILE__, __LINE__);
#define calloc(num, size)  mp_calloc(num, size, __FILE__, __LINE__);
#define realloc(ptr, size) mp_realloc(ptr, size, __FILE__, __LINE__);
#define free(ptr)		   mp_free(ptr, __FILE__, __LINE__);
#endif

#endif