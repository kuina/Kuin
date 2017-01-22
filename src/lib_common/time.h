#pragma once

#include "..\common.h"

// 'time'
EXPORT S64 _sys(void);
EXPORT S64 _now(void);
EXPORT S64 _intToDate(S64 time, S64* year, S64* month, S64* day, S64* hour, S64* minute, S64* second);
EXPORT S64 _dateToInt(S64 year, S64 month, S64 day, S64 hour, S64 minute, S64 second);
EXPORT void sleep(S64 ms);
