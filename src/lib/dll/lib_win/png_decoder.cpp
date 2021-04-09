#include "png_decoder.h"

#define IdatMax (256)

struct SPngData
{
	int Width;
	int Height;
	int BitDepth;
	int ColorType;
	int CompressionMethod;
	int FilterMethod;
	int InterlaceMethod;
	size_t DataSize[IdatMax];
	const U8* Data[IdatMax];
	int DataNum;

	int X, Y;
	int Filter;
	int InlineCnt;
	int InpixelCnt;
	int FirstByte;
	int Idx;
	int InterlacePass;
	U8* TwoLine;
	U8* CurLine;
	U8* PrvLine;
	U8* WindowBuf;
};

struct SHuffmanTree
{
	SHuffmanTree* Zero;
	SHuffmanTree* One;
	U32 Data, Weight, Depth;
};

static void Decode(SPngData* png_data, U8* argb);
static void Output(SPngData* png_data, U8* argb, U8 data);
static U32 GetNextBit(SPngData* png_data, int* cur_data, U32* byte_ptr, U32* bit_ptr);
static U32 GetNextMultiBit(SPngData* png_data, int* cur_data, U32* byte_ptr, U32* bit_ptr, int n);
static SHuffmanTree* MakeHuffmanTree(int n, U32* h_length, U32* h_code);
static SHuffmanTree* NewHuffmanTree();
static void DeleteHuffmanTree(SHuffmanTree* node);
static void MakeDynamicHuffmanCode(U32* h_length, U32* h_code, int n_lng, U32* lng);
static void Filter8(U8* cur_line, U8* prv_line, int x, int y, int unit_lng, int filter);
static void Step(SPngData* png_data, int* cur_data, U32* byte_ptr, int n);
static U8 SwapEndianU8(U8 n);
static U32 SwapEndianU32(U32 n);

void* DecodePng(size_t size, const void* data, int* width, int* height)
{
	UNUSED(size);

	const U8* ptr = static_cast<const U8*>(data);
	if (*reinterpret_cast<const U64*>(ptr) != 0x0a1a0a0d474e5089)
		THROW(0xe9170008); // '.PNG'
	ptr += sizeof(U64);

	*width = 0;
	*height = 0;
	SPngData png_data;
	memset(&png_data, 0, sizeof(png_data));

	// Read chunk.
	for (; ; )
	{
		U32 size2 = SwapEndianU32(*reinterpret_cast<const U32*>(ptr));
		ptr += sizeof(U32);
		U32 chunk = *reinterpret_cast<const U32*>(ptr);
		ptr += sizeof(U32);
		Bool iend = False;
		switch (chunk)
		{
			case 0x52444849: // 'IHDR'
				if (size2 != sizeof(U32) * 2 + sizeof(U8) * 5)
					THROW(0xe9170008);
				png_data.Width = static_cast<int>(SwapEndianU32(*reinterpret_cast<const U32*>(ptr)));
				*width = png_data.Width;
				ptr += sizeof(U32);
				png_data.Height = static_cast<int>(SwapEndianU32(*reinterpret_cast<const U32*>(ptr)));
				*height = png_data.Height;
				ptr += sizeof(U32);
				png_data.BitDepth = static_cast<int>(SwapEndianU8(*reinterpret_cast<const U8*>(ptr)));
				ptr += sizeof(U8);
				png_data.ColorType = static_cast<int>(SwapEndianU8(*reinterpret_cast<const U8*>(ptr)));
				ptr += sizeof(U8);
				png_data.CompressionMethod = static_cast<int>(SwapEndianU8(*reinterpret_cast<const U8*>(ptr)));
				ptr += sizeof(U8);
				png_data.FilterMethod = static_cast<int>(SwapEndianU8(*reinterpret_cast<const U8*>(ptr)));
				ptr += sizeof(U8);
				png_data.InterlaceMethod = static_cast<int>(SwapEndianU8(*reinterpret_cast<const U8*>(ptr)));
				ptr += sizeof(U8);
				break;
			case 0x54414449: // 'IDAT'
				if (png_data.DataNum == IdatMax)
					THROW(0xe9170008);
				png_data.Data[png_data.DataNum] = ptr;
				png_data.DataSize[png_data.DataNum] = static_cast<size_t>(size2);
				png_data.DataNum++;
				ptr += size2;
				break;
			case 0x444e4549: // 'IEND'
				iend = True;
				break;
			default:
				ptr += size2;
				break;
		}
		ptr += sizeof(U32); // Ignore a CRC.
		if (iend)
			break;
	}

	if (png_data.DataNum == 0)
		THROW(0xe9170008);
	if (png_data.Width == 0)
		THROW(0xe9170008);
	if (png_data.Height == 0)
		THROW(0xe9170008);

	U8* argb = static_cast<U8*>(AllocMem(png_data.Width * png_data.Height * 4));
	Decode(&png_data, argb);
	return argb;
}

