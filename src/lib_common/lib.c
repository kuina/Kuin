#include "lib.h"

#include "rnd.h"

typedef struct SBmSearch
{
	SClass Class;
	Char* Pattern;
	int* Dists;
	int PatternLen;
} SBmSearch;

static SRndState GlobalRnd;

EXPORT void* _cmdLine(void)
{
	int num;
	Char** cmds = CommandLineToArgvW(GetCommandLine(), &num);
	ASSERT(num >= 1);
	{
		int i;
		void** ptr;
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char*) * (size_t)(num - 1));
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
}

EXPORT S64 _rnd(S64 min, S64 max)
{
	THROWDBG(min > max, 0xe9170006);
	return RndGet(&GlobalRnd, min, max);
}

EXPORT double _rndFloat(double min, double max)
{
	THROWDBG(min >= max, 0xe9170006);
	return RndGetFloat(&GlobalRnd, min, max);
}

EXPORT U64 _rndBit64(void)
{
	return RndGetBit64(&GlobalRnd);
}

EXPORT void* _rndUuid(void)
{
	return RndGetUuid(&GlobalRnd);
}

EXPORT double _cos(double x)
{
	return cos(x);
}

EXPORT double _sin(double x)
{
	return sin(x);
}

EXPORT double _tan(double x)
{
	return tan(x);
}

EXPORT double _acos(double x)
{
	return acos(x);
}

EXPORT double _asin(double x)
{
	return asin(x);
}

EXPORT double _atan(double x)
{
	return atan(x);
}

EXPORT double _cosh(double x)
{
	return cosh(x);
}

EXPORT double _sinh(double x)
{
	return sinh(x);
}

EXPORT double _tanh(double x)
{
	return tanh(x);
}

EXPORT double _acosh(double x)
{
#if defined(VS2015)
	return acosh(x);
#else
	UNUSED(x);
	ASSERT(True);
	return 0.0;
#endif
}

EXPORT double _asinh(double x)
{
#if defined(VS2015)
	return asinh(x);
#else
	UNUSED(x);
	ASSERT(True);
	return 0.0;
#endif
}

EXPORT double _atanh(double x)
{
#if defined(VS2015)
	return atanh(x);
#else
	UNUSED(x);
	ASSERT(True);
	return 0.0;
#endif
}

EXPORT double _sqrt(double x)
{
	return sqrt(x);
}

EXPORT double _exp(double x)
{
	return exp(x);
}

EXPORT double _ln(double x)
{
	return log(x);
}

EXPORT double _log(double base, double x)
{
	return log(x) / log(base);
}

EXPORT double _floor(double x)
{
	return floor(x);
}

EXPORT double _ceil(double x)
{
	return ceil(x);
}

EXPORT double _round(double x, S64 precision)
{
	// Round half away from zero.
	if (precision == 0)
	{
		if (x >= 0.0)
			return floor(x + 0.5);
		return -floor(-x + 0.5);
	}
	else
	{
		double p = pow(10.0, (double)precision);
		if (x >= 0.0)
			return floor(x * p + 0.5) / p;
		return -floor(-x * p + 0.5) / p;
	}
}

EXPORT void _rot(double* x, double* y, double centerX, double centerY, double angle)
{
	double x2 = *x - centerX;
	double y2 = *y - centerY;
	double cos_theta = cos(angle);
	double sin_theta = sin(angle);
	double x3 = x2 * cos_theta - y2 * sin_theta;
	double y3 = x2 * sin_theta + y2 * cos_theta;
	*x = x3 + centerX;
	*y = y3 + centerY;
}

EXPORT double _invRot(double x, double y, double centerX, double centerY)
{
	double rad = atan2(y - centerY, x - centerX);
	return rad < 0.0 ? rad + 2.0 * M_PI : rad;
}

EXPORT double _dist(double x, double y, double centerX, double centerY)
{
	return hypot(x - centerX, y - centerY);
}

EXPORT Bool _chase(double* x, double target, double vel)
{
	if (*x == target)
		return True;
	if (*x < target)
	{
		*x += vel;
		if (*x >= target)
		{
			*x = target;
			return True;
		}
	}
	else
	{
		*x -= vel;
		if (*x <= target)
		{
			*x = target;
			return True;
		}
	}
	return False;
}

