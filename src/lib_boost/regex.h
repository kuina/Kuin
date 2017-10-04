#pragma once

#include "..\common.h"

EXPORT_CPP void* _find(S64* pos, const U8* text, const U8* pattern);
EXPORT_CPP void* _match(S64* pos, const U8* text, const U8* pattern);
EXPORT_CPP void* _all(U8** pos, const U8* text, const U8* pattern);
