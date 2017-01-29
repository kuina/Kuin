// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "draw.h"
#include "snd.h"
#include "input.h"

static const Char* WndClassName = L"KuinWndClass";

typedef struct SWnd
{
	SClass Class;
	HWND Handle;
	void* WndBuf;
} Wnd;

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

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

	{
		HICON icon = LoadIcon(Instance, reinterpret_cast<LPCWSTR>(static_cast<S64>(0x65))); // 0x65 is the resource ID of the application icon.
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = CS_HREDRAW | CS_VREDRAW;
		wnd_class.lpfnWndProc = WndProc;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = icon;
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = WndClassName;
		wnd_class.hIconSm = icon;
		RegisterClassEx(&wnd_class);
	}

	Draw::Init();
	Snd::Init();

	// TODO:
}

EXPORT_CPP void _fin()
{
	Snd::Fin();
	Draw::Fin();

	// TODO:
}

EXPORT_CPP Bool _act()
{
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				return False;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// TODO:

	Sleep(1);

	return True;
}

EXPORT_CPP SClass* _makeWnd(SClass* me_, S64 style, S64 width, S64 height)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Handle = CreateWindowEx(0, WndClassName, AppName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, Instance, NULL);
	ASSERT(me2->Handle != NULL);
	ShowWindow(me2->Handle, SW_SHOWNORMAL);
	me2->WndBuf = Draw::MakeWndBuf(static_cast<int>(width), static_cast<int>(height), me2->Handle);
	return me_;
}

EXPORT_CPP void _wndDtor(SClass* me_)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	Draw::FinWndBuf(me2->WndBuf);
}

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
		case WM_CLOSE:
			SendMessage(wnd, WM_DESTROY, 0, 0);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		// TODO:
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
}