EXPORT Bool _same(double n1, double n2)
{
	U64 i1 = *(U64*)&n1;
	U64 i2 = *(U64*)&n2;
	S64 diff;
	if ((i1 >> 63) != (i2 >> 63))
		return n1 == n2;
	diff = (S64)(i1 - i2);
	return -24 <= diff && diff <= 24;
}

EXPORT double _toRad(double degree)
{
	return degree * M_PI / 180.0;
}

EXPORT double _toDegree(double rad)
{
	return rad * 180.0 / M_PI;
}

EXPORT S64 _cmp(const U8* s1, const U8* s2)
{
	THROWDBG(s1 == NULL, 0xc0000005);
	THROWDBG(s2 == NULL, 0xc0000005);
	S64 result = (S64)wcscmp((const Char*)(s1 + 0x10), (const Char*)(s2 + 0x10));
	return result > 0 ? 1 : (result < 0 ? -1 : 0);
}

EXPORT S64 _cmpEx(const U8* s1, const U8* s2, S64 len, Bool ignoreCase)
{
	THROWDBG(s1 == NULL, 0xc0000005);
	THROWDBG(s2 == NULL, 0xc0000005);
	S64 result;
	if (ignoreCase)
		result = (S64)_wcsnicmp((const Char*)(s1 + 0x10), (const Char*)(s2 + 0x10), (size_t)len);
	else
		result = (S64)wcsncmp((const Char*)(s1 + 0x10), (const Char*)(s2 + 0x10), (size_t)len);
	return result > 0 ? 1 : (result < 0 ? -1 : 0);
}

EXPORT S64 _findStr(const U8* text, const U8* pattern)
{
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(pattern == NULL, 0xc0000005);
	const Char* result = wcsstr((const Char*)(text + 0x10), (const Char*)(pattern + 0x10));
	return result == NULL ? -1 : (S64)(result - (const Char*)(text + 0x10));
}

EXPORT S64 _findStrLast(const U8* text, const U8* pattern)
{
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(pattern == NULL, 0xc0000005);
	S64 len1 = ((const S64*)text)[1];
	S64 len2 = ((const S64*)pattern)[1];
	const Char* ptr1 = (const Char*)(text + 0x10);
	const Char* ptr2 = (const Char*)(pattern + 0x10);
	S64 i;
	for (i = len1 - len2; i >= 0; i--)
	{
		if (wcsncmp(ptr1 + i, ptr2, (size_t)len2) == 0)
			return i;
	}
	return -1;
}

