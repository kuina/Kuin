#include "mem.h"

static void*(*Allocator)(size_t size) = NULL;

void SetAllocator(void*(*allocator)(size_t size))
{
	Allocator = allocator;
}

void* Alloc(size_t size)
{
	return Allocator(size);
}
