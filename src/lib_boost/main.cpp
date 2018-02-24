// LibBoost.dll
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

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const Char* use_res_flags)
{
	UNUSED(use_res_flags);
	if (Heap != NULL)
		return;
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	Instance = static_cast<HINSTANCE>(GetModuleHandle(NULL));
}