EXPORT S64 _findStrEx(const U8* text, const U8* pattern, S64 start, Bool fromLast, Bool ignoreCase, Bool wholeWord)
{
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(pattern == NULL, 0xc0000005);
	S64 len1 = ((const S64*)text)[1];
	S64 len2 = ((const S64*)pattern)[1];
	const Char* ptr1 = (const Char*)(text + 0x10);
	const Char* ptr2 = (const Char*)(pattern + 0x10);
	S64 i;
	int(*func)(const Char*, const Char*, size_t) = ignoreCase ? _wcsnicmp : wcsncmp;
	THROWDBG(start < -1 || len1 <= start, 0xe9170006);
	if (fromLast)
	{
		if (start == -1 || start > len1 - len2)
			start = len1 - len2;
		for (i = start; i >= 0; i--)
		{
			if (func(ptr1 + i, ptr2, (size_t)len2) == 0)
			{
				if (wholeWord)
				{
					if (i > 0)
					{
						Char c = ptr1[i - 1];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
					if (i + len2 < len1)
					{
						Char c = ptr1[i + len2];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
				}
				return i;
			}
		}
	}
	else
	{
		if (start == -1)
			start = 0;
		for (i = start; i <= len1 - len2; i++)
		{
			if (func(ptr1 + i, ptr2, (size_t)len2) == 0)
			{
				if (wholeWord)
				{
					if (i > 0)
					{
						Char c = ptr1[i - 1];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
					if (i + len2 < len1)
					{
						Char c = ptr1[i + len2];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
				}
				return i;
			}
		}
	}
	return -1;
}

EXPORT SClass* _makeBmSearch(SClass* me_, const U8* pattern)
{
	THROWDBG(pattern == NULL, 0xc0000005);
	SBmSearch* me2 = (SBmSearch*)me_;
	{
		size_t len = (size_t)((S64*)pattern)[1];
		me2->PatternLen = (int)len;
		me2->Pattern = (Char*)AllocMem(sizeof(Char) * (len + 1));
		memcpy(me2->Pattern, pattern + 0x10, sizeof(Char) * (len + 1));
	}
	{
		int i, j;
		me2->Dists = (int*)AllocMem(sizeof(int) * (size_t)me2->PatternLen);
		for (i = me2->PatternLen - 1; i >= 0; i--)
		{
			int dist = me2->PatternLen - i - 1;
			Bool found = False;
			for (j = me2->PatternLen - 1; j > i; j--)
			{
				if (me2->Pattern[i] == me2->Pattern[j])
				{
					me2->Dists[i] = me2->Dists[j];
					found = True;
					break;
				}
			}
			if (found)
				continue;
			me2->Dists[i] = dist;
		}
	}
	return me_;
}

EXPORT void _bmSearchDtor(SClass* me_)
{
	SBmSearch* me2 = (SBmSearch*)me_;
	FreeMem(me2->Pattern);
	FreeMem(me2->Dists);
}

EXPORT S64 _bmSearchFind(SClass* me_, const U8* text)
{
	int text_len = (int)((S64*)text)[1];
	const Char* text2 = (const Char*)(text + 0x10);
	SBmSearch* me2 = (SBmSearch*)me_;
	int i = me2->PatternLen - 1;
	while (i < text_len)
	{
		int p = me2->PatternLen - 1;
		while (p >= 0 && i < text_len)
		{
			if (text2[i] == me2->Pattern[p])
			{
				i--;
				p--;
			}
			else
				break;
		}
		if (p < 0)
			return i + 1;
		int shift1 = me2->Dists[p];
		int shift2 = me2->PatternLen - p;
		i += shift1 > shift2 ? shift1 : shift2;
	}
	return -1;
}

void LibInit(void)
{
	// Initialize the random number system.
	InitRndMask();
#if defined(_DEBUG)
	// Test the random number system.
	{
		const U64 answers[] =
		{
			0x33ff358570beb516, 0xa09f66b21c23687b, 0x34506b19caf13173, 0x47bbd348fd8e122f,
			0xcb2fb52e99922f80, 0x7b633b3d7230b48d, 0xd7bb5c4f79c6886b, 0x27b2d2079e86b7da,
			0xb801da316661da6a, 0xfb20bf53344a71c4, 0xdd26c89a30d3fefe, 0x4d291ef4ed2381d5,
			0x03a063c847570621, 0x803d64a732ebe145, 0xf7d6f2d0bc4906be, 0xf44552c12646cd84,
			0x0a4df6f031fb46b3, 0x894ebd381ff0c2a8, 0x34d2221d79c8b86e, 0xf68cf4fdd4f5a265,
			0xa6dd0d5bdf172c87, 0xc3bd1fd6a7702d36, 0xb8e8c39722379c82, 0xa8e0c595ed87c7f6,
			0x368d2b6111065b24, 0x57ae24e8fc53eefc, 0xe80e4d6647ace128, 0xa323547aa04421b2,
			0x5cf311d7ea76c8c3, 0xa160420e0f3bf087, 0x31c91cf6323bcaf2, 0x341fd6794ac66886,
		};
		int i;
		RndInit(&GlobalRnd, 917);
		for (i = 0; i < sizeof(answers) / sizeof(answers[0]); i++)
		{
			U64 n = RndGetBit64(&GlobalRnd);
			ASSERT(n == answers[i]);
		}
	}
#endif
	RndInit(&GlobalRnd, (U32)(time(NULL)) ^ (U32)timeGetTime() ^ 0x2971c37b);
}
