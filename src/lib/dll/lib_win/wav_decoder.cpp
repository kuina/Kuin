#include "wav_decoder.h"

struct SWav
{
	size_t Size;
	const U8* Data;
	const U8* Cur;
	Bool Steleo;
	U32 SamplesPerSec;
	U32 BitsPerSample;
	U32 WaveSize;
	S64 Begin;
};

static void Close(void* handle);
static Bool Read(void* handle, void* buf, S64 size, S64 loop_pos);
static void CloseFileStream(SWav* handle);
static size_t ReadFileStream(SWav* handle, size_t size, void* buf);
static Bool SeekFileStream(SWav* handle, S64 offset, S64 origin);
static S64 TellFileStream(SWav* handle);

void* LoadWav(size_t size, const U8* data, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64))
{
	*func_close = Close;
	*func_read = Read;

	SWav* result = static_cast<SWav*>(AllocMem(sizeof(SWav)));
	Bool success = False;
	for (; ; )
	{
		result->Size = size;
		result->Data = data;
		result->Cur = data;

		U8 str[4];
		if (ReadFileStream(result, 4, str) != 4)
			break;
		if (str[0] != 0x52 || str[1] != 0x49 || str[2] != 0x46 || str[3] != 0x46) // 'RIFF'
			break;
		if (!SeekFileStream(result, 4, SEEK_CUR))
			break;
		if (ReadFileStream(result, 4, str) != 4)
			break;
		if (str[0] != 0x57 || str[1] != 0x41 || str[2] != 0x56 || str[3] != 0x45) // 'WAVE'
			break;

		Bool fmt_chunk = False;
		Bool data_chunk = False;
		for (; ; )
		{
			U32 size2;
			if (ReadFileStream(result, 4, str) != 4)
				break;
			if (ReadFileStream(result, 4, &size2) != 4)
				break;
			if (str[0] == 0x66 && str[1] == 0x6d && str[2] == 0x74 && str[3] == 0x20) // 'fmt '
			{
				if ((size_t)size2 != sizeof(PCMWAVEFORMAT))
					break;
				{
					PCMWAVEFORMAT fmt;
					if (ReadFileStream(result, sizeof(PCMWAVEFORMAT), &fmt) != sizeof(PCMWAVEFORMAT))
						break;
					if (fmt.wf.wFormatTag != 1)
						break;
					switch (fmt.wf.nChannels)
					{
						case 1: result->Steleo = False; break;
						case 2: result->Steleo = True; break;
						default:
							THROW(0xe9170008);
							break;
					}
					*channel = static_cast<S64>(fmt.wf.nChannels);
					result->SamplesPerSec = static_cast<U32>(fmt.wf.nSamplesPerSec);
					*samples_per_sec = static_cast<S64>(fmt.wf.nSamplesPerSec);
					result->BitsPerSample = static_cast<U32>(fmt.wBitsPerSample);
					*bits_per_sample = static_cast<S64>(fmt.wBitsPerSample);
				}
				fmt_chunk = True;
				continue;
			}
			if (str[0] == 0x64 && str[1] == 0x61 && str[2] == 0x74 && str[3] == 0x61) // 'data'
			{
				result->WaveSize = size2;
				*total = static_cast<S64>(size2);
				data_chunk = True;
				break;
			}
			if (!SeekFileStream(result, size2, SEEK_CUR))
				break;
		}
		if (!fmt_chunk || !data_chunk)
			break;
		result->Begin = TellFileStream(result);
		success = True;
		break;
	}
	if (!success)
	{
		FreeMem(result);
		return nullptr;
	}

	return result;
}

static void Close(void* handle)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	CloseFileStream(handle2);
	FreeMem(handle2);
}

static Bool Read(void* handle, void* buf, S64 size, S64 loop_pos)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	U8* dst = static_cast<U8*>(buf);
	S64 remain = size;
	while (remain > 0)
	{
		S64 actual_read = static_cast<S64>(ReadFileStream(handle2, remain, dst));
		if (actual_read <= 0)
		{
			if (loop_pos == -1)
			{
				memset(dst, 0x00, static_cast<size_t>(remain));
				return True;
			}
			S64 align = handle2->BitsPerSample / 8 * (handle2->Steleo ? 2 : 1);
			SeekFileStream(handle2, handle2->Begin + loop_pos / align * align, SEEK_SET);
			continue;
		}
		remain -= actual_read;
		dst += actual_read;
	}
	return False;
}

static void CloseFileStream(SWav* handle)
{
	UNUSED(handle);
	// Do nothing.
}

static size_t ReadFileStream(SWav* handle, size_t size, void* buf)
{
	size_t remain = handle->Size - (handle->Cur - handle->Data);
	if (size > remain)
		size = remain;
	memcpy(buf, handle->Cur, size);
	handle->Cur += size;
	return size;
}

static Bool SeekFileStream(SWav* handle, S64 offset, S64 origin)
{
	const U8* pos;
	switch (origin)
	{
		case SEEK_SET: pos = handle->Data + offset; break;
		case SEEK_CUR: pos = handle->Cur + offset; break;
		case SEEK_END: pos = handle->Data + handle->Size + offset; break;
			break;
		default:
			return False;
	}
	if (handle->Data + handle->Size < pos)
		return False;
	handle->Cur = pos;
	return True;
}

static S64 TellFileStream(SWav* handle)
{
	return handle->Cur - handle->Data;
}
