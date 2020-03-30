#undef MAGPIE_REPLACE_STD
#include "magpie.h"
#include <stdio.h>
#include <string.h>

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
	struct MemBlock* parent;
	struct MemBlock* less;
	struct MemBlock* more;
};

struct MemBlock* blocks = NULL;

void mp_insert(struct MemBlock* block);

// Searches for the pointer in the tree
struct MemBlock* mp_search(struct MemBlock* head, void* ptr);

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
	new_block->file = file;
	new_block->line = line;
	new_block->parent = NULL;
	new_block->less = NULL;
	new_block->more = NULL;

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
	new_block->parent = NULL;
	new_block->less = NULL;
	new_block->more = NULL;
	// Insert
	mp_insert(new_block);

	return new_block->ptr;
}
void* mp_realloc(void* ptr, size_t size, const char* file, uint32_t line);

void mp_free(void* ptr, const char* file, uint32_t line)
{
	struct MemBlock* block = mp_search(blocks, ptr);
	if (block == NULL)
	{
		char msg[1024];
		snprintf(msg, sizeof msg, "%s:%u Freeing invalid pointer with adress %p", file, line, ptr);
		MSG(msg);
	}
	alloc_count-;
	alloc_size -= block->size;
	// Remove from tree
	// Detach
	struct MemBlock* parent = block->parent;
	if (parent == NULL)
	{
		blocks = NULL;
	}
	else if (parent->less == block)
	{
		parent->less = NULL;
	}
	else if (parent->more == block)
	{
		parent->more = NULL;
	}
	// Reattach children
	mp_insert(block->less);
	mp_insert(block->more);

	free(block);
	free(ptr);
}

void mp_insert(struct MemBlock* block)
{
	if (block == NULL)
	{
		return;
	}
	if (blocks == NULL)
	{
		blocks = block;
		block->parent = NULL;
		return;
	}
	struct MemBlock* it = blocks;
	// Traverse to leaf
	while (it)
	{
		if (block->line < it->line)
		{
			if (it->less == NULL)
			{
				MSG("less");
				block->parent = it;
				it->less = block;
				break;
			}
			it = it->less;
		}
		else if (block->line > it->line)
		{
			if (it->more == NULL)
			{
				MSG("more");
				block->parent = it;
				it->more = block;
				break;
			}
			it = it->more;
		}
		// Allocated on the same line, not necessarily file
		else
		{
			// Same file
			// Traverse to more leaf
			if (block->parent == NULL && strcmp(block->file, it->file) == 0)
			{
				block->num++;
			}
			// Traverse to more if equal line count
			if (it->more == NULL)
			{
				MSG("dup");
				block->parent = it;
				it->more = block;
				break;
			}
			it = it->more;
		}
	}

	// Insert block's children as well
	// Used by mp_free
	mp_insert(block->less);
	mp_insert(block->more);
}

struct MemBlock* mp_search(struct MemBlock* head, void* ptr)
{
	if (head->ptr == ptr)
		return head;
	struct MemBlock* result = NULL;
	if (head->less)
		result = mp_search(head->less, ptr);
	if (result == NULL && head->more)
	{
		result = mp_search(head->more, ptr);
	}
	return result;
}