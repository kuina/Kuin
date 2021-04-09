#pragma once

#include "..\common.h"

EXPORT void _httpFin(SClass* me_);
EXPORT void* _httpGet(SClass* me_);
EXPORT SClass* _makeHttp(SClass* me_, const U8* url, Bool post, const U8* agent);
