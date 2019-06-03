#pragma once

#include "..\common.h"

EXPORT void InitCompiler(S64 lang);
EXPORT void FinCompiler(void);
EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, const void* related_files, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang, S64 app_code);
EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, const void* related_files, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang, S64 app_code, Bool not_deploy);
EXPORT void Interpret1(void* src, S64 line, void* me, void* replace_func, S64 cursor_x, S64 cursor_y, S64* new_cursor_x, S64 old_line, S64 new_line);
EXPORT Bool Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang);
EXPORT void Version(S64* major, S64* minor, S64* micro);
EXPORT void ResetMemAllocator(void);
EXPORT void* GetKeywords(void* src, const U8* src_name, S64 x, S64 y, void* callback);
EXPORT Bool RunDbg(const U8* path, const U8* cmd_line, void* idle_func, void* event_func, void* break_points_func, void* break_func, void* dbg_func);
EXPORT void SetBreakPoints(const void* break_points);
EXPORT Bool Archive(const U8* dst, const U8* src, S64 app_code);