static void Decode(SPngData* png_data, U8* argb)
{
	{
		if (png_data->BitDepth != 8)
			THROW(0xe9170008);
		int two_line = 0;
		switch (png_data->ColorType)
		{
			case 0: // Grayscale.
				two_line = png_data->Width;
				break;
			case 2: // Color.
				two_line = png_data->Width * 3;
				break;
			case 4: // Grayscale with alpha.
				two_line = png_data->Width * 2;
				break;
			case 6: // Color with alpha.
				two_line = png_data->Width * 4;
				break;
			default: // Unsupported format.
				THROW(0xe9170008);
				break;
		}

		png_data->X = -1;
		png_data->Y = 0;
		png_data->Filter = 0;
		png_data->InlineCnt = 0;
		png_data->InpixelCnt = 0;
		png_data->FirstByte = 0;
		png_data->Idx = 0;
		png_data->InterlacePass = 1;
		png_data->TwoLine = static_cast<U8*>(AllocMem(two_line * 2));
		png_data->CurLine = png_data->TwoLine;
		png_data->PrvLine = png_data->TwoLine + two_line;
	}

	{
		int cur_data = 0;
		U32 byte_ptr = 0;
		U32 bit_ptr = 1;
		U8 cmf = png_data->Data[cur_data][byte_ptr];
		Step(png_data, &cur_data, &byte_ptr, 1);
		U8 flg = png_data->Data[cur_data][byte_ptr];
		UNUSED(flg);
		Step(png_data, &cur_data, &byte_ptr, 1);
		if ((cmf & 0x0f) != 8)
			THROW(0xe9170008);
		if (((flg & 0x20) >> 5) != 0)
			THROW(0xe9170008);
		U32 window_size = 1 << (((cmf & 0xf0) >> 4) + 8);
		png_data->WindowBuf = static_cast<U8*>(AllocMem(window_size));
		U32 window_used = 0;

		SHuffmanTree* code_tree = nullptr;
		SHuffmanTree* code_tree_ptr = nullptr;
		SHuffmanTree* dist_tree = nullptr;
		SHuffmanTree* dist_tree_ptr = nullptr;
		for (; ; )
		{
			U32 bfinal = GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr);
			U32 btype = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 2);
			if (btype == 0)
			{
				// No Compression.
				if (bit_ptr != 1)
				{
					bit_ptr = 1;
					Step(png_data, &cur_data, &byte_ptr, 1);
				}
				int len = static_cast<int>(png_data->Data[cur_data][byte_ptr]);
				Step(png_data, &cur_data, &byte_ptr, 1);
				len += static_cast<int>(png_data->Data[cur_data][byte_ptr] << 8);
				Step(png_data, &cur_data, &byte_ptr, 3);
				for (int i = 0; i < len; i++)
				{
					Output(png_data, argb, png_data->Data[cur_data][byte_ptr]);
					Step(png_data, &cur_data, &byte_ptr, 1);
				}
			}
			else if (btype == 1 || btype == 2)
			{
				code_tree = nullptr;
				if (btype == 1)
				{
					U32 h_length[288], h_code[288];
					{
						for (int i = 0; i <= 143; i++)
						{
							h_length[i] = 8;
							h_code[i] = 0x30 + static_cast<U32>(i);
						}
						for (int i = 144; i <= 255; i++)
						{
							h_length[i] = 9;
							h_code[i] = 0x190 + (static_cast<U32>(i) - 144);
						}
						for (int i = 256; i <= 279; i++)
						{
							h_length[i] = 7;
							h_code[i] = static_cast<U32>(i) - 256;
						}
						for (int i = 280; i <= 287; i++)
						{
							h_length[i] = 8;
							h_code[i] = 0xc0 + (static_cast<U32>(i) - 280);
						}
					}
					code_tree = MakeHuffmanTree(288, h_length, h_code);
					dist_tree = nullptr;
				}
				else
				{
					U32 h_lit, h_dist, h_clen;
					U32* h_length_literal;
					U32* h_code_literal;
					U32* h_length_dist;
					U32* h_code_dist;
					U32 h_length_buf[322], h_code_buf[322];
					h_lit = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 5);
					h_dist = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 5);
					h_clen = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 4);
					U32 code_length_order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
					U32 code_length_code[19] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
					for (int i = 0; i < static_cast<int>(h_clen) + 4; i++)
						code_length_code[code_length_order[i]] = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 3);
					U32 hlength_code[19], hcode_code[19];
					MakeDynamicHuffmanCode(hlength_code, hcode_code, 19, code_length_code);
					h_length_literal = h_length_buf;
					h_code_literal = h_code_buf;
					h_length_dist = h_length_buf + h_lit + 257;
					h_code_dist = h_code_buf + h_lit + 257;
					SHuffmanTree* length_tree = MakeHuffmanTree(19, hlength_code, hcode_code);
					SHuffmanTree* length_tree_ptr = length_tree;
					U32 nextr = 0;
					while (nextr < h_lit + 257 + h_dist + 1)
					{
						if (GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr))
							length_tree_ptr = length_tree_ptr->One;
						else
							length_tree_ptr = length_tree_ptr->Zero;
						if (length_tree_ptr->Zero == nullptr && length_tree_ptr->One == nullptr)
						{
							U32 v = length_tree_ptr->Data, copylength;
							if (v <= 15)
							{
								h_length_buf[nextr] = v;
								nextr++;
							}
							else if (v == 16)
							{
								copylength = 3 + GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 2);
								while (copylength > 0)
								{
									h_length_buf[nextr] = h_length_buf[nextr - 1];
									nextr++;
									copylength--;
								}
							}
							else if (v == 17)
							{
								copylength = 3 + GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 3);
								while (copylength > 0)
								{
									h_length_buf[nextr] = 0;
									nextr++;
									copylength--;
								}
							}
							else if (v == 18)
							{
								copylength = 11 + GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, 7);
								while (copylength > 0)
								{
									h_length_buf[nextr] = 0;
									nextr++;
									copylength--;
								}
							}
							length_tree_ptr = length_tree;
						}
					}
					MakeDynamicHuffmanCode(h_length_literal, h_code_literal, static_cast<int>(h_lit + 257), h_length_literal);
					MakeDynamicHuffmanCode(h_length_dist, h_code_dist, static_cast<int>(h_dist + 1), h_length_dist);
					DeleteHuffmanTree(length_tree);
					code_tree = MakeHuffmanTree(static_cast<int>(h_lit + 257), h_length_literal, h_code_literal);
					dist_tree = MakeHuffmanTree(static_cast<int>(h_dist + 1), h_length_dist, h_code_dist);
				}
				code_tree_ptr = code_tree;
				if (code_tree != nullptr)
				{
					for (; ; )
					{
						if (GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr))
							code_tree_ptr = code_tree_ptr->One;
						else
							code_tree_ptr = code_tree_ptr->Zero;
						if (code_tree_ptr == nullptr)
							THROW(0xe9170008);
						if (code_tree_ptr->Zero == nullptr && code_tree_ptr->One == nullptr)
						{
							U32 value = code_tree_ptr->Data;
							if (value < 256)
							{
								png_data->WindowBuf[window_used] = static_cast<U8>(value);
								window_used++;
								window_used &= window_size - 1;
								Output(png_data, argb, static_cast<U8>(value));
							}
							else if (value == 256)
								break;
							else if (value <= 285)
							{
								U32 copy_length, dist_code, back_dist;
								{
									if (value <= 264)
										copy_length = 3 + (value - 257);
									else if (value >= 285)
										copy_length = 258;
									else
									{
										U32 ext_bits = 1 + (value - 265) / 4;
										U32 base = (8 << ((value - 265) / 4)) + 3;
										U32 offset = ((value - 265) & 3) * (2 << ((value - 265) / 4));
										copy_length = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, static_cast<int>(ext_bits));
										copy_length += base + offset;
									}
								}
								if (btype == 1)
								{
									dist_code = 16 * GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr);
									dist_code += 8 * GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr);
									dist_code += 4 * GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr);
									dist_code += 2 * GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr);
									dist_code += GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr);
								}
								else
								{
									dist_tree_ptr = dist_tree;
									while (dist_tree_ptr->Zero != nullptr || dist_tree_ptr->One != nullptr)
									{
										if (GetNextBit(png_data, &cur_data, &byte_ptr, &bit_ptr))
											dist_tree_ptr = dist_tree_ptr->One;
										else
											dist_tree_ptr = dist_tree_ptr->Zero;
									}
									dist_code = dist_tree_ptr->Data;
								}
								{
									if (dist_code <= 3)
										back_dist = dist_code + 1;
									else
									{
										U32 ext_bits = (dist_code - 2) / 2;
										U32 base = (4 << ((dist_code - 4) / 2)) + 1;
										U32 offset = (dist_code & 1) * (2 << ((dist_code - 4) / 2));
										back_dist = GetNextMultiBit(png_data, &cur_data, &byte_ptr, &bit_ptr, ext_bits);
										back_dist += base + offset;
									}
								}
								for (int i = 0; i < static_cast<int>(copy_length); i++)
								{
									U8 data = png_data->WindowBuf[(window_used - back_dist) & (window_size - 1)];
									Output(png_data, argb, data);
									png_data->WindowBuf[window_used] = data;
									window_used++;
									window_used &= window_size - 1;
								}
							}
							code_tree_ptr = code_tree;
						}
					}
				}
				DeleteHuffmanTree(code_tree);
				DeleteHuffmanTree(dist_tree);
				code_tree = nullptr;
				dist_tree = nullptr;
			}
			else
				THROW(0xe9170008);
			if (bfinal != 0)
				break;
		}
	}
	FreeMem(png_data->WindowBuf);
	png_data->WindowBuf = nullptr;
	FreeMem(png_data->TwoLine);
	png_data->TwoLine = nullptr;
}

