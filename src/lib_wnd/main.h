#pragma once

#include "..\common.h"

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT void _fin(void);
EXPORT Bool _act(void);
