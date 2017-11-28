// LibOgg.dll
//
// (C)Kuina-chan
//

#include "main.h"

static size_t OggCBRead(void* ptr, size_t size, size_t nmemb, void* data);
static int OggCBSeek(void* data, ogg_int64_t offset, int whence);
static int OggCBClose(void* data);
static long OggCBTell(void* data);
static void OggClose(void* buf);
static Bool OggRead(void* buf, void* data, S64 size, S64 loop_pos);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void* LoadOgg(const Char* path, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64))
{
	OggVorbis_File* file = (OggVorbis_File*)AllocMem(sizeof(OggVorbis_File));
	ov_callbacks cb = { OggCBRead, OggCBSeek, OggCBClose, OggCBTell };
	if (ov_open_callbacks(OpenFileStream(path), file, NULL, 0, cb) < 0)
		return NULL;
	if (!ov_seekable(file))
	{
		ov_clear(file);
		return NULL;
	}
	{
		vorbis_info* info = ov_info(file, -1);
		*channel = (S64)info->channels;
		*samples_per_sec = (S64)info->rate;
		*bits_per_sample = 16;
		*total = (S64)(info->channels * (16 / 8) * ov_pcm_total(file, -1));
		*func_close = OggClose;
		*func_read = OggRead;
	}
	return file;
}

EXPORT void init(void* heap, S64* heap_cnt, S64 app_code, const Char* app_name)
{
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name;
	Instance = (HINSTANCE)GetModuleHandle(NULL);
}

static size_t OggCBRead(void* ptr, size_t size, size_t nmemb, void* data)
{
	return ReadFileStream(data, size * nmemb, ptr) / size;
}

static int OggCBSeek(void* data, ogg_int64_t offset, int whence)
{
	SeekFileStream(data, offset, whence);
	return 0;
}

static int OggCBClose(void* data)
{
	CloseFileStream(data);
	return 0;
}

static long OggCBTell(void* data)
{
	return (long)TellFileStream(data);
}

static void OggClose(void* buf)
{
	OggVorbis_File* file = (OggVorbis_File*)buf;
	ov_clear(file);
	FreeMem(file);
}

static Bool OggRead(void* buf, void* data, S64 size, S64 loop_pos)
{
	OggVorbis_File* file = (OggVorbis_File*)buf;
	char* dst = (char*)data;
	int remain = (int)size;
	while (remain > 0)
	{
		int actual_read = (int)ov_read(file, dst, remain, 0, 2, 1, NULL);
		if (actual_read <= 0)
		{
			if (loop_pos == -1)
			{
				memset(dst, 0x00, (size_t)remain);
				return True;
			}
			ov_pcm_seek(file, loop_pos / (S64)(ov_info(file, -1)->channels * 2));
			continue;
		}
		remain -= actual_read;
		dst += actual_read;
	}
	return False;
}