static void Output(SPngData* png_data, U8* argb, U8 data)
{
	if (png_data->Y > png_data->Height)
		THROW(0xe9170008);
	if (png_data->X == -1)
	{
		png_data->Filter = data;
		png_data->InlineCnt = 0;
		png_data->InpixelCnt = 0;
		png_data->X++;
		return;
	}

	png_data->CurLine[png_data->InlineCnt + png_data->InpixelCnt] = data;
	png_data->InpixelCnt++;
	if (png_data->InterlaceMethod == 0)
	{
		// Non-interlace.
		switch (png_data->ColorType)
		{
			case 0: // Grayscale.
				if (png_data->InpixelCnt == 1)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 1, png_data->Filter);
					argb[png_data->Idx + 0] = png_data->CurLine[png_data->X];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->X];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->X];
					argb[png_data->Idx + 3] = 255;
					png_data->Idx += 4;
					png_data->X++;
					png_data->InlineCnt++;
					png_data->InpixelCnt = 0;
				}
				break;
			case 2: // Color.
				if (png_data->InpixelCnt == 3)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 3, png_data->Filter);
					argb[png_data->Idx + 0] = png_data->CurLine[png_data->InlineCnt + 0];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->InlineCnt + 1];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->InlineCnt + 2];
					argb[png_data->Idx + 3] = 255;
					png_data->Idx += 4;
					png_data->X++;
					png_data->InlineCnt += 3;
					png_data->InpixelCnt = 0;
				}
				break;
			case 4: // Grayscale with alpha.
				if (png_data->InpixelCnt == 2)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 2, png_data->Filter);
					argb[png_data->Idx + 0] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 3] = png_data->CurLine[png_data->InlineCnt + 1];
					png_data->Idx += 4;
					png_data->X++;
					png_data->InlineCnt += 2;
					png_data->InpixelCnt = 0;
				}
				break;
			case 6: // Color with alpha.
				if (png_data->InpixelCnt == 4)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 4, png_data->Filter);
					argb[png_data->Idx + 0] = png_data->CurLine[png_data->InlineCnt + 0];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->InlineCnt + 1];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->InlineCnt + 2];
					argb[png_data->Idx + 3] = png_data->CurLine[png_data->InlineCnt + 3];
					png_data->Idx += 4;
					png_data->X++;
					png_data->InlineCnt += 4;
					png_data->InpixelCnt = 0;
				}
				break;
			default:
				THROW(0xe9170008);
				break;
		}
		if (png_data->X >= png_data->Width)
		{
			png_data->Y++;
			png_data->X = -1;
			if (png_data->TwoLine != nullptr)
			{
				U8* swap = png_data->CurLine;
				png_data->CurLine = png_data->PrvLine;
				png_data->PrvLine = swap;
			}
		}
	}
	else
	{
		// Interlace.
		if (png_data->InterlaceMethod != 1)
			THROW(0xe9170008);
		int ilw = 0, ilh = 0, ilx = 0, ily = 0;
		switch (png_data->InterlacePass)
		{
			case 1:
				ilw = (png_data->Width + 7) / 8;
				ilh = (png_data->Height + 7) / 8;
				ilx = png_data->X * 8;
				ily = png_data->Y * 8;
				break;
			case 2:
				ilw = (png_data->Width + 3) / 8;
				ilh = (png_data->Height + 7) / 8;
				ilx = 4 + png_data->X * 8;
				ily = png_data->Y * 8;
				break;
			case 3:
				ilw = (png_data->Width + 3) / 4;
				ilh = (png_data->Height + 3) / 8;
				ilx = png_data->X * 4;
				ily = 4 + png_data->Y * 8;
				break;
			case 4:
				ilw = (png_data->Width + 1) / 4;
				ilh = (png_data->Height + 3) / 4;
				ilx = 2 + png_data->X * 4;
				ily = png_data->Y * 4;
				break;
			case 5:
				ilw = (png_data->Width + 1) / 2;
				ilh = (png_data->Height + 1) / 4;
				ilx = png_data->X * 2;
				ily = 2 + png_data->Y * 4;
				break;
			case 6:
				ilw = png_data->Width / 2;
				ilh = (png_data->Height + 1) / 2;
				ilx = 1 + png_data->X * 2;
				ily = png_data->Y * 2;
				break;
			case 7:
				ilw = png_data->Width;
				ilh = png_data->Height / 2;
				ilx = png_data->X;
				ily = 1 + png_data->Y * 2;
				break;
			default:
				THROW(0xe9170008);
				break;
		}
		switch (png_data->ColorType)
		{
			case 0: // Grayscale.
				if (png_data->InpixelCnt == 1)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 1, png_data->Filter);
					png_data->Idx = ilx * 4 + ily * png_data->Width * 4;
					argb[png_data->Idx] = png_data->CurLine[png_data->X];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->X];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->X];
					argb[png_data->Idx + 3] = 255;
					png_data->X++;
					png_data->InlineCnt++;
					png_data->InpixelCnt = 0;
				}
				break;
			case 2: // Color.
				if (png_data->InpixelCnt == 3)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 3, png_data->Filter);
					png_data->Idx = ilx * 4 + ily * png_data->Width * 4;
					argb[png_data->Idx] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->InlineCnt + 1];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->InlineCnt + 2];
					argb[png_data->Idx + 3] = 255;
					png_data->X++;
					png_data->InlineCnt += 3;
					png_data->InpixelCnt = 0;
				}
				break;
			case 4: // Grayscale with alpha.
				if (png_data->InpixelCnt == 2)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 2, png_data->Filter);
					png_data->Idx = ilx * 4 + ily * png_data->Width * 4;
					argb[png_data->Idx] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 3] = png_data->CurLine[png_data->InlineCnt + 1];
					png_data->X++;
					png_data->InlineCnt += 2;
					png_data->InpixelCnt = 0;
				}
				break;
			case 6: // Color with alpha.
				if (png_data->InpixelCnt == 4)
				{
					Filter8(png_data->CurLine, png_data->PrvLine, png_data->X, png_data->Y, 4, png_data->Filter);
					png_data->Idx = ilx * 4 + ily * png_data->Width * 4;
					argb[png_data->Idx] = png_data->CurLine[png_data->InlineCnt];
					argb[png_data->Idx + 1] = png_data->CurLine[png_data->InlineCnt + 1];
					argb[png_data->Idx + 2] = png_data->CurLine[png_data->InlineCnt + 2];
					argb[png_data->Idx + 3] = png_data->CurLine[png_data->InlineCnt + 3];
					png_data->X++;
					png_data->InlineCnt += 4;
					png_data->InpixelCnt = 0;
				}
				break;
			default:
				THROW(0xe9170008);
				break;
		}
		if (png_data->X >= ilw)
		{
			png_data->Y++;
			png_data->X = -1;
			if (png_data->TwoLine != nullptr)
			{
				U8* swap = png_data->CurLine;
				png_data->CurLine = png_data->PrvLine;
				png_data->PrvLine = swap;
			}
			if (png_data->Y >= ilh)
			{
				png_data->Y = 0;
				png_data->InterlacePass++;
			}
		}
	}
}

