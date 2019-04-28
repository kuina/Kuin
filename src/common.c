#include "common.h"

typedef struct SClassTable
{
	void* Addr;
	void(*Ctor)(struct SClass* me_);
	void(*Dtor)(struct SClass* me_);
	int(*Cmp)(const struct SClass* me_, const struct SClass* t);
	struct SClass*(*Copy)(const struct SClass* me_);
	U8*(*ToBin)(const struct SClass* me_);
	S64(*FromBin)(struct SClass* me_, U8* bin, S64 idx);
} SClassTable;

typedef struct SFile
{
	Bool Pack;
	FILE* Handle;
	S64 Pos;
	S64 Size;
	S64 Cur;
	U64 Key;
} SFile;

SEnvVars EnvVars;

#if !defined(DBG)
static SFile* OpenPackFile(const Char* path);
#endif
static U8 GetKey(U64 key, U8 data, U64 pos);

Bool InitEnvVars(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (EnvVars.Heap != NULL)
		return False;

	EnvVars.Heap = heap;
#if defined(_DEBUG)
	EnvVars.HeapCnt = heap_cnt;
#else
	UNUSED(heap_cnt);
#endif
	EnvVars.AppCode = app_code;
	EnvVars.UseResFlags = use_res_flags;

	// The resource root directory.
#if defined(DBG)
	{
		Char cur_dir_path[KUIN_MAX_PATH + 12 + 1];
		GetModuleFileName(NULL, cur_dir_path, KUIN_MAX_PATH);
		{
			Char* ptr = wcsrchr(cur_dir_path, L'\\');
			if (ptr != NULL)
				*(ptr + 1) = L'\0';
		}
		wcscat(cur_dir_path, L"_curdir_.txt");
		if (PathFileExists(cur_dir_path))
		{
			Char path[KUIN_MAX_PATH + 1];
			FILE* file_ptr = _wfopen(cur_dir_path, L"r, ccs=UTF-8");
			fgetws(path, KUIN_MAX_PATH, file_ptr);
			{
				Char* ptr = path;
				while (ptr[1] != L'\0')
					ptr++;
				while (ptr >= path && (*ptr == L'\n' || *ptr == L'\r'))
				{
					*ptr = L'\0';
					ptr--;
				}
			}
			wcscpy(EnvVars.ResRoot, path);
		}
		else
		{
			Char* ptr;
			GetModuleFileName(NULL, EnvVars.ResRoot, KUIN_MAX_PATH);
			ptr = wcsrchr(EnvVars.ResRoot, L'\\');
			if (ptr != NULL)
				*(ptr + 1) = L'\0';
			ptr = EnvVars.ResRoot;
			while (*ptr != L'\0')
			{
				if (*ptr == L'\\')
					*ptr = L'/';
				ptr++;
			}
		}
	}
#else
	{
		Char* ptr;
		GetModuleFileName(NULL, EnvVars.ResRoot, KUIN_MAX_PATH);
		ptr = wcsrchr(EnvVars.ResRoot, L'\\');
		if (ptr != NULL)
			*(ptr + 1) = L'\0';
		ptr = EnvVars.ResRoot;
		while (*ptr != L'\0')
		{
			if (*ptr == L'\\')
				*ptr = L'/';
			ptr++;
		}
	}
#endif

	return True;
}

void* AllocMem(size_t size)
{
	void* result = HeapAlloc(EnvVars.Heap, HEAP_GENERATE_EXCEPTIONS, (SIZE_T)size);
#if defined(_DEBUG)
	memset(result, 0xcd, size);
	(*EnvVars.HeapCnt)++;
#endif
	return result;
}

void* ReAllocMem(void* ptr, size_t size)
{
	if (ptr == NULL)
		return HeapAlloc(EnvVars.Heap, HEAP_GENERATE_EXCEPTIONS, (SIZE_T)size);
	else
		return HeapReAlloc(EnvVars.Heap, HEAP_GENERATE_EXCEPTIONS, ptr, (SIZE_T)size);
}

void FreeMem(void* ptr)
{
	HeapFree(EnvVars.Heap, 0, ptr);
#if defined(_DEBUG)
	(*EnvVars.HeapCnt)--;
	ASSERT(*EnvVars.HeapCnt >= 0);
#endif
}

