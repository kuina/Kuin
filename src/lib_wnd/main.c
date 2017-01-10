// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

static const Char* WndClassName = L"KuinWndClass";

HWND Wnd;

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : (const Char*)(app_name + 0x10);
	Instance = (HINSTANCE)GetModuleHandle(NULL);

	{
		HICON icon = LoadIcon(Instance, (LPCWSTR)(S64)0x65); // 0x65 is the resource ID of the application icon.
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = CS_HREDRAW | CS_VREDRAW;
		wnd_class.lpfnWndProc = WndProc;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = icon;
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = WndClassName;
		wnd_class.hIconSm = icon;
		RegisterClassEx(&wnd_class);
	}

	{
#if defined(DBG)
		const Char* title;
		Char title2[1025];
		if (wcslen(AppName) + 6 > 1024)
			THROW(0x1000, L"");
		wcscpy(title2, AppName);
		wcscat(title2, L" [dbg]");
		title = title2;
#else
		const Char* title = AppName;
#endif
		Wnd = CreateWindowEx(0, WndClassName, title, WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, Instance, NULL);
		ASSERT(Wnd != NULL);
		ShowWindow(Wnd, SW_SHOWNORMAL);
	}

	// TODO:
}

EXPORT void _fin(void)
{
	// TODO:
}

EXPORT Bool _act(void)
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

	return True;
}

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	if (wnd == Wnd)
	{
		switch (msg)
		{
			case WM_CLOSE:
				SendMessage(Wnd, WM_DESTROY, 0, 0);
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			// TODO:
		}
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
}
