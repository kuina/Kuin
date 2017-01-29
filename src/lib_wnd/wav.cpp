#include "wav.h"

struct SWav
{
	void* FileStream;
	Bool Steleo;
	U32 SamplesPerSec;
	U32 BitsPerSample;
	U32 WaveSize;
};

static void Close(void* handle);
static Bool Read(void* handle, void* buf, S64 size, S64 looppos);

void* LoadWav(const Char* path, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64))
{
	*func_close = Close;
	*func_read = Read;

	SWav* result = static_cast<SWav*>(AllocMem(sizeof(SWav)));
	Bool success = False;
	for (; ; )
	{
		result->FileStream = OpenFileStream(path);
		if (result->FileStream == NULL)
			break;

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
							THROW(0x1000, L"");
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

static Bool Read(void* handle, void* buf, S64 size, S64 looppos)
{
	SWav* handle2 = reinterpret_cast<SWav*>(handle);
	S64 size2 = static_cast<S64>(ReadFileStream(handle2->FileStream, size, buf));
	if (size != size2)
	{
		memset(static_cast<U8*>(buf) + size2, 0x00, static_cast<size_t>(size - size2));
		return True;
	}
	return False;
}
