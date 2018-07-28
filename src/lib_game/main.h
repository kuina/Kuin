#pragma once

#include "..\common.h"

// 'game'
EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const Char* app_name);
EXPORT SClass* _makeRect(SClass* me_, double x, double y, double width, double height, double weight);
EXPORT void _rectMove(SClass* me_);
EXPORT void _rectUpdate(SClass* me_);
EXPORT SClass* _makeMapImpl(SClass* me_, S64 width, S64 height, const void* data);
EXPORT SClass* _makeMapEmpty(SClass* me_, S64 width, S64 height);
EXPORT void _mapDtor(SClass* me_);
EXPORT void _hitMapRect(SClass* map, SClass* rect);
