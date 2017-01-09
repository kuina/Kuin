#pragma once

#include "..\common.h"

// 'file'
EXPORT void _streamFin(SClass* me_);
EXPORT S64 _streamGetPos(SClass* me_);
EXPORT void _streamSetPos(SClass* me_, S64 origin, S64 pos);
EXPORT void* _streamRead(SClass* me_, S64 size);
EXPORT void _streamWrite(SClass* me_, void* bin);
EXPORT S64 _streamFileSize(SClass* me_);
EXPORT Bool _streamTerm(SClass* me_);
EXPORT SClass* _makeReader(SClass* me_, const U8* path);
EXPORT SClass* _makeWriter(SClass* me_, const U8* path, Bool append);
