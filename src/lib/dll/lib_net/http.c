#include "http.h"

#include <WinInet.h>

#define HTTP_DATA_SIZE (1024)

#pragma comment(lib, "Wininet.lib")

typedef struct SHttpList
{
	struct SHttpList* Next;
	size_t Size;
	U8 Data[HTTP_DATA_SIZE];
} SHttpList;

typedef struct SHttp
{
	SClass Class;
	HINTERNET HandleOpen;
	HINTERNET HandleConnect;
	HINTERNET HandleRequest;
	Bool ThreadExit;
	Bool Success;
	size_t TotalSize;
	SHttpList* DataTop;
	SHttpList* DataBottom;
	CRITICAL_SECTION* Mutex;
	HANDLE ThreadHandle;
} SHttp;

static DWORD WINAPI HttpThread(LPVOID param);

EXPORT void _httpFin(SClass* me_)
{
	SHttp* me2 = (SHttp*)me_;
	if (me2->Mutex == NULL)
		return;
	EnterCriticalSection(me2->Mutex);
	me2->ThreadExit = True;
	LeaveCriticalSection(me2->Mutex);
	WaitForSingleObject(me2->ThreadHandle, INFINITE);
	CloseHandle(me2->ThreadHandle);
	me2->ThreadHandle = NULL;
	DeleteCriticalSection(me2->Mutex);
	FreeMem(me2->Mutex);
	me2->Mutex = NULL;

	if (me2->HandleRequest != NULL)
		InternetCloseHandle(me2->HandleRequest);
	if (me2->HandleConnect != NULL)
		InternetCloseHandle(me2->HandleConnect);
	if (me2->HandleOpen != NULL)
		InternetCloseHandle(me2->HandleOpen);

	while (me2->DataTop != NULL)
	{
		SHttpList* ptr = me2->DataTop;
		me2->DataTop = me2->DataTop->Next;
		FreeMem(ptr);
	}
	me2->Success = False;
	me2->TotalSize = 0;
	me2->DataTop = NULL;
	me2->DataBottom = NULL;
}

EXPORT void* _httpGet(SClass* me_)
{
	SHttp* me2 = (SHttp*)me_;
	if (!me2->Success)
		return NULL;

	EnterCriticalSection(me2->Mutex);
	me2->Success = False;
	U8* result;
	if (me2->TotalSize == 0)
	{
		result = (U8*)AllocMem(0x10);
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = 0;
	}
	else
	{
		char* str = (char*)AllocMem((size_t)me2->TotalSize + 1);
		char* ptr = str;
		SHttpList* ptr2 = me2->DataTop;
		while (ptr2 != NULL)
		{
			memcpy(ptr, ptr2->Data, ptr2->Size);
			ptr += ptr2->Size;
			ptr2 = ptr2->Next;
		}
		ASSERT(ptr == str + me2->TotalSize);
		str[me2->TotalSize] = L'\0';
		size_t len = (size_t)MultiByteToWideChar(CP_UTF8, 0, str, (int)me2->TotalSize, NULL, 0);
		result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		if (MultiByteToWideChar(CP_UTF8, 0, str, (int)me2->TotalSize, (Char*)(result + 0x10), (int)len) != (int)len)
		{
			FreeMem(result);
			result = (U8*)AllocMem(0x10);
			((S64*)result)[0] = DefaultRefCntFunc;
			((S64*)result)[1] = 0;
		}
		else
			((Char*)(result + 0x10))[len] = L'\0';
		FreeMem(str);
	}
	LeaveCriticalSection(me2->Mutex);
	return result;
}

