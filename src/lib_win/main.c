// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt)
{
	// TODO:
}

EXPORT void _fin(void)
{
	// TODO:
}
