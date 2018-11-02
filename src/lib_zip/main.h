#pragma once

#include "..\common.h"

#pragma comment(lib, "zlibstat.lib")

#include <zlib.h>

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT Bool _zip(const U8* out_path, const U8* path, S64 compression_level);
EXPORT Bool _unzip(const U8* out_path, const U8* path);
