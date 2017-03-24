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
	void* Handle;
} SFile;

void* Heap;
S64* HeapCnt;
S64 AppCode;
const Char* AppName;
HINSTANCE Instance;

void* AllocMem(size_t size)
{
	void* result = HeapAlloc(Heap, HEAP_GENERATE_EXCEPTIONS, (SIZE_T)size);
#if defined(_DEBUG)
	memset(result, 0xcd, size);
	(*HeapCnt)++;
#endif
	return result;
}

void FreeMem(void* ptr)
{
	HeapFree(Heap, 0, ptr);
#if defined(_DEBUG)
	(*HeapCnt)--;
#endif
}

void ThrowImpl(U32 code, const Char* msg)
{
	void* arg0 = NULL;
	if (msg != NULL)
	{
		size_t len = wcslen(msg);
		arg0 = AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)arg0)[0] = 1; // The caught class refers to this.
		((S64*)arg0)[1] = len;
		wcscpy((Char*)((S64*)arg0 + 2), msg);
	}
	{
		ULONG_PTR args[1];
		args[0] = (ULONG_PTR)arg0;
		RaiseException((DWORD)code, 0, 1, args);
	}
}

void* LoadFileAll(const Char* path, size_t* size)
{
	if (path[0] == L':')
	{
		path++;
		// TODO:
		return NULL;
	}
	else
	{
		FILE* file_ptr = _wfopen(path, L"rb");
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
	if (path[0] == L':')
	{
		path++;
		{
			SFile* result = (SFile*)AllocMem(sizeof(SFile));
			result->Pack = True;
			// TODO:
			return result;
		}
	}
	else
	{
		FILE* file_ptr = _wfopen(path, L"rb");
		if (file_ptr == NULL)
			return NULL;
		{
			SFile* result = (SFile*)AllocMem(sizeof(SFile));
			result->Pack = False;
			result->Handle = file_ptr;
			return result;
		}
	}
}

void CloseFileStream(void* handle)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
	{
		// TODO:
	}
	else
		fclose((FILE*)handle2->Handle);
	FreeMem(handle2);
}

size_t ReadFileStream(void* handle, size_t size, void* buf)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
	{
		// TODO:
		return 0;
	}
	else
		return fread(buf, 1, size, (FILE*)handle2->Handle);
}

Bool SeekFileStream(void* handle, S64 offset, S64 origin)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
	{
		// TODO:
		return False;
	}
	else
		return fseek((FILE*)handle2->Handle, (long)offset, (int)origin) == 0;
}

S64 TellFileStream(void* handle)
{
	SFile* handle2 = (SFile*)handle;
	ASSERT(handle2 != NULL);
	if (handle2->Pack)
	{
		// TODO:
		return 0;
	}
	else
		return (S64)ftell((FILE*)handle2->Handle);
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