static U32 GetNextBit(SPngData* png_data, int* cur_data, U32* byte_ptr, U32* bit_ptr)
{
	U32 a = png_data->Data[*cur_data][*byte_ptr] & (*bit_ptr);
	*bit_ptr <<= 1;
	if (*bit_ptr >= 256)
	{
		*bit_ptr = 1;
		Step(png_data, cur_data, byte_ptr, 1);
	}
	return a != 0 ? 1 : 0;
}

static U32 GetNextMultiBit(SPngData* png_data, int* cur_data, U32* byte_ptr, U32* bit_ptr, int n)
{
	U32 v = 0, m = 1;
	for (int i = 0; i < n; i++)
	{
		if (GetNextBit(png_data, cur_data, byte_ptr, bit_ptr))
			v |= m;
		m <<= 1;
	}
	return v;
}

static SHuffmanTree* MakeHuffmanTree(int n, U32* h_length, U32* h_code)
{
	SHuffmanTree* result = NewHuffmanTree();
	SHuffmanTree* ptr;
	for (int i = 0; i < n; i++)
	{
		if (h_length[i] > 0)
		{
			ptr = result;
			U32 mask = 1 << (h_length[i] - 1);
			for (int j = 0; j < static_cast<int>(h_length[i]); j++)
			{
				if ((h_code[i] & mask) != 0)
				{
					if (ptr->One == nullptr)
						ptr->One = NewHuffmanTree();
					ptr = ptr->One;
				}
				else
				{
					if (ptr->Zero == nullptr)
						ptr->Zero = NewHuffmanTree();
					ptr = ptr->Zero;
				}
				mask >>= 1;
			}
			ptr->Data = static_cast<U32>(i);
		}
	}
	return result;
}

