#include "bc_decoder.h"

void* DecodeBc(size_t size, const void* data, int* width, int* height)
{
	UNUSED(size);

	const U8* ptr = static_cast<const U8*>(data);
	if (*reinterpret_cast<const DWORD*>(ptr) != 0x20534444)
		THROW(0xe9170008); // ' SDD'
	ptr += sizeof(DWORD);
	if (*reinterpret_cast<const DWORD*>(ptr) != 124)
		THROW(0xe9170008); // Always 124.
	ptr += sizeof(DWORD);
	// DWORD header_flags = *reinterpret_cast<const DWORD*>(ptr);
	ptr += sizeof(DWORD);
	DWORD header_height = *reinterpret_cast<const DWORD*>(ptr);
	ptr += sizeof(DWORD);
	DWORD header_width = *reinterpret_cast<const DWORD*>(ptr);
	ptr += sizeof(DWORD);
	// DWORD header_pitch_or_linear_size
	ptr += sizeof(DWORD);
	if (*reinterpret_cast<const DWORD*>(ptr) != 1)
		THROW(0xe9170008); // DWORD header_depth
	ptr += sizeof(DWORD);
	if (*reinterpret_cast<const DWORD*>(ptr) != 1)
		THROW(0xe9170008); // DWORD header_mip_map
	ptr += sizeof(DWORD);
	for (int i = 0; i < 11; i++)
		ptr += sizeof(DWORD);
	if (*reinterpret_cast<const DWORD*>(ptr) != 32)
		THROW(0xe9170008); // Always 32.
	ptr += sizeof(DWORD);
	if ((*reinterpret_cast<const DWORD*>(ptr) & 0x00000004) == 0)
		THROW(0xe9170008);
	ptr += sizeof(DWORD);
	if (*reinterpret_cast<const DWORD*>(ptr) != 0x30315844)
		THROW(0xe9170008); // 'DX10'
	ptr += sizeof(DWORD);
	for (int i = 0; i < 11; i++)
		ptr += sizeof(DWORD);

	*width = static_cast<int>(header_width);
	*height = static_cast<int>(header_height);
	return const_cast<U8*>(ptr); // 'Const' is removed to fit the interface, but of course it must not be rewritten.
}
