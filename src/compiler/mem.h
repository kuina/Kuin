#pragma once

#include "..\common.h"

void* Alloc(size_t size);
void InitAllocator(S64 mem_buf_num);
void FinAllocator(void);
void ResetAllocator(S64 idx);
