#include "main.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

static Clib_mfcApp theApp;
static Bool ExitAct;

BEGIN_MESSAGE_MAP(CKuinBackground, CFrameWnd)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CKuinWnd, CFrameWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWnd::OnDestroy()
{
	ExitAct = True;
	CFrameWnd::OnDestroy();
}

BOOL Clib_mfcApp::InitInstance()
{
	CWinApp::InitInstance();
	return TRUE;
}

EXPORT_CPP void _init()
{
	ExitAct = False;

	CKuinBackground* wnd = new CKuinBackground();
	wnd->Create(NULL, L"");
	wnd->ShowWindow(SW_HIDE);
	wnd->UpdateWindow();
	theApp.m_pMainWnd = wnd;
}

EXPORT_CPP void _fin()
{
	// Do nothing.
}

EXPORT_CPP void _dummy()
{
	// Do nothing.
}

EXPORT_CPP Bool _act()
{
	if (ExitAct)
		return False;

	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!AfxGetApp()->PumpMessage())
		{
			ExitAct = True;
			return False;
		}
	}
	LONG cnt = 0;
	while (AfxGetApp()->OnIdle(cnt))
		cnt++;
	return True;
}

EXPORT_CPP void* _makeWnd()
{
	CKuinWnd* result = new CKuinWnd();
	result->Create(NULL, L"", WS_OVERLAPPEDWINDOW);
	HICON icon = LoadIcon(static_cast<HINSTANCE>(GetModuleHandle(NULL)), reinterpret_cast<LPCWSTR>(static_cast<S64>(0x65))); // 0x65 is the resource ID of the application icon.
	result->SetIcon(icon, FALSE);
	result->ShowWindow(SW_SHOWNORMAL);
	result->UpdateWindow();
	return result;
}

EXPORT_CPP HWND _getHwnd(void* ptr)
{
	CKuinWnd* ptr2 = static_cast<CKuinWnd*>(ptr);
	return ptr2->m_hWnd;
}
