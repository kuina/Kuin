#pragma once

#include "..\common.h"

void* Alloc(size_t size);
void InitAllocator(void);
void FinAllocator(void);
void ResetAllocator(void);
