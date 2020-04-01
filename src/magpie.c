#undef MAGPIE_REPLACE_STD
#include "magpie.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

// The number of allocated blocks of memory
size_t alloc_count = 0;
// The number of bytes allocated
size_t alloc_size = 0;

#define BUFFER_PAD 3

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
	void* ptr;
	size_t size;
	const char* file;
	uint32_t line;
	size_t num;
	struct MemBlock* next;
};

struct PHashTable
{
	// Describes the allocated amount of buckets in the hash table
	size_t size;
	// Describes how many buckets are in use
	size_t count;
	struct MemBlock** items;
};

static struct PHashTable phashtable = {0};

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
void mp_insert(struct MemBlock* block);

// Reinserts a block without addng to count and discards chain
void mp_reinsert(struct MemBlock* block);

// Searches for the pointer in the tree
struct MemBlock* mp_search(void* ptr);

// Searches and removes a memblock from the hashmap
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

int mp_terminate()
{
	char msg[1024];
	int remaining_blocks = 0;

	for (size_t i = 0, j = phashtable.count; i < phashtable.size; i++)
	{
		struct MemBlock* it = phashtable.items[i];
		while (it)
		{
			j--;
			snprintf(msg, sizeof msg,
					 "Memory block allocated at %s:%u with a size of %zu bytes has not been freed. Block was "
					 "allocation num %zu",
					 it->file, it->line, it->size, it->num);
			MSG(msg);
			it = it->next;
		}
	}
	return remaining_blocks;
}

void* mp_malloc(size_t size, const char* file, uint32_t line)
{
	struct MemBlock* new_block = malloc(sizeof(struct MemBlock));

	// Allocate request
	new_block->ptr = malloc(size + BUFFER_PAD);
	if (new_block->ptr == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%d Failed to allocate memory for %zu bytes", file, line, size);
		MSG(msg);
		return NULL;
	}
	alloc_count++;
	alloc_size += size;
	new_block->size += size;
	new_block->num = alloc_count;
	new_block->file = file;
	new_block->line = line;
	new_block->next = NULL;

	// Insert
	mp_insert(new_block);

	return new_block->ptr;
}
void* mp_calloc(size_t num, size_t size, const char* file, uint32_t line)
{
	struct MemBlock* new_block = malloc(sizeof(struct MemBlock));

	// Allocate request
	new_block->ptr = calloc(num + 1, size);
	if (new_block->ptr == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%u Failed to allocate memory for %zu bytes", file, line, size * num);
		MSG(msg);
		return NULL;
	}
	alloc_count++;
	alloc_size += num * size;
	new_block->size = size;
	new_block->file = file;
	new_block->line = line;
	new_block->next = NULL;
	// Insert
	mp_insert(new_block);

	return new_block->ptr;
}
void* mp_realloc(void* ptr, size_t size, const char* file, uint32_t line);

void mp_free(void* ptr, const char* file, uint32_t line)
{
	static int i = 0;
	i++;
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

	// TODO check overflow
	free(ptr);
	free(block);
}

void mp_insert(struct MemBlock* block)
{
	block->next = NULL;
	// Hash the pointer
	if (phashtable.size == 0)
	{
		phashtable.size = 4;
		phashtable.items = calloc(phashtable.size, sizeof(*phashtable.items));
	}
	if (phashtable.count >= phashtable.size * 0.7)
	{
		MSG("Resizing");
		size_t old_size = phashtable.size;
		phashtable.size *= 2;

		struct MemBlock** old_items = phashtable.items;
		phashtable.items = calloc(phashtable.size, sizeof(*phashtable.items));

		// Count will be reincreased when reinserting items
		phashtable.count = 0;
		// Rehash and insert
		for (size_t i = 0; i < old_size; i++)
		{
			struct MemBlock* it = old_items[i];
			// Save the next since it will be cleared in the iterator when inserting
			struct MemBlock* next;
			while (it)
			{
				next = it->next;
				mp_insert(it);
				it = next;
			}
		}
		free(old_items);
	}
	size_t hash = mp_hash_ptr(block->ptr);
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

struct MemBlock* mp_search(void* ptr)
{
	size_t hash = mp_hash_ptr(ptr);
	struct MemBlock* it = phashtable.items[hash];
	while (it)
	{
		if (it->ptr == ptr)
		{
			return ptr;
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
		if (it->ptr == ptr)
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
			return it;
		}
		prev = it;
		it = it->next;
	}
	return NULL;
}