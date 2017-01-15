#include "common.h"

void* Heap;
S64* HeapCnt;
S64 AppCode;
const Char* AppName;
HINSTANCE Instance;

static int DefaultCmp(const SClass* me_, const SClass* t);
static SClass* DefaultCopy(const SClass* me_);
static U8* DefaultToBin(const SClass* me_);
static S64 DefaultFromBin(SClass* me_, U8* bin, S64 idx);

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

void InitClass(SClass* class_, void(*ctor)(SClass* me_), void(*dtor)(SClass* me_))
{
	if (ctor != NULL)
		class_->Ctor = ctor;
	if (dtor != NULL)
		class_->Dtor = dtor;
	class_->Cmp = DefaultCmp;
	class_->Copy = DefaultCopy;
	class_->ToBin = DefaultToBin;
	class_->FromBin = DefaultFromBin;
}

void* LoadFileAll(const Char* path, size_t* size, Bool occurExcpt)
{
	if (path[0] == L':')
	{
		path++;
		// TODO:
	}
	else
	{
		FILE* file_ptr = _wfopen(path, L"rb");
		if (file_ptr == NULL)
		{
			if (occurExcpt)
				THROW(0x1000, L"");
			return NULL;
		}
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

static int DefaultCmp(const SClass* me_, const SClass* t)
{
	UNUSED(me_);
	UNUSED(t);
	THROW(0x1000, L"");
	return 0;
}

static SClass* DefaultCopy(const SClass* me_)
{
	UNUSED(me_);
	THROW(0x1000, L"");
	return NULL;
}

static U8* DefaultToBin(const SClass* me_)
{
	UNUSED(me_);
	THROW(0x1000, L"");
	return NULL;
}

static S64 DefaultFromBin(SClass* me_, U8* bin, S64 idx)
{
	UNUSED(me_);
	UNUSED(bin);
	THROW(0x1000, L"");
	return idx;
}
