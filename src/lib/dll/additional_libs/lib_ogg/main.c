// LibOgg.dll
//
// (C)Kuina-chan
//

#include "main.h"

typedef struct SHandle
{
	size_t Size;
	const U8* Data;
	const U8* Cur;
	OggVorbis_File* File;
} SHandle;

static size_t OggCBRead(void* ptr, size_t size, size_t nmemb, void* data);
static int OggCBSeek(void* data, ogg_int64_t offset, int whence);
static int OggCBClose(void* data);
static long OggCBTell(void* data);
static void OggClose(void* buf);
static Bool OggRead(void* buf, void* data, S64 size, S64 loop_pos);
static void CloseFileStream(SHandle* handle);
static size_t ReadFileStream(SHandle* handle, size_t size, void* buf);
static Bool SeekFileStream(SHandle* handle, S64 offset, S64 origin);
static S64 TellFileStream(SHandle* handle);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void* LoadOgg(size_t size, const U8* data, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64))
{
	OggVorbis_File* file = (OggVorbis_File*)AllocMem(sizeof(OggVorbis_File));
	ov_callbacks cb = { OggCBRead, OggCBSeek, OggCBClose, OggCBTell };
	SHandle* handle = (SHandle*)AllocMem(sizeof(SHandle));
	handle->Size = size;
	handle->Data = data;
	handle->Cur = data;
	handle->File = file;
	if (ov_open_callbacks(handle, file, NULL, 0, cb) < 0)
		return NULL;
	if (!ov_seekable(file))
	{
		ov_clear(file);
		FreeMem(file);
		FreeMem(handle);
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
	return handle;
}

EXPORT void init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);
}

static size_t OggCBRead(void* ptr, size_t size, size_t nmemb, void* data)
{
	return ReadFileStream((SHandle*)data, size * nmemb, ptr) / size;
}

static int OggCBSeek(void* data, ogg_int64_t offset, int whence)
{
	SeekFileStream((SHandle*)data, offset, whence);
	return 0;
}

static int OggCBClose(void* data)
{
	CloseFileStream((SHandle*)data);
	return 0;
}

static long OggCBTell(void* data)
{
	return (long)TellFileStream((SHandle*)data);
}

static void OggClose(void* buf)
{
	SHandle* handle = (SHandle*)buf;
	ov_clear(handle->File);
	FreeMem(handle->File);
	FreeMem(handle);
}

static Bool OggRead(void* buf, void* data, S64 size, S64 loop_pos)
{
	SHandle* handle = (SHandle*)buf;
	char* dst = (char*)data;
	int remain = (int)size;
	while (remain > 0)
	{
		int actual_read = (int)ov_read(handle->File, dst, remain, 0, 2, 1, NULL);
		if (actual_read <= 0)
		{
			if (loop_pos == -1)
			{
				memset(dst, 0x00, (size_t)remain);
				return True;
			}
			ov_pcm_seek(handle->File, loop_pos / (S64)(ov_info(handle->File, -1)->channels * 2));
			continue;
		}
		remain -= actual_read;
		dst += actual_read;
	}
	return False;
}

static void CloseFileStream(SHandle* handle)
{
	UNUSED(handle);
	// Do nothing.
}

static size_t ReadFileStream(SHandle* handle, size_t size, void* buf)
{
	size_t remain = handle->Size - (handle->Cur - handle->Data);
	if (size > remain)
		size = remain;
	memcpy(buf, handle->Cur, size);
	handle->Cur += size;
	return size;
}

static Bool SeekFileStream(SHandle* handle, S64 offset, S64 origin)
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

static S64 TellFileStream(SHandle* handle)
{
	return handle->Cur - handle->Data;
}

