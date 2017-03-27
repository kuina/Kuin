#include "time.h"

EXPORT S64 _sysTime(void)
{
	return (S64)timeGetTime();
}

EXPORT S64 _now(void)
{
	return _time64(NULL);
}

EXPORT S64 _intToDate(S64 time, S64* year, S64* month, S64* day, S64* hour, S64* minute, S64* second)
{
	const struct tm* t;
	const __time64_t time2 = time;
	t = _gmtime64(&time2);
	*year = (S64)t->tm_year + 1900;
	*month = (S64)t->tm_mon + 1;
	*day = (S64)t->tm_mday;
	*hour = (S64)t->tm_hour;
	*minute = (S64)t->tm_min;
	*second = (S64)t->tm_sec;
	return (S64)t->tm_wday;
}

EXPORT S64 _dateToInt(S64 year, S64 month, S64 day, S64 hour, S64 minute, S64 second)
{
	struct tm t;
	t.tm_year = (int)year - 1900;
	t.tm_mon = (int)month - 1;
	t.tm_mday = (int)day;
	t.tm_hour = (int)hour;
	t.tm_min = (int)minute;
	t.tm_sec = (int)second;
	return _mkgmtime64(&t);
}

EXPORT void sleep(S64 ms)
{
	Sleep((DWORD)ms);
}
