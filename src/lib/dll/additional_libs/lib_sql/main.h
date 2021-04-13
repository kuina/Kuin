#pragma once

#include "..\..\common.h"

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT Bool _sqlApply(SClass* me_);
EXPORT void* _sqlErrMsg(SClass* me_);
EXPORT Bool _sqlExec(SClass* me_, const void* cmd);
EXPORT void _sqlFin(SClass* me_);
EXPORT void* _sqlGetBlob(SClass* me_, S64 col);
EXPORT double _sqlGetFloat(SClass* me_, S64 col);
EXPORT S64 _sqlGetInt(SClass* me_, S64 col);
EXPORT void* _sqlGetStr(SClass* me_, S64 col);
EXPORT void _sqlNext(SClass* me_);
EXPORT Bool _sqlPrepare(SClass* me_, const void* cmd);
EXPORT Bool _sqlSetBlob(SClass* me_, S64 idx, const void* value);
EXPORT Bool _sqlSetFloat(SClass* me_, S64 idx, double value);
EXPORT Bool _sqlSetInt(SClass* me_, S64 idx, S64 value);
EXPORT Bool _sqlSetStr(SClass* me_, S64 idx, const void* value);
EXPORT Bool _sqlTerm(SClass* me_);
EXPORT SClass* _makeSql(SClass* me_, const U8* path);
