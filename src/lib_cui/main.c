// LibCui.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include <fcntl.h>
#include <io.h>

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt)
{
	Heap = heap;
	HeapCnt = heap_cnt;

	wprintf(L""); // Open 'stdout'
	_setmode(_fileno(stdout), _O_U16TEXT); // Set the output format to UTF-16.
}

EXPORT void _fin(void)
{
}
