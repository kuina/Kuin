// Random Generator with SFMT-607

#pragma once

#include "..\common.h"

#define RND_M_EXP (607) // Mersenne exponent. The period is 2^RND_M_EXP-1.
#define RND_N (RND_M_EXP / 128 + 1)

typedef struct SRndState
{
	S128 State[RND_N];
	int Idx;
} SRndState;

void RndInit(SRndState* rnd_, U32 seed);
S64 RndGet(SRndState* rnd_, S64 min, S64 max);
double RndGetFloat(SRndState* rnd_, double min, double max);
U64 RndGetBit64(SRndState* rnd_);
