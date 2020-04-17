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
}

EXPORT S64 _rnd(S64 min, S64 max)
{
	THROWDBG(max - min < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	return RndGet(&GlobalRnd, min, max);
}

EXPORT double _rndFloat(double min, double max)
{
	THROWDBG(min >= max, EXCPT_DBG_ARG_OUT_DOMAIN);
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
	THROWDBG(s1 == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(s2 == NULL, EXCPT_ACCESS_VIOLATION);
	S64 result = (S64)wcscmp((const Char*)(s1 + 0x10), (const Char*)(s2 + 0x10));
	return result > 0 ? 1 : (result < 0 ? -1 : 0);
}

EXPORT S64 _cmpEx(const U8* s1, const U8* s2, S64 len, Bool ignoreCase)
{
	THROWDBG(s1 == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(s2 == NULL, EXCPT_ACCESS_VIOLATION);
	S64 result;
	if (ignoreCase)
		result = (S64)_wcsnicmp((const Char*)(s1 + 0x10), (const Char*)(s2 + 0x10), (size_t)len);
	else
		result = (S64)wcsncmp((const Char*)(s1 + 0x10), (const Char*)(s2 + 0x10), (size_t)len);
	return result > 0 ? 1 : (result < 0 ? -1 : 0);
}

EXPORT S64 _minInt(S64 n1, S64 n2)
{
	return n1 < n2 ? n1 : n2;
}

EXPORT double _minFloat(double n1, double n2)
{
	return n1 < n2 ? n1 : n2;
}

EXPORT S64 _maxInt(S64 n1, S64 n2)
{
	return n1 > n2 ? n1 : n2;
}

EXPORT double _maxFloat(double n1, double n2)
{
	return n1 > n2 ? n1 : n2;
}

EXPORT S64 _levenshtein(const U8* s1, const U8* s2)
{
	THROWDBG(s1 == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(s2 == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len1 = ((S64*)s1)[1];
	S64 len2 = ((S64*)s2)[1];
	const Char* str1 = (const Char*)(s1 + 0x10);
	const Char* str2 = (const Char*)(s2 + 0x10);
	S64* buf = (S64*)AllocMem(sizeof(S64) * (size_t)(len1 + 1) * (size_t)(len2 + 1));
	S64 min_value;
	S64 value;
	S64 i;
	S64 j;
	for (i = 0; i < len1 + 1; i++)
		buf[i * (len2 + 1)] = i;
	for (i = 1; i < len2 + 1; i++)
		buf[i] = i;
	for (i = 1; i < len1 + 1; i++)
	{
		for (j = 1; j < len2 + 1; j++)
		{
			min_value = buf[(i - 1) * (len2 + 1) + j] + 1;
			value = buf[i * (len2 + 1) + (j - 1)] + 1;
			if (min_value > value)
				min_value = value;
			value = buf[(i - 1) * (len2 + 1) + (j - 1)] + (str1[i - 1] == str2[j - 1] ? 0 : 1);
			if (min_value > value)
				min_value = value;
			buf[i * (len2 + 1) + j] = min_value;
		}
	}
	value = buf[len1 * (len2 + 1) + len2];
	FreeMem(buf);
	return value;
}

EXPORT double _lerp(double first, double last, double rate)
{
	if (rate < 0.0)
		rate = 0.0;
	else if (rate > 1.0)
		rate = 1.0;
	return first * (1.0 - rate) + last * rate;
}

EXPORT double _qerp(double first, double last, Bool easeIn, double rate)
{
	if (rate < 0.0)
		rate = 0.0;
	else if (rate > 1.0)
		rate = 1.0;
	double rate2 = easeIn ? (rate * rate) : (rate * (2.0 - rate));
	return first * (1.0 - rate2) + last * rate2;
}

EXPORT double _cerp(double first, double last, double rate)
{
	if (rate < 0.0)
		rate = 0.0;
	else if (rate > 1.0)
		rate = 1.0;
	double rate2 = rate * rate * (3.0 - 2.0 * rate);
	return first * (1.0 - rate2) + last * rate2;
}

EXPORT double _hermite(const void* pos, double rate)
{
	THROWDBG(pos == NULL, EXCPT_ACCESS_VIOLATION);
	int len = (int)((S64*)pos)[1];
	double len2 = (double)len;
	const double* pos2 = (const double*)((const U8*)pos + 0x10);
	if (rate < 0.0)
		rate = 0.0;
	else if (rate > len2 - 1.0)
		rate = len2 - 1.0;
	if (len <= 1)
		return len == 1 ? pos2[0] : 0.0f;
	int idx = (int)rate;
	int idx_minus1 = idx - 1;
	if (idx_minus1 < 0)
		idx_minus1 = 0;
	int idx_plus1 = idx + 1;
	if (idx_plus1 > len - 1)
		idx_plus1 = len - 1;
	int idx_plus2 = idx + 2;
	if (idx_plus2 > len - 1)
		idx_plus2 = len - 1;
	double dx0 = (pos2[idx_plus1] - pos2[idx_minus1]) / 2.0;
	double dx1 = (pos2[idx_plus2] - pos2[idx]) / 2.0;
	double x0 = pos2[idx];
	double x1 = pos2[idx_plus1];
	double rate2 = rate - (double)idx;
	return (((2.0 * (x0 - x1) + (dx0 + dx1)) * rate2 + (-3.0 * (x0 - x1) - (2.0 * dx0 + dx1))) * rate2 + dx0) * rate2 + x0;
}

EXPORT SClass* _makeBmSearch(SClass* me_, const U8* pattern)
{
	THROWDBG(pattern == NULL, EXCPT_ACCESS_VIOLATION);
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

EXPORT S64 _bmSearchFind(SClass* me_, const U8* text, S64 start)
{
	S64 text_len = (S64)((S64*)text)[1];
	const Char* text2 = (const Char*)(text + 0x10);
	SBmSearch* me2 = (SBmSearch*)me_;
	if (start < -1 || text_len <= start)
		return -1;
	if (start == -1)
		start = 0;
	S64 i = start + (S64)me2->PatternLen - 1;
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

EXPORT void* _countUp(S64 min, S64 max)
{
	THROWDBG(min > max, EXCPT_DBG_ARG_OUT_DOMAIN);
	S64 len = max - min + 1;
	THROWDBG(len <= 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	U8* buf = (U8*)AllocMem(0x10 + sizeof(S64) * len);
	S64* ptr = (S64*)(buf + 0x10);
	((S64*)buf)[0] = DefaultRefCntFunc;
	((S64*)buf)[1] = (S64)len;
	S64 i;
	for (i = 0; i < len; i++)
		ptr[i] = min + i;
	return buf;
}

EXPORT S64 _addChkOverflow(Bool* overflowed, S64 n1, S64 n2)
{
	if (AddAsm(&n1, n2))
		*overflowed = True;
	else
		*overflowed = False;
	return n1;
}

EXPORT S64 _subChkOverflow(Bool* overflowed, S64 n1, S64 n2)
{
	if (SubAsm(&n1, n2))
		*overflowed = True;
	else
		*overflowed = False;
	return n1;
}

EXPORT S64 _mulChkOverflow(Bool* overflowed, S64 n1, S64 n2)
{
	if (MulAsm(&n1, n2))
		*overflowed = True;
	else
		*overflowed = False;
	return n1;
}

EXPORT U64 _addr(SClass* me_)
{
	return (U64)me_;
}

EXPORT U64 _toBit64Forcibly(double x)
{
	return *(U64*)&x;
}

EXPORT double _toFloatForcibly(U64 x)
{
	return *(double*)&x;
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
	RndInit(&GlobalRnd, MakeSeed(0x2971c37b));
}
