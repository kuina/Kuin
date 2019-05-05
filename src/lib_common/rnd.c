#include "rnd.h"

typedef struct SRnd
{
	SClass Class;
	SRndState* RndState;
} SRnd;

static S128 RndMask;
static const int UuidPos[32] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 14, 15, 16, 17, 19, 20, 21, 22, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
};

static void RndDo(S128* r, S128 a, S128 b, S128 c, const S128* d);

EXPORT SClass* _makeRnd(SClass* me_, S64 seed)
{
	THROWDBG(seed < -1 || 0xffffffff < seed, EXCPT_DBG_ARG_OUT_DOMAIN);
	SRnd* me2 = (SRnd*)me_;
	me2->RndState = (SRndState*)AllocMem(sizeof(SRndState));
	if (seed == -1)
		RndInit(me2->RndState, (U32)(time(NULL)) ^ (U32)timeGetTime() ^ 0x731ae8f9);
	else
		RndInit(me2->RndState, (U32)seed);
	return me_;
}

EXPORT void _rndDtor(SClass* me_)
{
	FreeMem(((SRnd*)me_)->RndState);
}

EXPORT S64 _rndRnd(SClass* me_, S64 min, S64 max)
{
	THROWDBG(max - min < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	SRnd* me2 = (SRnd*)me_;
	return RndGet(me2->RndState, min, max);
}

EXPORT double _rndRndFloat(SClass* me_, double min, double max)
{
	THROWDBG(min >= max, EXCPT_DBG_ARG_OUT_DOMAIN);
	SRnd* me2 = (SRnd*)me_;
	return RndGetFloat(me2->RndState, min, max);
}

EXPORT U64 _rndRndBit64(SClass* me_)
{
	SRnd* me2 = (SRnd*)me_;
	return RndGetBit64(me2->RndState);
}

void InitRndMask(void)
{
	RndMask = _mm_set_epi32(0x7ff7fb2f, 0xff777b7d, 0xef7f3f7d, 0xfdff37ff);
}

void RndInit(SRndState* rnd_, U32 seed)
{
	// Initialize the buffers.
	{
		int i;
		U32* p = (U32*)(&rnd_->State[0]);
		p[0] = seed;
		for (i = 1; i < RND_N * (128 / 32); i++)
			p[i] = 1812433253 * (p[i - 1] ^ (p[i - 1] >> 30)) + i;
		rnd_->Idx = RND_N * (128 / 32);
	}

	// Check the parity.
	{
		const U32 parity[4] =
		{
			0x00000001,
			0x00000000,
			0x00000000,
			0x5986f054,
		};
		int inner = 0;
		U32* p = (U32*)(&rnd_->State[0]);
		{
			int i;
			for (i = 0; i < 4; i++)
				inner ^= p[i] & parity[i];
			for (i = 16; i > 0; i /= 2)
				inner ^= inner >> i;
		}

		if (inner % 2 != 1)
		{
			int i;
			for (i = 0; i < 4; i++)
			{
				U32 work = 1;
				int j;
				for (j = 0; j < 32; j++)
				{
					if ((work & parity[i]) != 0)
					{
						p[i] ^= work;
						goto EndOfModification;
					}
					work *= 2;
				}
			}
			EndOfModification:;
		}
	}
}

S64 RndGet(SRndState* rnd_, S64 min, S64 max)
{
	U64 n = (U64)(max - min + 1);
	U64 m = 0 - ((0 - n) % n);
	U64 r;
	if (m == 0)
		r = RndGetBit64(rnd_);
	else
	{
		do
		{
			r = RndGetBit64(rnd_);
		} while (m <= r);
	}
	return (S64)(r % n) + min;
}

double RndGetFloat(SRndState* rnd_, double min, double max)
{
	return (double)(RndGetBit64(rnd_)) / 18446744073709551616.0 * (max - min) + min;
}

U64 RndGetBit64(SRndState* rnd_)
{
	// Reset if buffer overrun.
	if (rnd_->Idx >= RND_N * (128 / 32))
	{
		ASSERT(rnd_->Idx % 2 == 0);

		{
			S128 r1 = rnd_->State[RND_N - 2];
			S128 r2 = rnd_->State[RND_N - 1];
			int i = 0;
			while (i < RND_N - 2)
			{
				RndDo(&rnd_->State[i], rnd_->State[i], rnd_->State[i + 2], r1, &r2);
				r1 = r2;
				r2 = rnd_->State[i];
				i++;
			}
			while (i < RND_N)
			{
				RndDo(&rnd_->State[i], rnd_->State[i], rnd_->State[i + 2 - RND_N], r1, &r2);
				r1 = r2;
				r2 = rnd_->State[i];
				i++;
			}
			rnd_->Idx = 0;
		}
	}

	// Generate a 64 bit random number using 2 buffers.
	{
		U64* p = (U64*)(&rnd_->State[0]);
		U64 result = p[rnd_->Idx / 2];
		rnd_->Idx += 2;
		return result;
	}
}

void* RndGetUuid(SRndState* rnd_)
{
	const size_t len = 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12; // RRRRRRRR-RRRR-4RRR-rRRR-RRRRRRRRRRRR
	U64 r1 = RndGetBit64(rnd_);
	U64 r2 = RndGetBit64(rnd_);
	int i;
	U8* uuid = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	Char* uuid2 = (Char*)(uuid + 0x10);
	((S64*)uuid)[0] = DefaultRefCntFunc;
	((S64*)uuid)[1] = (S64)len;
	for (i = 0; i < 32; i++)
	{
		int n = ((i / 16 == 0 ? r1 : r2) >> (i % 16 * 4)) & 0x0f;
		if (i == 12)
			n = 4;
		else if (i == 16)
			n = (n & 0x03) | 0x08;
		Char c = (Char)(n <= 9 ? (L'0' + n) : (L'a' + n - 10));
		uuid2[UuidPos[i]] = c;
	}
	uuid2[8] = L'-';
	uuid2[13] = L'-';
	uuid2[18] = L'-';
	uuid2[23] = L'-';
	uuid2[len] = L'\0';
	return uuid;
}

static void RndDo(S128* r, S128 a, S128 b, S128 c, const S128* d)
{
	S128 v, x, y, z;
	y = _mm_srli_epi32(b, 13);
	z = _mm_srli_si128(c, 3);
	v = _mm_slli_epi32(*d, 15);
	z = _mm_xor_si128(z, a);
	z = _mm_xor_si128(z, v);
	x = _mm_slli_si128(a, 3);
	y = _mm_and_si128(y, RndMask);
	z = _mm_xor_si128(z, x);
	z = _mm_xor_si128(z, y);
	*r = z;
}
