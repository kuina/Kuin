#include "lib.h"

#include <time.h>

EXPORT double _acos(double x)
{
	return acos(x);
}

EXPORT double _acosh(double x)
{
	return acosh(x);
}

EXPORT U64 _addr(SClass* me_)
{
	return (U64)me_;
}

EXPORT double _asin(double x)
{
	return asin(x);
}

EXPORT double _asinh(double x)
{
	return asinh(x);
}

EXPORT double _atan(double x)
{
	return atan(x);
}

EXPORT double _atanh(double x)
{
	return atanh(x);
}

EXPORT double _ceil(double x)
{
	return ceil(x);
}

EXPORT void* _cmdLine(void)
{
	int num;
	Char** cmds = CommandLineToArgvW(GetCommandLine(), &num);
	ASSERT(num >= 1);
	int i;
	void** ptr;
	U8* result = (U8*)AllocMem(0x10 + sizeof(void**) * (size_t)(num - 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)(num - 1);
	ptr = (void**)(result + 0x10);
	for (i = 1; i < num; i++)
	{
		size_t len = wcslen(cmds[i]);
		U8* item = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)item)[0] = 1;
		((S64*)item)[1] = (S64)len;
		memcpy(item + 0x10, cmds[i], sizeof(Char) * (len + 1));
		*ptr = item;
		ptr++;
	}
	return result;
}

EXPORT double _cos(double x)
{
	return cos(x);
}

EXPORT double _cosh(double x)
{
	return cosh(x);
}

EXPORT double _dist(double x, double y, double centerX, double centerY)
{
	return hypot(x - centerX, y - centerY);
}

EXPORT double _exp(double x)
{
	return exp(x);
}

EXPORT double _floor(double x)
{
	return floor(x);
}

EXPORT double _invRot(double x, double y, double centerX, double centerY)
{
	double rad = atan2(y - centerY, x - centerX);
	return rad < 0.0 ? rad + 2.0 * M_PI : rad;
}

EXPORT double _ln(double x)
{
	return log(x);
}

EXPORT S64 _now(void)
{
	return _time64(NULL);
}

EXPORT double _sin(double x)
{
	return sin(x);
}

EXPORT double _sinh(double x)
{
	return sinh(x);
}

EXPORT void __sleep(S64 ms)
{
	THROWDBG(ms < 0, 0xe9170006);
	S64 i;
	for (i = 0; i < ms / 10000; i++)
		Sleep(10000);
	Sleep((DWORD)(ms % 10000));
}

EXPORT double _sqrt(double x)
{
	return sqrt(x);
}

EXPORT S64 _sysTime(void)
{
	return (S64)timeGetTime();
}

EXPORT double _tan(double x)
{
	return tan(x);
}

EXPORT double _tanh(double x)
{
	return tanh(x);
}

EXPORT U64 _toBit64Forcibly(double x)
{
	return *(U64*)&x;
}

EXPORT double _toFloatForcibly(U64 x)
{
	return *(double*)&x;
}
