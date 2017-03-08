#pragma once

#include "..\common.h"

EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void*(*allocator)(size_t size), void(*log_func)(const Char* code, const Char* msg, const Char* src, int row, int col));
EXPORT void Version(int* major, int* minor, int* micro);
