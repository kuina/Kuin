#include "main.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

enum EWndKind
{
	WndKind_WndNormal = 0x00,
	WndKind_WndFixed,
	WndKind_WndAspect,
	WndKind_WndMdi,
	WndKind_WndMdiChild,
	WndKind_WndDock,
	WndKind_WndDockChild,
	WndKind_Draw = 0x80,
	WndKind_Btn,
	WndKind_Chk,
	WndKind_Radio,
	WndKind_Edit,
	WndKind_EditMulti,
	WndKind_List,
	WndKind_Combo,
	WndKind_ComboList,
	WndKind_Label,
	WndKind_Group,
};

static Clib_mfcApp theApp;
static int WndNum;

BEGIN_MESSAGE_MAP(CKuinWndNormal, CWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndNormal::OnDestroy()
{
	WndNum--;
	CWnd::OnDestroy();
}

BEGIN_MESSAGE_MAP(CKuinWndFixed, CWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndFixed::OnDestroy()
{
	WndNum--;
	CWnd::OnDestroy();
}

BEGIN_MESSAGE_MAP(CKuinWndAspect, CWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndAspect::OnDestroy()
{
	WndNum--;
	CWnd::OnDestroy();
}

BEGIN_MESSAGE_MAP(CKuinWndMdi, CMDIFrameWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndMdi::OnDestroy()
{
	WndNum--;
	CMDIFrameWnd::OnDestroy();
}

BEGIN_MESSAGE_MAP(CKuinWndMdiChild, CMDIChildWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndMdiChild::OnDestroy()
{
	WndNum--;
	CMDIChildWnd::OnDestroy();
}

BEGIN_MESSAGE_MAP(CKuinWndDock, CFrameWndEx)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndDock::OnDestroy()
{
	WndNum--;
	CFrameWndEx::OnDestroy();
}

BEGIN_MESSAGE_MAP(CKuinWndDockChild, CDockablePane)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CKuinWndDockChild::OnDestroy()
{
	WndNum--;
	CDockablePane::OnDestroy();
}

BOOL Clib_mfcApp::InitInstance()
{
	CWinApp::InitInstance();
	return TRUE;
}

EXPORT_CPP void _init()
{
	WndNum = 0;

	CKuinBackground* wnd = new CKuinBackground();
	wnd->Create(NULL, L"");
	wnd->ShowWindow(SW_HIDE);
	wnd->UpdateWindow();
	theApp.m_pMainWnd = wnd;

	{
		WNDCLASS wnd_class;
		wnd_class.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wnd_class.lpfnWndProc = AfxWndProc;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = AfxGetInstanceHandle();
		wnd_class.hIcon = LoadIcon(static_cast<HINSTANCE>(GetModuleHandle(NULL)), reinterpret_cast<LPCWSTR>(static_cast<S64>(0x65))); // 0x65 is the resource ID of the application icon.
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = L"KuinWndClass";
		AfxRegisterClass(&wnd_class);
	}

	CDockingManager::SetDockingMode(DT_IMMEDIATE);
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
	if (WndNum == 0)
		return False;

	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!AfxGetApp()->PumpMessage())
			return False;
	}
	LONG cnt = 0;
	while (AfxGetApp()->OnIdle(cnt))
		cnt++;
	return True;
}

EXPORT_CPP void* _makeWnd(S64 kind, void* parent, S64 x, S64 y, S64 width, S64 height, S64 anchor_num, const S64* anchor, const Char* text)
{
	RECT rect;
	{
		rect.left = static_cast<LONG>(x);
		rect.top = static_cast<LONG>(y);
		rect.right = static_cast<LONG>(width);
		rect.bottom = static_cast<LONG>(height);
	}
	const UINT default_id = 0xffff;
	switch (static_cast<EWndKind>(kind))
	{
		case WndKind_WndNormal:
			{
				WndNum++;
				CKuinWndNormal* result = new CKuinWndNormal();
				HWND parent2 = parent == NULL ? theApp.m_pMainWnd->m_hWnd : static_cast<CWnd*>(parent)->m_hWnd;
				result->CreateEx(0, L"KuinWndClass", text, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width == -1 ? CW_USEDEFAULT : static_cast<int>(width), height == -1 ? CW_USEDEFAULT : static_cast<int>(height), parent2, NULL);
				result->ShowWindow(SW_SHOWNORMAL);
				result->UpdateWindow();
				return result;
			}
		case WndKind_WndFixed:
			{
				WndNum++;
				CKuinWndFixed* result = new CKuinWndFixed();
				HWND parent2 = parent == NULL ? theApp.m_pMainWnd->m_hWnd : static_cast<CWnd*>(parent)->m_hWnd;
				result->CreateEx(0, L"KuinWndClass", text, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, width == -1 ? CW_USEDEFAULT : static_cast<int>(width), height == -1 ? CW_USEDEFAULT : static_cast<int>(height), parent2, NULL);
				result->ShowWindow(SW_SHOWNORMAL);
				result->UpdateWindow();
				return result;
			}
		case WndKind_WndAspect:
		{
			WndNum++;
			CKuinWndAspect* result = new CKuinWndAspect();
			HWND parent2 = parent == NULL ? theApp.m_pMainWnd->m_hWnd : static_cast<CWnd*>(parent)->m_hWnd;
			result->CreateEx(0, L"KuinWndClass", text, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width == -1 ? CW_USEDEFAULT : static_cast<int>(width), height == -1 ? CW_USEDEFAULT : static_cast<int>(height), parent2, NULL);
			result->ShowWindow(SW_SHOWNORMAL);
			result->UpdateWindow();
			return result;
		}
		case WndKind_WndMdi:
		{
			WndNum++;
			CKuinWndMdi* result = new CKuinWndMdi();
			CWnd* parent2 = parent == NULL ? theApp.m_pMainWnd : static_cast<CWnd*>(parent);
			result->Create(NULL, text, WS_OVERLAPPEDWINDOW, rect, parent2);
			result->ShowWindow(SW_SHOWNORMAL);
			result->UpdateWindow();
			return result;
		}
		case WndKind_WndMdiChild:
		{
			WndNum++;
			CKuinWndMdiChild* result = new CKuinWndMdiChild();
			CMDIFrameWnd* parent2 = parent == NULL ? NULL : static_cast<CMDIFrameWnd*>(parent);
			result->Create(NULL, text, WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, rect, parent2);
			result->ShowWindow(SW_SHOWNORMAL);
			result->UpdateWindow();
			return result;
		}
		case WndKind_WndDock:
		{
			WndNum++;
			CKuinWndDock* result = new CKuinWndDock();
			CWnd* parent2 = parent == NULL ? theApp.m_pMainWnd : static_cast<CWnd*>(parent);
			result->Create(NULL, text, WS_OVERLAPPEDWINDOW, rect, parent2);
			result->EnableAutoHidePanes(CBRS_ALIGN_ANY);
			result->ShowWindow(SW_SHOWNORMAL);
			result->UpdateWindow();
			return result;
		}
		case WndKind_WndDockChild:
		{
			WndNum++;
			CKuinWndDockChild* result = new CKuinWndDockChild();
			CFrameWndEx* parent2 = parent == NULL ? NULL : static_cast<CFrameWndEx*>(parent);
			result->Create(text, parent2, rect, TRUE, 1, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI);
			result->EnableDocking(CBRS_ALIGN_ANY);
			parent2->DockPane(result);
			result->ShowWindow(SW_SHOWNORMAL);
			result->UpdateWindow();
			return result;
		}
		case WndKind_Draw:
			{
				CKuinDraw* result = new CKuinDraw();
				result->Create(NULL, WS_VISIBLE | WS_CHILD, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Btn:
			{
				CKuinBtn* result = new CKuinBtn();
				result->Create(text, WS_VISIBLE | WS_CHILD | WS_TABSTOP, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Chk:
			{
				CKuinChk* result = new CKuinChk();
				result->Create(text, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Radio:
			{
				CKuinRadio* result = new CKuinRadio();
				result->Create(text, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Edit:
			{
				CKuinEdit* result = new CKuinEdit();
				result->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_EditMulti:
			{
				CKuinEditMulti* result = new CKuinEditMulti();
				result->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_List:
			{
				CKuinList* result = new CKuinList();
				result->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Combo:
			{
				CKuinCombo* result = new CKuinCombo();
				result->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWN, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_ComboList:
			{
				CKuinComboList* result = new CKuinComboList();
				result->Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Label:
			{
				CKuinLabel* result = new CKuinLabel();
				result->Create(text, WS_VISIBLE | WS_CHILD | SS_SIMPLE, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		case WndKind_Group:
			{
				CKuinGroup* result = new CKuinGroup();
				result->Create(text, WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, static_cast<CWnd*>(parent), default_id);
				return result;
			}
		default:
			return NULL;
	}
}

EXPORT_CPP void _showWnd(void* wnd)
{
	CWnd* wnd2 = static_cast<CWnd*>(wnd);
	wnd2->ShowWindow(SW_SHOWNORMAL);
	wnd2->UpdateWindow();
}

EXPORT_CPP void _destroyWnd(void* wnd)
{
	CWnd* wnd2 = static_cast<CWnd*>(wnd);
	wnd2->DestroyWindow();
}

EXPORT_CPP HWND _getHwnd(void* ptr)
{
	CWnd* ptr2 = static_cast<CWnd*>(ptr);
	return ptr2->m_hWnd;
}