void ThrowImpl(U32 code)
{
	RaiseException((DWORD)code, 0, 0, NULL);
}

void* LoadFileAll(const Char* path, size_t* size)
{
#if !defined(DBG)
	if (path[0] == L'r' && path[1] == L'e' && path[2] == L's' && path[3] == L'/')
	{
		SFile* handle = OpenPackFile(path + 4);
		if (handle == NULL)
			return NULL;
		void* result = AllocMem((size_t)handle->Size);
		fread(result, 1, (size_t)handle->Size, handle->Handle);
		U8* ptr = (U8*)result;
		S64 i;
		U64 pos = (U64)handle->Pos - (U64)sizeof(U64) * 3;
		for (i = 0; i < handle->Size; i++)
		{
			ptr[i] = GetKey(handle->Key, ptr[i], pos);
			pos++;
		}
		*size = (size_t)handle->Size;
		fclose(handle->Handle);
		FreeMem(handle);
		return result;
	}
	else
#endif
	{
#if defined(DBG)
		FILE* file_ptr;
		if (path[0] == L'r' && path[1] == L'e' && path[2] == L's' && path[3] == L'/')
		{
			Char path2[KUIN_MAX_PATH * 2 + 1];
			wcscpy(path2, EnvVars.ResRoot);
			wcscat(path2, path);
			file_ptr = _wfopen(path2, L"rb");
		}
		else
			file_ptr = _wfopen(path, L"rb");
#else
		FILE* file_ptr = _wfopen(path, L"rb");
#endif
		if (file_ptr == NULL)
			return NULL;
		_fseeki64(file_ptr, 0, SEEK_END);
		{
			S64 size2 = _ftelli64(file_ptr);
			*size = (size_t)size2;
		}
		_fseeki64(file_ptr, 0, SEEK_SET);
		{
			void* result = AllocMem(*size);
			fread(result, 1, *size, file_ptr);
			fclose(file_ptr);
			return result;
		}
	}
}

void* OpenFileStream(const Char* path)
{
#if !defined(DBG)
	if (path[0] == L'r' && path[1] == L'e' && path[2] == L's' && path[3] == L'/')
	{
		SFile* handle = OpenPackFile(path + 4);
		if (handle == NULL)
			return NULL;
		return handle;
	}
	else
#endif
	{
#if defined(DBG)
		FILE* file_ptr;
		if (path[0] == L'r' && path[1] == L'e' && path[2] == L's' && path[3] == L'/')
		{
			Char path2[KUIN_MAX_PATH * 2 + 1];
			wcscpy(path2, EnvVars.ResRoot);
			wcscat(path2, path);
			file_ptr = _wfopen(path2, L"rb");
	}
		else
			file_ptr = _wfopen(path, L"rb");
#else
		FILE* file_ptr = _wfopen(path, L"rb");
#endif
		if (file_ptr == NULL)
			return NULL;
		{
			SFile* result = (SFile*)AllocMem(sizeof(SFile));
			result->Pack = False;
			result->Handle = file_ptr;
			result->Pos = 0;
			result->Size = 0;
			result->Cur = 0;
			result->Key = 0;
			return result;
		}
	}
}

void CloseFileStream(void* handle)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL && handle2->Handle != NULL);
	fclose(handle2->Handle);
	FreeMem(handle2);
}

size_t ReadFileStream(void* handle, size_t size, void* buf)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
	{
		S64 size2 = (S64)size;
		S64 rest = handle2->Pos + handle2->Size - handle2->Cur;
		if (size2 > rest)
			size2 = rest;
		ASSERT(size2 >= 0);
		size_t size3 = fread(buf, 1, (size_t)size2, handle2->Handle);
		S64 i;
		U8* ptr = (U8*)buf;
		U64 pos = (U64)(handle2->Cur - sizeof(U64) * 3);
		for (i = 0; i < (S64)size3; i++)
		{
			ptr[i] = GetKey(handle2->Key, ptr[i], pos);
			pos++;
		}
		handle2->Cur += (S64)size3;
		return size3;
	}
	else
		return fread(buf, 1, size, handle2->Handle);
}

