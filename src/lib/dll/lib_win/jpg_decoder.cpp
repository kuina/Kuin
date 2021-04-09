#include "jpg_decoder.h"

#define CLIP(x) ((x) < 0 ? 0 : ((x) > 0xff ? 0xff : static_cast<U8>(x)))

struct SComponent
{
	int Cid;
	int Ssx;
	int Ssy;
	int Width;
	int Height;
	int Stride;
	int Qtsel;
	int Actabsel;
	int Dctabsel;
	int Dcpred;
	U8* Pixels;
};

struct SVlcCode
{
	U8 Bits;
	U8 Code;
};

struct SJpgData
{
	const U8* Ptr;
	S64 Size;
	int Len;
	Bool EndFlag;
	int Width;
	int Height;
	int NComp;
	SComponent Component[3];
	int QtUsed;
	int MBSizeX;
	int MBSizeY;
	int MBWidth;
	int MBHeight;
	U8* Rgb;
	SVlcCode VlcTab[4][65536];
	int RSTInterval;
	int Block[64];
	U8 QTab[4][64];
	int Buf;
	int BufBits;
	int QTAvail;
};

static const U8 Zz[64] =
{
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

static void Skip(SJpgData* jpg_data, int count);
static void ReadLen(SJpgData* jpg_data);
static int GetVlc(SJpgData* jpg_data, SVlcCode* vlc, U8* code);
static int ShowBits(SJpgData* jpg_data, int bits);
static void RowIdct(int* blk);
static void ColIDCT(int* blk, U8* out, int stride);
static void DecodeSof(SJpgData* jpg_data);
static void DecodeDht(SJpgData* jpg_data);
static void DecodeSos(SJpgData* jpg_data);
static void DecodeDqt(SJpgData* jpg_data);
static void DecodeDri(SJpgData* jpg_data);
static U16 SwapEndianU16(U16 n);

void* DecodeJpg(size_t size, const void* data, int* width, int* height)
{
	SJpgData jpg_data;
	memset(&jpg_data, 0, sizeof(SJpgData));
	jpg_data.Ptr = static_cast<const U8*>(data);
	jpg_data.Size = static_cast<S64>(size);
	// Start of image.
	if (*reinterpret_cast<const U16*>(jpg_data.Ptr) != 0xd8ff)
		THROW(0xe9170008);
	Skip(&jpg_data, 2);
	while (!jpg_data.EndFlag)
	{
		if (*jpg_data.Ptr != 0xff)
			THROW(0xe9170008);
		Skip(&jpg_data, 2);
		switch (jpg_data.Ptr[-1])
		{
			// Start of frame.
			case 0xc0: DecodeSof(&jpg_data); break;
				// Define huffman table.
			case 0xc4: DecodeDht(&jpg_data); break;
				// Start of scan.
			case 0xda: DecodeSos(&jpg_data); break;
				// Define quantization table.
			case 0xdb: DecodeDqt(&jpg_data); break;
				// Define restart interval.
			case 0xdd: DecodeDri(&jpg_data); break;
				// Comment
			case 0xfe:
				ReadLen(&jpg_data);
				Skip(&jpg_data, jpg_data.Len);
				break;
			default:
				if ((jpg_data.Ptr[-1] & 0xf0) != 0xe0)
					THROW(0xe9170008);
				ReadLen(&jpg_data);
				Skip(&jpg_data, jpg_data.Len);
				break;
		}
	}
	{
		SComponent* c = jpg_data.Component;
		for (int i = 0; i < jpg_data.NComp; i++)
		{
			if (c->Width < jpg_data.Width || c->Height < jpg_data.Height)
			{
				int xshift = 0, yshift = 0;
				while (c->Width < jpg_data.Width)
				{
					c->Width <<= 1;
					xshift++;
				}
				while (c->Height < jpg_data.Height)
				{
					c->Height <<= 1;
					yshift++;
				}
				U8* out = static_cast<U8*>(AllocMem(c->Width * c->Height));
				U8* lin = c->Pixels;
				U8* lout = out;
				for (int y = 0; y < c->Height; y++)
				{
					lin = &c->Pixels[(y >> yshift) * c->Stride];
					for (int x = 0; x < c->Width; x++)
						lout[x] = lin[x >> xshift];
					lout += c->Width;
				}
				c->Stride = c->Width;
				FreeMem(c->Pixels);
				c->Pixels = out;
			}
			if (c->Width < jpg_data.Width || c->Height < jpg_data.Height)
				THROW(0xe9170008);
			c++;
		}
		if (jpg_data.NComp != 3)
			THROW(0xe9170008);
		{
			U8* prgb = jpg_data.Rgb;
			const U8* py = jpg_data.Component[0].Pixels;
			const U8* pcb = jpg_data.Component[1].Pixels;
			const U8* pcr = jpg_data.Component[2].Pixels;
			for (int yy = jpg_data.Height; yy > 0; yy--)
			{
				for (int x = 0; x < jpg_data.Width; x++)
				{
					int y = py[x] << 8;
					int cb = pcb[x] - 128;
					int cr = pcr[x] - 128;
					*prgb = CLIP((y + 359 * cr + 128) >> 8);
					prgb++;
					*prgb = CLIP((y - 88 * cb - 183 * cr + 128) >> 8);
					prgb++;
					*prgb = CLIP((y + 454 * cb + 128) >> 8);
					prgb++;
					*prgb = 255;
					prgb++;
				}
				py += jpg_data.Component[0].Stride;
				pcb += jpg_data.Component[1].Stride;
				pcr += jpg_data.Component[2].Stride;
			}
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (jpg_data.Component[i].Pixels != 0)
			FreeMem(jpg_data.Component[i].Pixels);
	}
	*width = jpg_data.Width;
	*height = jpg_data.Height;
	return jpg_data.Rgb;
}

static void Skip(SJpgData* jpg_data, int count)
{
	jpg_data->Ptr += count;
	jpg_data->Size -= count;
	jpg_data->Len -= count;
	if (jpg_data->Size < 0)
		THROW(0xe9170008);
}

static void ReadLen(SJpgData* jpg_data)
{
	jpg_data->Len = SwapEndianU16(*reinterpret_cast<const U16*>(jpg_data->Ptr));
	if (jpg_data->Len > jpg_data->Size)
		THROW(0xe9170008);
	Skip(jpg_data, 2);
}

static int GetVlc(SJpgData* jpg_data, SVlcCode* vlc, U8* code)
{
	int value = ShowBits(jpg_data, 16);
	int bits = vlc[value].Bits;
	if (bits == 0)
		THROW(0xe9170008);
	if (jpg_data->BufBits < bits)
		ShowBits(jpg_data, bits);
	jpg_data->BufBits -= bits;
	value = vlc[value].Code;
	if (code != 0)
		*code = static_cast<U8>(value);
	bits = value & 15;
	if (bits == 0)
		return 0;
	value = ShowBits(jpg_data, bits);
	if (jpg_data->BufBits < bits)
		ShowBits(jpg_data, bits);
	jpg_data->BufBits -= bits;
	if (value < (1 << (bits - 1)))
		value += ((0xffffffff) << bits) + 1;
	return value;
}

static int ShowBits(SJpgData* jpg_data, int bits)
{
	if (bits == 0)
		return 0;
	while (jpg_data->BufBits < bits)
	{
		if (jpg_data->Size <= 0)
		{
			jpg_data->Buf = (jpg_data->Buf << 8) | 0xff;
			jpg_data->BufBits += 8;
			continue;
		}
		U8 new_byte = *jpg_data->Ptr;
		Skip(jpg_data, 1);
		jpg_data->BufBits += 8;
		jpg_data->Buf = (jpg_data->Buf << 8) | new_byte;
		if (new_byte == 0xff)
		{
			if (jpg_data->Size == 0)
				THROW(0xe9170008);
			U8 marker = *jpg_data->Ptr;
			Skip(jpg_data, 1);
			switch (marker)
			{
				case 0x00:
				case 0xff:
					break;
				case 0xd9:
					jpg_data->Size = 0;
					break;
				default:
					if ((marker & 0xf8) != 0xd0)
						THROW(0xe9170008);
					jpg_data->Buf = (jpg_data->Buf << 8) | marker;
					jpg_data->BufBits += 8;
					break;
			}
		}
	}
	return (jpg_data->Buf >> (jpg_data->BufBits - bits)) & ((1 << bits) - 1);
}

static void RowIdct(int* blk)
{
	int x0 = (blk[0] << 11) + 128;
	int x1 = blk[4] << 11;
	int x2 = blk[6];
	int x3 = blk[2];
	int x4 = blk[1];
	int x5 = blk[7];
	int x6 = blk[5];
	int x7 = blk[3];
	int x8;
	if (x1 == 0 && x2 == 0 && x3 == 0 && x4 == 0 && x5 == 0 && x6 == 0 && x7 == 0)
	{
		blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
		return;
	}
	x8 = 565 * (x4 + x5);
	x4 = x8 + (2841 - 565) * x4;
	x5 = x8 - (2841 + 565) * x5;
	x8 = 2408 * (x6 + x7);
	x6 = x8 - (2408 - 1609) * x6;
	x7 = x8 - (2408 + 1609) * x7;
	x8 = x0 + x1;
	x0 -= x1;
	x1 = 1108 * (x3 + x2);
	x2 = x1 - (2676 + 1108) * x2;
	x3 = x1 + (2676 - 1108) * x3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;
	blk[0] = (x7 + x1) >> 8;
	blk[1] = (x3 + x2) >> 8;
	blk[2] = (x0 + x4) >> 8;
	blk[3] = (x8 + x6) >> 8;
	blk[4] = (x8 - x6) >> 8;
	blk[5] = (x0 - x4) >> 8;
	blk[6] = (x3 - x2) >> 8;
	blk[7] = (x7 - x1) >> 8;
}

static void ColIDCT(int* blk, U8* out, int stride)
{
	int x0 = (blk[0] << 8) + 8192;
	int x1 = blk[8 * 4] << 8;
	int x2 = blk[8 * 6];
	int x3 = blk[8 * 2];
	int x4 = blk[8 * 1];
	int x5 = blk[8 * 7];
	int x6 = blk[8 * 5];
	int x7 = blk[8 * 3];
	int x8;
	if (x1 == 0 && x2 == 0 && x3 == 0 && x4 == 0 && x5 == 0 && x6 == 0 && x7 == 0)
	{
		x1 = CLIP(((blk[0] + 32) >> 6) + 128);
		for (x0 = 8; x0 > 0; x0--)
		{
			*out = static_cast<U8>(x1);
			out += stride;
		}
		return;
	}
	x8 = 565 * (x4 + x5) + 4;
	x4 = (x8 + (2841 - 565) * x4) >> 3;
	x5 = (x8 - (2841 + 565) * x5) >> 3;
	x8 = 2408 * (x6 + x7) + 4;
	x6 = (x8 - (2408 - 1609) * x6) >> 3;
	x7 = (x8 - (2408 + 1609) * x7) >> 3;
	x8 = x0 + x1;
	x0 -= x1;
	x1 = 1108 * (x3 + x2) + 4;
	x2 = (x1 - (2676 + 1108) * x2) >> 3;
	x3 = (x1 + (2676 - 1108) * x3) >> 3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;
	*out = CLIP(((x7 + x1) >> 14) + 128);
	out += stride;
	*out = CLIP(((x3 + x2) >> 14) + 128);
	out += stride;
	*out = CLIP(((x0 + x4) >> 14) + 128);
	out += stride;
	*out = CLIP(((x8 + x6) >> 14) + 128);
	out += stride;
	*out = CLIP(((x8 - x6) >> 14) + 128);
	out += stride;
	*out = CLIP(((x0 - x4) >> 14) + 128);
	out += stride;
	*out = CLIP(((x3 - x2) >> 14) + 128);
	out += stride;
	*out = CLIP(((x7 - x1) >> 14) + 128);
}

static void DecodeSof(SJpgData* jpg_data)
{
	ReadLen(jpg_data);
	if (jpg_data->Len < 9)
		THROW(0xe9170008);
	if (*jpg_data->Ptr != 8)
		THROW(0xe9170008);
	jpg_data->Height = static_cast<int>(SwapEndianU16(*reinterpret_cast<const U16*>(jpg_data->Ptr + 1)));
	jpg_data->Width = static_cast<int>(SwapEndianU16(*reinterpret_cast<const U16*>(jpg_data->Ptr + 3)));
	jpg_data->NComp = static_cast<int>(jpg_data->Ptr[5]);
	Skip(jpg_data, 6);
	if (jpg_data->NComp != 3)
		THROW(0xe9170008);
	if (jpg_data->Len < jpg_data->NComp * 3)
		THROW(0xe9170008);
	SComponent* c = jpg_data->Component;
	int ssxmax = 0, ssymax = 0;
	for (int i = 0; i < jpg_data->NComp; i++)
	{
		c->Cid = jpg_data->Ptr[0];
		c->Ssx = jpg_data->Ptr[1] >> 4;
		if (c->Ssx == 0 || (c->Ssx & (c->Ssx - 1)) != 0)
			THROW(0xe9170008);
		c->Ssy = jpg_data->Ptr[1] & 15;
		if (c->Ssy == 0 || (c->Ssy & (c->Ssy - 1)) != 0)
			THROW(0xe9170008);
		c->Qtsel = jpg_data->Ptr[2];
		if ((c->Qtsel & 0xfc) != 0)
			THROW(0xe9170008);
		Skip(jpg_data, 3);
		jpg_data->QtUsed |= 1 << c->Qtsel;
		if (c->Ssx > ssxmax)
			ssxmax = c->Ssx;
		if (c->Ssy > ssymax)
			ssymax = c->Ssy;
		c++;
	}
	jpg_data->MBSizeX = ssxmax << 3;
	jpg_data->MBSizeY = ssymax << 3;
	jpg_data->MBWidth = (jpg_data->Width + jpg_data->MBSizeX - 1) / jpg_data->MBSizeX;
	jpg_data->MBHeight = (jpg_data->Height + jpg_data->MBSizeY - 1) / jpg_data->MBSizeY;
	c = jpg_data->Component;
	for (int i = 0; i < jpg_data->NComp; i++)
	{
		c->Width = (jpg_data->Width * c->Ssx + ssxmax - 1) / ssxmax;
		c->Height = (jpg_data->Height * c->Ssy + ssymax - 1) / ssymax;
		c->Stride = jpg_data->MBWidth * jpg_data->MBSizeX * c->Ssx / ssxmax;
		if (c->Width < 3 && c->Ssx != ssxmax || c->Height < 3 && c->Ssy != ssymax)
			THROW(0xe9170008);
		c->Pixels = static_cast<U8*>(AllocMem(c->Stride * (jpg_data->MBHeight * jpg_data->MBSizeY * c->Ssy / ssymax)));
		c++;
	}
	jpg_data->Rgb = static_cast<U8*>(AllocMem(jpg_data->Width * jpg_data->Height * 4));
	Skip(jpg_data, jpg_data->Len);
}

static void DecodeDht(SJpgData* jpg_data)
{
	ReadLen(jpg_data);
	U8 counts[16];
	while (jpg_data->Len >= 17)
	{
		int i = jpg_data->Ptr[0];
		if ((i & 0xec) != 0 || (i & 0x02) != 0)
			THROW(0xe9170008);
		i = (i | (i >> 3)) & 3;
		for (int codelen = 0; codelen < 16; codelen++)
			counts[codelen] = jpg_data->Ptr[codelen + 1];
		Skip(jpg_data, 17);
		SVlcCode* vlc = &jpg_data->VlcTab[i][0];
		int remain = 65536;
		int spread = 65536;
		for (int codelen = 1; codelen <= 16; codelen++)
		{
			spread >>= 1;
			int cur = counts[codelen - 1];
			if (cur == 0)
				continue;
			if (jpg_data->Len < cur)
				THROW(0xe9170008);
			remain -= cur << (16 - codelen);
			if (remain < 0)
				THROW(0xe9170008);
			for (i = 0; i < cur; i++)
			{
				U8 code = jpg_data->Ptr[i];
				for (int j = spread; j > 0; j--)
				{
					vlc->Bits = (U8)codelen;
					vlc->Code = code;
					vlc++;
				}
			}
			Skip(jpg_data, cur);
		}
		while (remain--)
		{
			vlc->Bits = 0;
			vlc++;
		}
	}
	if (jpg_data->Len != 0)
		THROW(0xe9170008);
}

static void DecodeSos(SJpgData* jpg_data)
{
	ReadLen(jpg_data);
	if (jpg_data->Len < 4 + 2 * jpg_data->NComp)
		THROW(0xe9170008);
	if (jpg_data->Ptr[0] != jpg_data->NComp)
		THROW(0xe9170008);
	Skip(jpg_data, 1);
	SComponent* c = jpg_data->Component;
	for (int i = 0; i < jpg_data->NComp; i++)
	{
		if (jpg_data->Ptr[0] != c->Cid || (jpg_data->Ptr[1] & 0xee) != 0)
			THROW(0xe9170008);
		c->Dctabsel = jpg_data->Ptr[1] >> 4;
		c->Actabsel = (jpg_data->Ptr[1] & 1) | 2;
		Skip(jpg_data, 2);
		c++;
	}
	if (jpg_data->Ptr[0] != 0 || jpg_data->Ptr[1] != 63 || jpg_data->Ptr[2] != 0)
		THROW(0xe9170008);
	Skip(jpg_data, jpg_data->Len);
	int mbx = 0, mby = 0, rstcount = jpg_data->RSTInterval, nextrst = 0;
	for (; ; )
	{
		c = jpg_data->Component;
		for (int i = 0; i < jpg_data->NComp; i++)
		{
			for (int sby = 0; sby < c->Ssy; sby++)
			{
				for (int sbx = 0; sbx < c->Ssx; sbx++)
				{
					{
						U8* out = &c->Pixels[((mby * c->Ssy + sby) * c->Stride + mbx * c->Ssx + sbx) << 3];
						memset(jpg_data->Block, 0, sizeof(jpg_data->Block));
						c->Dcpred += GetVlc(jpg_data, &jpg_data->VlcTab[c->Dctabsel][0], nullptr);
						jpg_data->Block[0] = c->Dcpred * jpg_data->QTab[c->Qtsel][0];
						int coef = 0;
						do
						{
							U8 code;
							int value = GetVlc(jpg_data, &jpg_data->VlcTab[c->Actabsel][0], &code);
							if (code == 0)
								break;
							if ((code & 0x0f) == 0 && code != 0xf0)
								THROW(0xe9170008);
							coef += (code >> 4) + 1;
							if (coef > 63)
								THROW(0xe9170008);
							jpg_data->Block[(int)Zz[coef]] = value * jpg_data->QTab[c->Qtsel][coef];
						} while (coef < 63);
						for (coef = 0; coef < 64; coef += 8)
							RowIdct(&jpg_data->Block[coef]);
						for (coef = 0; coef < 8; coef++)
							ColIDCT(&jpg_data->Block[coef], &out[coef], c->Stride);
					}
				}
			}
			c++;
		}
		mbx++;
		if (mbx >= jpg_data->MBWidth)
		{
			mbx = 0;
			mby++;
			if (mby >= jpg_data->MBHeight)
				break;
		}
		rstcount--;
		if (jpg_data->RSTInterval != 0 && rstcount == 0)
		{
			jpg_data->BufBits &= 0xf8;
			int i = ShowBits(jpg_data, 16);
			if (jpg_data->BufBits < 16)
				ShowBits(jpg_data, 16);
			jpg_data->BufBits -= 16;
			if ((i & 0xfff8) != 0xffd0 || (i & 7) != nextrst)
				THROW(0xe9170008);
			nextrst = (nextrst + 1) & 7;
			rstcount = jpg_data->RSTInterval;
			for (i = 0; i < 3; i++)
				jpg_data->Component[i].Dcpred = 0;
		}
	}
	jpg_data->EndFlag = True;
}

static void DecodeDqt(SJpgData* jpg_data)
{
	ReadLen(jpg_data);
	while (jpg_data->Len >= 65)
	{
		int i = jpg_data->Ptr[0];
		if ((i & 0xfc) != 0)
			THROW(0xe9170008);
		jpg_data->QTAvail |= 1 << i;
		U8* t = &jpg_data->QTab[i][0];
		for (i = 0; i < 64; i++)
			t[i] = jpg_data->Ptr[i + 1];
		Skip(jpg_data, 65);
	}
	if (jpg_data->Len != 0)
		THROW(0xe9170008);
}

static void DecodeDri(SJpgData* jpg_data)
{
	ReadLen(jpg_data);
	if (jpg_data->Len < 2)
		THROW(0xe9170008);
	jpg_data->RSTInterval = static_cast<int>(SwapEndianU16(*reinterpret_cast<const U16*>(jpg_data->Ptr)));
	Skip(jpg_data, jpg_data->Len);
}

static U16 SwapEndianU16(U16 n)
{
	return ((n & 0x00ff) << 8) | ((n & 0xFF00) >> 8);
}
