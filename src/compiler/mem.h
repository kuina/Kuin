#pragma once

#include "..\common.h"

void SetAllocator(void*(*allocator)(size_t size));
void* Alloc(size_t size);
