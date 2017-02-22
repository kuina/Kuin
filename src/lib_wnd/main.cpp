// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "draw.h"
#include "snd.h"
#include "input.h"

typedef struct SWnd
{
	SClass Class;
	void* Mfc;
	void* WndBuf;
} Wnd;

static HMODULE MfcHandle;
static void(*MfcInit)();
static void(*MfcFin)();
static Bool(*MfcAct)();
static void*(*MfcMakeWnd)(S64 width, S64 height);
static HWND(*MfcGetHwnd)(void* ptr);

static Bool ExitAct;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : reinterpret_cast<const Char*>(app_name + 0x10);
	Instance = static_cast<HINSTANCE>(GetModuleHandle(NULL));

	MfcHandle = LoadLibrary(L"d0003.knd");
	if (MfcHandle == NULL)
		THROW(0x1000, L"");
	MfcInit = reinterpret_cast<void(*)()>(GetProcAddress(MfcHandle, "_init"));
	if (MfcInit == NULL)
		THROW(0x1000, L"");
	MfcFin = reinterpret_cast<void(*)()>(GetProcAddress(MfcHandle, "_fin"));
	if (MfcFin == NULL)
		THROW(0x1000, L"");
	MfcAct = reinterpret_cast<Bool(*)()>(GetProcAddress(MfcHandle, "_act"));
	if (MfcAct == NULL)
		THROW(0x1000, L"");
	MfcMakeWnd = reinterpret_cast<void*(*)(S64, S64)>(GetProcAddress(MfcHandle, "_makeWnd"));
	if (MfcMakeWnd == NULL)
		THROW(0x1000, L"");
	MfcGetHwnd = reinterpret_cast<HWND(*)(void*)>(GetProcAddress(MfcHandle, "_getHwnd"));
	if (MfcGetHwnd == NULL)
		THROW(0x1000, L"");

	ExitAct = False;

	MfcInit();
	Draw::Init();
	Snd::Init();
	Input::Init();

	// TODO:
}

EXPORT_CPP void _fin()
{
	Input::Fin();
	Snd::Fin();
	Draw::Fin();
	MfcFin();

	if (MfcHandle != NULL)
		FreeLibrary(MfcHandle);

	// TODO:
}

EXPORT_CPP Bool _act()
{
	if (ExitAct)
		return False;

	if (!MfcAct())
	{
		ExitAct = True;
		return False;
	}

	Input::Update();

	Sleep(1);

	return True;
}

EXPORT_CPP SClass* _makeWnd(SClass* me_, S64 width, S64 height, S64 style, Bool drawBuf)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Mfc = MfcMakeWnd(width, height);
	if (drawBuf)
		me2->WndBuf = Draw::MakeWndBuf(static_cast<int>(width), static_cast<int>(height), MfcGetHwnd(me2->Mfc));
	return me_;
}

EXPORT_CPP SClass* _makeTextEditor(SClass* me_, SClass* parent, S64 left, S64 top, S64 width, S64 height)
{
	// TODO:
	return NULL;
}

EXPORT_CPP void _wndDtor(SClass* me_)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	if (me2->WndBuf != NULL)
		Draw::FinWndBuf(me2->WndBuf);
}