Bool SeekFileStream(void* handle, S64 offset, S64 origin)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
	{
		S64 pos = 0;
		switch (origin)
		{
			case SEEK_SET:
				pos = handle2->Pos;
				break;
			case SEEK_CUR:
				pos = handle2->Cur;
				break;
			case SEEK_END:
				pos = handle2->Pos + handle2->Size;
				break;
			default:
				ASSERT(False);
				break;
		}
		pos += offset;
		if (pos < handle2->Pos || handle2->Pos + handle2->Size < pos)
			return False;
		handle2->Cur = pos;
		int result = _fseeki64(handle2->Handle, pos, SEEK_SET);
		ASSERT(result == 0);
		UNUSED(result);
		return True;
	}
	else
		return _fseeki64(handle2->Handle, offset, (int)origin) == 0;
}

S64 TellFileStream(void* handle)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
		return handle2->Cur - handle2->Pos;
	else
		return _ftelli64(handle2->Handle);
}

Bool StrCmpIgnoreCase(const Char* a, const Char* b)
{
	while (*a != L'\0')
	{
		Char a2 = L'A' <= *a && *a <= L'Z' ? (*a - L'A' + L'a') : *a;
		Char b2 = L'A' <= *b && *b <= L'Z' ? (*b - L'A' + L'a') : *b;
		if (a2 != b2)
			return False;
		a++;
		b++;
	}
	return *b == L'\0';
}

U8 SwapEndianU8(U8 n)
{
	return n;
}

U16 SwapEndianU16(U16 n)
{
	return ((n & 0x00ff) << 8) | ((n & 0xFF00) >> 8);
}

U32 SwapEndianU32(U32 n)
{
	n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
	n = ((n & 0x0000ffff) << 16) | ((n & 0xffff0000) >> 16);
	return n;
}

U64 SwapEndianU64(U64 n)
{
	n = ((n & 0x00ff00ff00ff00ff) << 8) | ((n & 0xff00ff00ff00ff00) >> 8);
	n = ((n & 0x0000ffff0000ffff) << 16) | ((n & 0xffff0000ffff0000) >> 16);
	n = ((n & 0x00000000ffffffff) << 32) | ((n & 0xffffffff00000000) >> 32);
	return n;
}

Bool IsPowerOf2(U64 n)
{
	return (n & (n - 1)) == 0;
}

U32 MakeSeed(U32 key)
{
	return (U32)(time(NULL)) ^ (U32)timeGetTime() ^ key;
}

U32 XorShift(U32* seed)
{
	U32 x = *seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*seed = x;
	return x;
}

U64 XorShift64(U32* seed)
{
	U32 a = XorShift(seed);
	U32 b = XorShift(seed);
	return ((U64)a << 32) | (U64)b;
}

S64 XorShiftInt(U32* seed, S64 min, S64 max)
{
	U64 n = (U64)(max - min + 1);
	U64 m = 0 - ((0 - n) % n);
	U64 r;
	if (m == 0)
		r = XorShift64(seed);
	else
	{
		do
		{
			r = XorShift64(seed);
		} while (m <= r);
	}
	return (S64)(r % n) + min;
}

double XorShiftFloat(U32* seed, double min, double max)
{
	return (double)(XorShift64(seed)) / 18446744073709551616.0 * (max - min) + min;
}

char* Utf16ToUtf8(const U8* str)
{
	if (str == NULL)
		return NULL;
	const Char* str2 = (const Char*)(str + 0x10);
	int len_str = (int)((S64*)str)[1];
	size_t len = (size_t)(WideCharToMultiByte(CP_UTF8, 0, str2, len_str, NULL, 0, NULL, NULL));
	char* buf = (char*)(AllocMem(len + 1));
	if (WideCharToMultiByte(CP_UTF8, 0, str2, len_str, buf, (int)len, NULL, NULL) != (int)len)
	{
		FreeMem(buf);
		return NULL;
	}
	buf[len] = L'\0';
	return buf;
}

