#undef MP_REPLACE_STD
#include "magpie.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define MP_CHECK_OVERFLOW
#define MP_FILL_ON_FREE

#ifndef MP_BUFFER_PAD_LEN
#define MP_BUFFER_PAD_LEN 5
#endif

#ifndef MP_BUFFER_PAD_VAL
#define MP_BUFFER_PAD_VAL '#'
#endif

// The number of allocated blocks of memory
size_t alloc_count = 0;
// The number of bytes allocated
size_t alloc_size = 0;

void msg_default(const char* msg)
{
	(void)puts(msg);
}
void (*msg_func)(const char* msg) = msg_default;

#define MSG(m)    \
	if (msg_func) \
		msg_func(m);

// A memory block stored based on line of initial allocation in a binary tree
struct MemBlock
{
	size_t size;
	const char* file;
	uint32_t line;
	uint32_t count;
	struct MemBlock* next;
	char bytes[1];
};

struct PHashTable
{
	// Describes the allocated amount of buckets in the hash table
	size_t size;
	// Describes how many buckets are in use
	size_t count;
	struct MemBlock** items;
};

// Describes the location of a malloc
// Used to track where allocations come from and how many has been allocated from the same place in the code
struct PAllocLocation
{
	const char* file;
	uint32_t line;
	// How many allocations have been done at file:line
	// Does not decrement on free
	uint32_t count;
	struct PAllocLocation *prev, *next;
};

static struct PHashTable phashtable = {0};
static struct PAllocLocation* locations = NULL;

// Hash functions from https://gist.github.com/badboy/6267743
#if SIZE_MAX == 0xffffffff // 32 bit
size mp_hash_ptr(void* ptr)
{
	int c2 = 0x27d4eb2d; // a prime or an odd constant
	key = (key ^ 61) ^ (key >>> 16);
	key = key + (key << 3);
	key = key ^ (key >>> 4);
	key = key * c2;
	key = key ^ (key >>> 15);
	return key;
}
#elif SIZE_MAX == 0xffffffffffffffff // 64 bit
size_t mp_hash_ptr(void* ptr)
{

	size_t key = (size_t)ptr;
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);

	// Fit to table
	// Since size is a power of two, it is faster than modulo
	return key & (phashtable.size - 1);
	;
}
#endif

// Inserts block and correctly resizes the hashtable
// Counts and increases how many allocations have come from the same file and line
void mp_insert(struct MemBlock* block, const char* file, uint32_t line);

// Resizes the list either up (1) or down (-1), does nothing if incorrect value
void mp_resize(int direction);

// Searches for the pointer in the tree
struct MemBlock* mp_search(void* ptr);

// Searches and removes a memblock storing the ptr from the hashmap
// Returns the memblock, or NULL if failed
struct MemBlock* mp_remove(void* ptr);

size_t mp_get_count()
{
	return alloc_count;
}

size_t mp_get_size()
{
	return alloc_size;
}

void mp_print_locations()
{
	struct PAllocLocation* it = locations;
	while (it)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "Allocator at %s:%u made %u allocations", it->file, it->line, it->count);
		MSG(msg);
		it = it->next;
	}
}

size_t mp_terminate()
{
	char msg[1024];
	size_t remaining_blocks = 0;

	// Free remaining blocks
	for (size_t i = 0; i < phashtable.size; i++)
	{
		struct MemBlock* it = phashtable.items[i];
		struct MemBlock* next = NULL;
		while (it)
		{
			remaining_blocks++;
			next = it->next;
			snprintf(msg, sizeof msg,
					 "Memory block allocated at %s:%u with a size of %zu bytes has not been freed. Block was "
					 "allocation num %u",
					 it->file, it->line, it->size, it->count);
			MSG(msg);
			mp_free(it->bytes);
			it = next;
		}
	}
	snprintf(msg, sizeof msg, "A total of %zu memory blocks remain to be freed after program execution",
			 remaining_blocks);
	MSG(msg);
	free(phashtable.items);
	phashtable.items = NULL;
	phashtable.count = 0;
	phashtable.size = 0;

	// Free the location list
	struct PAllocLocation* it = locations;
	struct PAllocLocation* next = NULL;
	while (it)
	{
		next = it->next;
		free(it);
		it = next;
	}
	locations = NULL;
	return remaining_blocks;
}

