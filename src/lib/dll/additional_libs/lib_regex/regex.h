#pragma once

#include "..\..\common.h"

EXPORT_CPP void _regexDtor(SClass* me_);
EXPORT_CPP void* _regexFind(SClass* me_, S64* pos, const U8* text, S64 start);
EXPORT_CPP void* _regexFindAll(SClass* me_, U8** pos, const U8* text);
EXPORT_CPP void* _regexFindLast(SClass* me_, S64* pos, const U8* text, S64 start);
EXPORT_CPP void* _regexMatch(SClass* me_, const U8* text);
EXPORT_CPP void* _regexReplace(SClass* me_, const U8* text, const U8* newText, Bool all);
EXPORT_CPP SClass* _makeRegex(SClass* me_, const U8* pattern);
