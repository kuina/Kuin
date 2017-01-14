#pragma once

#include "..\common.h"

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT_CPP void _fin();
EXPORT_CPP Bool _act();
EXPORT_CPP SClass* _makeWnd(SClass* me_, S64 style, S64 width, S64 height);