int mp_validate_internal(void* ptr, const char* file, uint32_t line)
{
	struct MemBlock* block = mp_search(ptr);
	if (block == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%u Validation of invalid or already freed pointer with adress %p", file, line,
				 ptr);
		MSG(msg);
		return MP_VALIDATE_INVALID;
	}
	// Check integrity of buffer padding to detect overflows/overruns
	size_t i = 0;
	char* p = block->bytes + block->size;
	for (i = 0; i < MP_BUFFER_PAD_LEN; i++, p++)
	{
		if (*p != MP_BUFFER_PAD_VAL)
		{
			char msg[1024];
			snprintf(msg, sizeof msg, "Buffer overflow after %zu bytes on pointer %p allocated at %s:%u", block->size,
					 ptr, block->file, block->line);
			MSG(msg);
			return MP_VALIDATE_OVERFLOW;
		}
	}
	return MP_VALIDATE_OK;
}

void* mp_malloc_internal(size_t size, const char* file, uint32_t line)
{
	// Allocate size for the block info and the buffer requested
	struct MemBlock* new_block = malloc(sizeof(struct MemBlock) + size - 1 + MP_BUFFER_PAD_LEN);

// Fill the padding with MP_BUFFER_PAD_VAL
#ifdef MP_CHECK_OVERFLOW
	memset(new_block->bytes + size, MP_BUFFER_PAD_VAL, MP_BUFFER_PAD_LEN);
#endif

	// Allocate request
	if (new_block == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%d Failed to allocate memory for %zu bytes", file, line, size);
		MSG(msg);
		return NULL;
	}
	alloc_count++;
	alloc_size += size;
	new_block->size = size;
	new_block->file = file;
	new_block->line = line;
	new_block->next = NULL;

	// Insert
	mp_insert(new_block, file, line);

	return new_block->bytes;
}
void* mp_calloc_internal(size_t num, size_t size, const char* file, uint32_t line)
{
	// Allocate size for the block info and the buffer requested
	struct MemBlock* new_block = calloc(1, sizeof(struct MemBlock) + num * size - 1 + MP_BUFFER_PAD_LEN);

	// Fill the padding with MP_BUFFER_PAD_VAL
#ifdef MP_CHECK_OVERFLOW
	memset(new_block->bytes + num * size, MP_BUFFER_PAD_VAL, MP_BUFFER_PAD_LEN);
#endif

	// Allocate request

	if (new_block == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%u Failed to allocate memory for %zu bytes", file, line, size * num);
		MSG(msg);
		return NULL;
	}
	alloc_count++;
	alloc_size += num * size;
	new_block->size = num * size;
	new_block->file = file;
	new_block->line = line;
	new_block->next = NULL;
	// Insert
	mp_insert(new_block, file, line);

	return new_block->bytes;
}
void* mp_realloc_internal(void* ptr, size_t size, const char* file, uint32_t line);

void mp_free_internal(void* ptr, const char* file, uint32_t line)
{
	struct MemBlock* block = mp_remove(ptr);
	if (block == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%u Freeing invalid or already freed pointer with adress %p", file, line, ptr);
		MSG(msg);
		return;
	}
	alloc_count--;
	alloc_size -= block->size;

	// Check integrity of buffer padding to detect overflows/overruns
	size_t i = 0;
	char* p = block->bytes + block->size;
	for (i = 0; i < MP_BUFFER_PAD_LEN; i++, p++)
	{
		if (*p != MP_BUFFER_PAD_VAL)
		{
			char msg[1024];
			snprintf(msg, sizeof msg, "Buffer overflow after %zu bytes on pointer %p allocated at %s:%u", block->size,
					 ptr, block->file, block->line);
			MSG(msg);
			break;
		}
	}

#ifdef MP_FILL_ON_FREE
	memset(block->bytes, MP_BUFFER_PAD_VAL, block->size);
#endif
		free(block);
}

