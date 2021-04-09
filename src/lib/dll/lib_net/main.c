#include "main.h"

#include <winsock2.h>

static WSADATA* WsaData = NULL;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

	WsaData = (WSADATA*)AllocMem(sizeof(WSADATA));
	if (WSAStartup(MAKEWORD(2, 2), WsaData) != 0)
	{
		FreeMem(WsaData);
		WsaData = NULL;
	}
}

EXPORT void _fin(void)
{
	if (WsaData != NULL)
	{
		WSACleanup();
		FreeMem(WsaData);
		WsaData = NULL;
	}
}
