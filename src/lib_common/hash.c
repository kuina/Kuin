#include "hash.h"

static const U32 K[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static void HashBlock(U32* h, const U8* data);

EXPORT void* _hash(const U8* data)
{
	THROWDBG(data == NULL, EXCPT_ACCESS_VIOLATION);
	U8* result = (U8*)AllocMem(0x10 + 0x20);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = 0x20;

	{
		S64 size = *(S64*)(data + 0x08);
		const U8* data2 = data + 0x10;
		U32* h = (U32*)(result + 0x10);
		S64 idx = 0;
		S64 padding_size;
		U8 d[64] = { 0 };


		// Initial values.
		h[0] = 0x6a09e667;
		h[1] = 0xbb67ae85;
		h[2] = 0x3c6ef372;
		h[3] = 0xa54ff53a;
		h[4] = 0x510e527f;
		h[5] = 0x9b05688c;
		h[6] = 0x1f83d9ab;
		h[7] = 0x5be0cd19;

		// Process every 64 bytes.
		while (size - idx >= 64)
		{
			HashBlock(h, data2 + idx);
			idx += 64;
		}

		// Stuff padding for fraction of 64 bytes.
		padding_size = size - idx;
		memcpy(d, data2 + idx, (size_t)padding_size);
		d[padding_size] = 0x80;

		// One block is added if the fraction is 56 bytes or more.
		if (padding_size >= 56)
		{
			HashBlock(h, d);
			memset(d, 0, 56);
		}

		// Add the size[bit] of the data in big endian to the end 8 bytes.
		{
			U64 size64 = (U64)size * 8;
			int i;
			for (i = 0; i < 8; i++)
				d[56 + i] = (U8)(size64 >> ((7 - i) * 8));
		}

		// The last block.
		HashBlock(h, d);
	}

	return result;
}

static void HashBlock(U32* h, const U8* data)
{
	int i;
	U32 sit[64];
	U32 n[8];
	for (i = 0; i < 16; i++)
		sit[i] = data[i * 4 + 3] | (data[i * 4 + 2] << 8) | (data[i * 4 + 1] << 16) | (data[i * 4] << 24);
	for (i = 16; i < 64; i++)
	{
		U32 sigma2 = ((sit[i - 2] >> 17) | (sit[i - 2] << (32 - 17))) ^ ((sit[i - 2] >> 19) | (sit[i - 2] << (32 - 19))) ^ (sit[i - 2] >> 10);
		U32 sigma1 = ((sit[i - 15] >> 7) | (sit[i - 15] << (32 - 7))) ^ ((sit[i - 15] >> 18) | (sit[i - 15] << (32 - 18))) ^ (sit[i - 15] >> 3);
		sit[i] = sigma2 + sit[i - 7] + sigma1 + sit[i - 16];
	}
	for (i = 0; i < 8; i++)
		n[i] = h[i];
	for (i = 0; i < 64; i++)
	{
		U32 sum2 = ((n[4] >> 6) | (n[4] << (32 - 6))) ^ ((n[4] >> 11) | (n[4] << (32 - 11))) ^ ((n[4] >> 25) | (n[4] << (32 - 25)));
		U32 ch = (n[4] & n[5]) ^ (~n[4] & n[6]);
		U32 sum1 = ((n[0] >> 2) | (n[0] << (32 - 2))) ^ ((n[0] >> 13) | (n[0] << (32 - 13))) ^ ((n[0] >> 22) | (n[0] << (32 - 22)));
		U32 maj = (n[0] & n[1]) ^ (n[0] & n[2]) ^ (n[1] & n[2]);
		U32 tmp1 = n[7] + sum2 + ch + K[i] + sit[i];
		U32 tmp2 = sum1 + maj;
		n[7] = n[6];
		n[6] = n[5];
		n[5] = n[4];
		n[4] = n[3] + tmp1;
		n[3] = n[2];
		n[2] = n[1];
		n[1] = n[0];
		n[0] = tmp1 + tmp2;
	}
	for (i = 0; i < 8; i++)
	{
		h[i] += n[i];
		h[i] = ((h[i] & 0xff) << 24) | (((h[i] >> 8) & 0xff) << 16) | (((h[i] >> 16) & 0xff) << 8) | ((h[i] >> 24) & 0xff);
	}
}
