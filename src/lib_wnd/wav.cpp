#include "wav.h"

struct SWav
{
	void* FileStream;
	Bool Steleo;
	U32 SamplesPerSec;
	U32 BitsPerSample;
	U32 WaveSize;
	S64 Begin;
};

static void Close(void* handle);
static Bool Read(void* handle, void* buf, S64 size, S64 loop_pos);
static void SetPos(void* handle, S64 pos);
static S64 GetPos(void* handle);

void* LoadWav(const Char* path, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64), void(**func_set_pos)(void*, S64), S64(**func_get_pos)(void*))
{
	*func_close = Close;
	*func_read = Read;
	*func_set_pos = SetPos;
	*func_get_pos = GetPos;

	SWav* result = static_cast<SWav*>(AllocMem(sizeof(SWav)));
	Bool success = False;
	for (; ; )
	{
		result->FileStream = OpenFileStream(path);
		if (result->FileStream == NULL)
		{
			THROW(EXCPT_FILE_READ_FAILED);
			break;
		}

		U8 str[4];
		if (ReadFileStream(result->FileStream, 4, str) != 4)
			break;
		if (str[0] != 0x52 || str[1] != 0x49 || str[2] != 0x46 || str[3] != 0x46) // 'RIFF'
			break;
		if (!SeekFileStream(result->FileStream, 4, SEEK_CUR))
			break;
		if (ReadFileStream(result->FileStream, 4, str) != 4)
			break;
		if (str[0] != 0x57 || str[1] != 0x41 || str[2] != 0x56 || str[3] != 0x45) // 'WAVE'
			break;

		Bool fmt_chunk = False;
		Bool data_chunk = False;
		for (; ; )
		{
			U32 size;
			if (ReadFileStream(result->FileStream, 4, str) != 4)
				break;
			if (ReadFileStream(result->FileStream, 4, &size) != 4)
				break;
			if (str[0] == 0x66 && str[1] == 0x6d && str[2] == 0x74 && str[3] == 0x20) // 'fmt '
			{
				if ((size_t)size != sizeof(PCMWAVEFORMAT))
					break;
				{
					PCMWAVEFORMAT fmt;
					if (ReadFileStream(result->FileStream, sizeof(PCMWAVEFORMAT), &fmt) != sizeof(PCMWAVEFORMAT))
						break;
					if (fmt.wf.wFormatTag != 1)
						break;
					switch (fmt.wf.nChannels)
					{
						case 1: result->Steleo = False; break;
						case 2: result->Steleo = True; break;
						default:
							THROW(EXCPT_INVALID_DATA_FMT);
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
				result->WaveSize = size;
				*total = static_cast<S64>(size);
				data_chunk = True;
				break;
			}
			if (!SeekFileStream(result->FileStream, size, SEEK_CUR))
				break;
		}
		if (!fmt_chunk || !data_chunk)
			break;
		result->Begin = TellFileStream(result->FileStream);
		success = True;
		break;
	}
	if (!success)
	{
		FreeMem(result);
		return NULL;
	}

	return result;
}

static void Close(void* handle)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	CloseFileStream(handle2->FileStream);
	FreeMem(handle2);
}

static Bool Read(void* handle, void* buf, S64 size, S64 loop_pos)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	U8* dst = static_cast<U8*>(buf);
	S64 remain = size;
	while (remain > 0)
	{
		S64 actual_read = static_cast<S64>(ReadFileStream(handle2->FileStream, remain, dst));
		if (actual_read <= 0)
		{
			if (loop_pos == -1)
			{
				memset(dst, 0x00, static_cast<size_t>(remain));
				return True;
			}
			S64 align = handle2->BitsPerSample / 8 * (handle2->Steleo ? 2 : 1);
			SeekFileStream(handle2->FileStream, handle2->Begin + loop_pos / align * align, SEEK_SET);
			continue;
		}
		remain -= actual_read;
		dst += actual_read;
	}
	return False;
}

static void SetPos(void* handle, S64 pos)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	S64 align = handle2->BitsPerSample / 8 * (handle2->Steleo ? 2 : 1);
	SeekFileStream(handle2->FileStream, handle2->Begin + pos / align * align, SEEK_SET);
}

static S64 GetPos(void* handle)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	return TellFileStream(handle2->FileStream) - handle2->Begin;
}
