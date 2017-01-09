#pragma once

#include "..\common.h"

// 'file'
EXPORT void _streamFin(SClass* me_);
EXPORT S64 _streamFileSize(SClass* me_);
EXPORT SClass* _loadReader(SClass* me_, const U8* path);
