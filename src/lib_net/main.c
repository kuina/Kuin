// LibNet.dll
//
// (C)Kuina-chan
//

#include "main.h"

#pragma comment(lib, "ws2_32.lib")

#define DATA_SIZE (1024 * 1024)

typedef struct STcp
{
	SClass Class;
	SOCKET Socket;
	HANDLE ThreadHandle;
	CRITICAL_SECTION* Mutex;
	U8* Data;
} STcp;

static WSADATA* WsaData = NULL;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	if (Heap != NULL)
		return;
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : (Char*)(app_name + 0x10);
	Instance = (HINSTANCE)GetModuleHandle(NULL);

	WsaData = (WSADATA*)AllocMem(sizeof(WSADATA));
	if (WSAStartup(MAKEWORD(2, 0), WsaData) != 0)
	{
		FreeMem(WsaData);
		WsaData = NULL;
	}
}

EXPORT void _fin(void)
{
	if (WsaData == NULL)
	{
		WSACleanup();
		FreeMem(WsaData);
		WsaData = NULL;
	}
}