U8* Utf8ToUtf16(const char* str)
{
	if (str == NULL)
		return NULL;
	int len_str = (int)strlen(str);
	size_t len = (size_t)MultiByteToWideChar(CP_UTF8, 0, str, len_str, NULL, 0);
	U8* buf = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	((S64*)buf)[0] = DefaultRefCntFunc;
	((S64*)buf)[1] = (S64)len;
	if (MultiByteToWideChar(CP_UTF8, 0, str, len_str, (Char*)(buf + 0x10), (int)len) != (int)len)
	{
		FreeMem(buf);
		return NULL;
	}
	((Char*)(buf + 0x10))[len] = L'\0';
	return buf;
}

Bool IsResUsed(EUseResFlagsKind kind)
{
	S64 idx = (S64)kind;
	ASSERT(1 <= idx && (idx - 1) / 8 < USE_RES_FLAGS_LEN);
	return (EnvVars.UseResFlags[(idx - 1) / 8] & (1 << ((idx - 1) % 8))) != 0;
}

#if !defined(DBG)
static SFile* OpenPackFile(const Char* path)
{
	FILE* file_ptr;
	{
		Char path2[KUIN_MAX_PATH + 1];
		wcscpy(path2, EnvVars.ResRoot);
		wcscat(path2, L"res.knd");
		file_ptr = _wfopen(path2, L"rb");
		if (file_ptr == NULL)
			return NULL;
	}
	SFile* handle = (SFile*)AllocMem(sizeof(SFile));
	handle->Pack = True;
	handle->Handle = file_ptr;
	handle->Pos = -1;
	handle->Size = 0;
	handle->Cur = 0;
	U64 key;
	U64 len;
	{
		fread(&key, sizeof(U64), 1, file_ptr);
		key ^= (U64)EnvVars.AppCode * 0x9271ac8394027acb + 0x35718394ca72849e;
		handle->Key = key;
	}
	{
		U64 signature;
		fread(&signature, sizeof(U64), 1, file_ptr);
		signature ^= key;
		if (signature != 0x83261772fa0c01a7)
		{
			FreeMem(handle);
			fclose(file_ptr);
			return NULL;
		}
	}
	{
		fread(&len, sizeof(U64), 1, file_ptr);
		len ^= 0x9c4cab83ce74a67e ^ key;
		if (len > 65535)
		{
			FreeMem(handle);
			fclose(file_ptr);
			return NULL;
		}
	}
	{
		U64 pos = 0;
		U64 i;
		U64 j;
		Char path2[KUIN_MAX_PATH + 1];
		for (i = 0; i < len; i++)
		{
			U64 size;
			U64 path_len;
			{
				fread(&size, sizeof(U64), 1, file_ptr);
				U8* ptr = (U8*)&size;
				for (j = 0; j < 8; j++)
				{
					ptr[j] = GetKey(key, ptr[j], pos);
					pos++;
				}
			}
			{
				fread(&path_len, sizeof(U64), 1, file_ptr);
				U8* ptr = (U8*)&path_len;
				for (j = 0; j < 8; j++)
				{
					ptr[j] = GetKey(key, ptr[j], pos);
					pos++;
				}
				if (path_len > KUIN_MAX_PATH)
				{
					FreeMem(handle);
					fclose(file_ptr);
					return NULL;
				}
			}
			{
				fread(path2, sizeof(Char), (size_t)path_len, file_ptr);
				U8* ptr = (U8*)path2;
				for (j = 0; j < path_len * 2; j++)
				{
					ptr[j] = GetKey(key, ptr[j], pos);
					pos++;
				}
				path2[path_len] = L'\0';
				if (wcscmp(path2, path) == 0)
				{
					handle->Pos = (S64)((U64)sizeof(U64) * 3 + pos);
					handle->Size = (S64)size;
					handle->Cur = handle->Pos;
					return handle;
				}
				_fseeki64(file_ptr, (S64)size, SEEK_CUR);
				pos += size;
			}
		}
	}
	FreeMem(handle);
	fclose(file_ptr);
	return NULL;
}
#endif

static U8 GetKey(U64 key, U8 data, U64 pos)
{
	U64 rnd = ((pos ^ key) * 0x351cd819923acae7) >> 32;
	return (U8)(data ^ rnd);
}
