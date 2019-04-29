// LibCui.dll
//
// (C)Kuina-chan
//

#include "main.h"
#include "cui.h"

#include <fcntl.h>
#include <io.h>

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (!InitEnvVars(heap, heap_cnt, app_code, use_res_flags))
		return;

	wprintf(L""); // Open 'stdout'
	_setmode(_fileno(stdin), _O_U8TEXT); // Set the input format to UTF-8.
	_setmode(_fileno(stdout), _O_U8TEXT); // Set the output format to UTF-8.

	InitCui();
}

EXPORT void _fin(void)
{
	FinCui();
}
