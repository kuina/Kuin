#pragma once

#include "..\common.h"

EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col));
EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col));
EXPORT void Version(int* major, int* minor, int* micro);
EXPORT void InitMemAllocator(void);
EXPORT void FinMemAllocator(void);
EXPORT void ResetMemAllocator(void);
