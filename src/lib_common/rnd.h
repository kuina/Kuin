// Random Generator with SFMT-607.

#pragma once

#include "..\common.h"

#define RND_M_EXP (607) // Mersenne exponent. The period is 2^RND_M_EXP-1.
#define RND_N (RND_M_EXP / 128 + 1)

typedef struct SRndState
{
	S128 State[RND_N];
	int Idx;
} SRndState;

EXPORT SClass* _makeRnd(SClass* me_, S64 seed);
EXPORT void _rndDtor(SClass* me_);
EXPORT S64 _rndRnd(SClass* me_, S64 min, S64 max);
EXPORT double _rndRndFloat(SClass* me_, double min, double max);
EXPORT U64 _rndRndBit64(SClass* me_);
void InitRndMask(void);
void RndInit(SRndState* rnd_, U32 seed);
S64 RndGet(SRndState* rnd_, S64 min, S64 max);
double RndGetFloat(SRndState* rnd_, double min, double max);
U64 RndGetBit64(SRndState* rnd_);
void* RndGetUuid(SRndState* rnd_);