static SHuffmanTree* NewHuffmanTree()
{
	SHuffmanTree* result = static_cast<SHuffmanTree*>(AllocMem(sizeof(SHuffmanTree)));
	result->Zero = nullptr;
	result->One = nullptr;
	result->Data = 0x7fffffff;
	result->Weight = 0;
	result->Depth = 1;
	return result;
}

static void DeleteHuffmanTree(SHuffmanTree* node)
{
	if (node != nullptr)
	{
		DeleteHuffmanTree(node->Zero);
		DeleteHuffmanTree(node->One);
		FreeMem(node);
	}
}

static void MakeDynamicHuffmanCode(U32* h_length, U32* h_code, int n_lng, U32* lng)
{
	for (int i = 0; i < n_lng; i++)
	{
		h_length[i] = lng[i];
		h_code[i] = 0;
	}
	U32 max_lng = 0;
	for (int i = 0; i < n_lng; i++)
	{
		if (max_lng < lng[i])
			max_lng = lng[i];
	}
	U32* bl_count = static_cast<U32*>(AllocMem(sizeof(U32) * (max_lng + 1)));
	U32* next_code = static_cast<U32*>(AllocMem(sizeof(U32) * (max_lng + 1)));
	for (int i = 0; i < static_cast<int>(max_lng + 1); i++)
	{
		bl_count[i] = 0;
		next_code[i] = 0;
	}
	for (int i = 0; i < n_lng; i++)
		bl_count[lng[i]]++;
	U32 code = 0;
	bl_count[0] = 0;
	for (int i = 1; i <= static_cast<int>(max_lng); i++)
	{
		code = (code + bl_count[i - 1]) << 1;
		next_code[i] = code;
	}
	for (int i = 0; i < n_lng; i++)
	{
		U32 len = lng[i];
		if (len > 0)
		{
			h_code[i] = next_code[len];
			next_code[len]++;
		}
	}
	FreeMem(bl_count);
	FreeMem(next_code);
}

