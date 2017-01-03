#include "common.h"

void* Heap;
S64* HeapCnt;

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

void Throw(U32 code, const Char* msg)
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