EXPORT SClass* _makeHttp(SClass* me_, const U8* url, Bool post, const U8* agent)
{
	THROWDBG(url == NULL, EXCPT_ACCESS_VIOLATION);
	SHttp* me2 = (SHttp*)me_;
	URL_COMPONENTS url_components;
	Char host_name[2049];
	Char url_path[2049];
	memset(&url_components, 0, sizeof(url_components));
	url_components.dwStructSize = sizeof(url_components);
	url_components.lpszHostName = host_name;
	url_components.lpszUrlPath = url_path;
	url_components.dwHostNameLength = 2049;
	url_components.dwUrlPathLength = 2049;
	if (!InternetCrackUrl((const Char*)(url + 0x10), (DWORD)((S64*)(url + 0x08))[1], 0, &url_components))
		return NULL;
	DWORD flags;
	if (url_components.nScheme == INTERNET_SCHEME_HTTP)
		flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT;
	else if (url_components.nScheme == INTERNET_SCHEME_HTTPS)
		flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	else
		return NULL;
	// TODO: Parameters
	if (post)
	{
	}
	Bool success = False;
	for (; ; )
	{
		me2->HandleOpen = InternetOpen(agent == NULL ? L"" : (const Char*)(agent + 0x10), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (me2->HandleOpen == NULL)
			break;
		me2->HandleConnect = InternetConnect(me2->HandleOpen, url_components.lpszHostName, url_components.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (me2->HandleConnect == NULL)
			break;
		me2->HandleRequest = HttpOpenRequest(me2->HandleConnect, post ? L"POST" : L"GET", url_components.lpszUrlPath, NULL, NULL, NULL, flags, 0);
		if (me2->HandleRequest == NULL)
			break;
		const Char* header = post ? L"Content-Type: application/x-www-form-urlencoded" : L"";
		if (!HttpSendRequest(me2->HandleRequest, header, (DWORD)wcslen(header), NULL, 0)) // TODO: Parameters
			break;
		{
			DWORD status_code;
			DWORD len = (DWORD)sizeof(DWORD);
			if (!HttpQueryInfo(me2->HandleRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &len, 0))
				break;
			if (status_code < 200 || 299 < status_code)
				break;
		}
		success = True;
		break;
	}
	if (!success)
	{
		if (me2->HandleRequest != NULL)
			InternetCloseHandle(me2->HandleRequest);
		if (me2->HandleConnect != NULL)
			InternetCloseHandle(me2->HandleConnect);
		if (me2->HandleOpen != NULL)
			InternetCloseHandle(me2->HandleOpen);
		return NULL;
	}
	me2->Mutex = (CRITICAL_SECTION*)AllocMem(sizeof(CRITICAL_SECTION));

	me2->ThreadExit = False;
	me2->Success = False;
	me2->TotalSize = 0;
	SHttpList* http_list = (SHttpList*)AllocMem(sizeof(SHttpList));
	http_list->Next = NULL;
	http_list->Size = 0;
	me2->DataTop = http_list;
	me2->DataBottom = http_list;
	InitializeCriticalSection(me2->Mutex);
	me2->ThreadHandle = CreateThread(NULL, 0, HttpThread, me2, 0, NULL);
	return me_;
}

static DWORD WINAPI HttpThread(LPVOID param)
{
	SHttp* param2 = (SHttp*)param;
	for (; ; )
	{
		Sleep(1);

		EnterCriticalSection(param2->Mutex);
		if (param2->ThreadExit)
		{
			param2->Success = False;
			param2->TotalSize = 0;
			LeaveCriticalSection(param2->Mutex);
			break;
		}

		DWORD read_size = 0;
		if (!InternetReadFile(param2->HandleRequest, param2->DataBottom->Data, HTTP_DATA_SIZE, &read_size))
		{
			param2->ThreadExit = True;
			param2->Success = False;
			param2->TotalSize = 0;
			LeaveCriticalSection(param2->Mutex);
			break;
		}

		if (read_size == 0)
		{
			param2->ThreadExit = True;
			param2->Success = True;
			LeaveCriticalSection(param2->Mutex);
			break;
		}
		else
		{
			param2->DataBottom->Size = (size_t)read_size;
			param2->TotalSize += (size_t)read_size;
			SHttpList* http_list = (SHttpList*)AllocMem(sizeof(SHttpList));
			http_list->Next = NULL;
			http_list->Size = 0;
			param2->DataBottom->Next = http_list;
			param2->DataBottom = http_list;
		}

		LeaveCriticalSection(param2->Mutex);
	}
	return TRUE;
}
