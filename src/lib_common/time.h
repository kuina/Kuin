#pragma once

#include "..\common.h"

EXPORT S64 _sysTime(void);
EXPORT S64 _now(void);
EXPORT S64 _intToDate(S64 time, S64* year, S64* month, S64* day, S64* hour, S64* minute, S64* second);
EXPORT S64 _dateToInt(S64 year, S64 month, S64 day, S64 hour, S64 minute, S64 second);
EXPORT S64 _intToLocalDate(S64 time, S64* year, S64* month, S64* day, S64* hour, S64* minute, S64* second);
EXPORT S64 _localDateToInt(S64 year, S64 month, S64 day, S64 hour, S64 minute, S64 second);
EXPORT void sleep(S64 ms);
