#pragma once

#include "..\common.h"

EXPORT void InitCompiler(S64 mem_num, S64 lang);
EXPORT void FinCompiler(void);
EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang);
EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang);
EXPORT void Interpret1(const void* src, const void* color);
EXPORT void Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang);
EXPORT void Interpret3(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env);
EXPORT void Version(S64* major, S64* minor, S64* micro);
EXPORT void ResetMemAllocator(S64 mem_idx);
EXPORT void* GetHint(const U8* src, S64 row, S64 col);
EXPORT Bool RunDbg(const U8* path, const U8* cmd_line, void* idle_func, void* event_func);