void mp_insert(struct MemBlock* block, const char* file, uint32_t line)
{
	block->next = NULL;
	// Hash the pointer
	if (phashtable.size == 0)
	{
		phashtable.size = 16;
		phashtable.items = calloc(phashtable.size, sizeof(*phashtable.items));
	}
	if (phashtable.count + 1 >= phashtable.size * 0.7)
	{
		mp_resize(1);
	}
	{
		// Takes the hash of the bytes pointer of the block
		size_t hash = mp_hash_ptr(block->bytes);
		struct MemBlock* it = phashtable.items[hash];

		if (it == NULL)
		{
			phashtable.items[hash] = block;
			phashtable.count++;
		}

		// Chain if hash collision
		else
		{
			while (it->next)
			{
				it = it->next;
			}
			it->next = block;
		}
	}
	// Items are being reinserted
	if (file == NULL)
	{
		return;
	}
	// Location
	if (locations == NULL)
	{
		locations = malloc(sizeof(struct PAllocLocation));
		locations->count = 0;
		locations->file = file;
		locations->line = line;
		locations->prev = NULL;
		locations->next = NULL;
		block->count = locations->count++;
		return;
	}
	struct PAllocLocation* it = locations;
	while (it)
	{

		if (it->file == file && it->line == line)
		{
			block->count = it->count++;

			// Make sure it is always sorted by biggest on head
			while (it->prev && it->prev->count < it->count)
			{
				struct PAllocLocation* prev = it->prev;
				// Head is changing
				if (prev->prev == NULL)
				{
					locations->prev = it;
					locations->next = it->next;
					if (it->next)
						it->next->prev = locations;
					it->next = locations;
					locations = it;
					it->prev = NULL;
					break;
				}
				prev->next = it->next;
				it->prev = prev->prev;
				prev->prev->next = it;
				prev->prev = it;

				it->next = prev;
			}
			return;
		}
		// At end
		if (it->next == NULL)
		{
			struct PAllocLocation* new_location = malloc(sizeof(struct PAllocLocation));
			new_location->count = 0;
			new_location->file = file;
			new_location->line = line;
			new_location->prev = it;
			new_location->next = NULL;
			it->next = new_location;
			block->count = new_location->count++;

			return;
		}
		it = it->next;
	}
}

void mp_resize(int direction)
{
	MSG("Resizing");
	size_t old_size = phashtable.size;
	if (direction == 1)
		phashtable.size *= 2;
	else if (direction == -1)
		phashtable.size /= 2;
	else
		return;

	struct MemBlock** old_items = phashtable.items;
	phashtable.items = calloc(phashtable.size, sizeof(struct MemBlock*));

	// Count will be reincreased when reinserting items
	phashtable.count = 0;
	// Rehash and insert
	for (size_t i = 0; i < old_size; i++)
	{
		struct MemBlock* it = old_items[i];
		// Save the next since it will be cleared in the iterator when inserting
		struct MemBlock* next = NULL;
		while (it)
		{
			next = it->next;
			mp_insert(it, NULL, 0);
			it = next;
		}
	}
	free(old_items);
}

struct MemBlock* mp_search(void* ptr)
{
	size_t hash = mp_hash_ptr(ptr);
	struct MemBlock* it = phashtable.items[hash];

	// Search chain for the correct pointer
	while (it)
	{
		if (it->bytes == ptr)
		{
			return it;
		}
		it = it->next;
	}
	return NULL;
}

struct MemBlock* mp_remove(void* ptr)
{
	size_t hash = mp_hash_ptr(ptr);
	struct MemBlock* it = phashtable.items[hash];
	struct MemBlock* prev = NULL;

	// Search chain for the correct pointer
	while (it)
	{
		if (it->bytes == ptr)
		{
			// Bucket gets removed, no more left in chain
			if (it->next == NULL)
				phashtable.count--;
			if (prev) // Has a parent remove and reconnect chain
			{
				prev->next = it->next;
			}
			else // First one one chain, change head
			{
				phashtable.items[hash] = it->next;
			}

			// Check for resize down
			if (phashtable.count - 1 <= phashtable.size * 0.4)
			{
				mp_resize(-1);
			}

			return it;
		}
		prev = it;
		it = it->next;
	}
	return NULL;
}