static void Filter8(U8* cur_line, U8* prv_line, int x, int y, int unit_lng, int filter)
{
	switch (filter)
	{
		case 1:
			if (x > 0)
			{
				for (int i = 0; i < unit_lng; i++)
					cur_line[x * unit_lng + i] += cur_line[x * unit_lng + i - unit_lng];
			}
			break;
		case 2:
			if (y > 0)
			{
				for (int i = 0; i < unit_lng; i++)
					cur_line[x * unit_lng + i] += prv_line[x * unit_lng + i];
			}
			break;
		case 3:
			for (int i = 0; i < unit_lng; i++)
			{
				U32 a = x > 0 ? cur_line[x * unit_lng + i - unit_lng] : 0;
				a += y > 0 ? prv_line[x * unit_lng + i] : 0;
				cur_line[x * unit_lng + i] += static_cast<U8>(a / 2);
			}
			break;
		case 4:
			for (int i = 0; i < unit_lng; i++)
			{
				U32 a = x > 0 ? cur_line[x * unit_lng + i - unit_lng] : 0;
				U32 b = y > 0 ? prv_line[x * unit_lng + i] : 0;
				U32 c = (x > 0 && y > 0) ? prv_line[x * unit_lng - unit_lng + i] : 0;
				U8 paeth;
				{
					int aa = (int)a, bb = (int)b, cc = (int)c;
					int p = aa + bb - cc;
					int pa = p > aa ? p - aa : aa - p, pb = p > bb ? p - bb : bb - p, pc = p > cc ? p - cc : cc - p;
					if (pa <= pb && pa <= pc)
						paeth = static_cast<U8>(aa);
					else if (pb <= pc)
						paeth = static_cast<U8>(bb);
					else
						paeth = static_cast<U8>(cc);
				}
				cur_line[x * unit_lng + i] += paeth;
			}
			break;
	}
}

static void Step(SPngData* png_data, int* cur_data, U32* byte_ptr, int n)
{
	*byte_ptr += n;
	if (*byte_ptr >= png_data->DataSize[*cur_data])
	{
		*byte_ptr -= static_cast<U32>(png_data->DataSize[*cur_data]);
		(*cur_data)++;
	}
}

static U8 SwapEndianU8(U8 n)
{
	return n;
}

static U32 SwapEndianU32(U32 n)
{
	n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
	n = ((n & 0x0000ffff) << 16) | ((n & 0xffff0000) >> 16);
	return n;
}
