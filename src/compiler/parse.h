#pragma once

#include "..\common.h"

#include "dict.h"
#include "option.h"

SDict* Parse(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const SOption* option, U8* use_res_flags);
Bool InterpretImpl1(void* str, void* color, void* comment_level, void* flags, S64 line, void* me, void* replace_func, S64 cursor_x, S64 cursor_y, S64* new_cursor_x, S64 old_line, S64 new_line);
void* GetKeywordsRoot(const Char** str, const Char* end, const Char* src_name, int x, int y, U64 flags, void* callback, int keyword_list_num, const void* keyword_list);
void GetDbgVars(int keyword_list_num, const void* keyword_list, const Char* pos_name, int pos_row, HANDLE process_handle, U64 start_addr, const CONTEXT* context, void* callback);
