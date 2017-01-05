#include "lib.h"

#include "rnd.h"

static SRndState GlobalRnd;

EXPORT S64 _rnd(S64 min, S64 max)
{
	return RndGet(&GlobalRnd, min, max);
}

EXPORT double _rndFloat(double min, double max)
{
	return RndGetFloat(&GlobalRnd, min, max);
}

EXPORT U64 _rndBit64(void)
{
	return RndGetBit64(&GlobalRnd);
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

EXPORT double _log(double x, double base)
{
	return log(x) / log(base);
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

EXPORT double _invRot(double x, double y)
{
	double rad = atan2(y, x);
	return rad < 0.0 ? rad + 2.0 * M_PI : rad;
}

EXPORT double _dist(double x, double y)
{
	return hypot(x, y);
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

EXPORT double _floor(double x)
{
	return floor(x);
}

EXPORT double _ceil(double x)
{
	return ceil(x);
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
