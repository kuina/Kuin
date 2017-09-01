// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include <commctrl.h>

#include "draw.h"
#include "snd.h"
#include "input.h"

enum EWndKind
{
	WndKind_WndNormal = 0x00,
	WndKind_WndFix,
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
	WndKind_Calendar,
	WndKind_Progress,
	WndKind_Rebar,
	WndKind_Status,
	WndKind_Toolbar,
	WndKind_Trackbar,
	WndKind_LabelLink,
	WndKind_ListView,
	WndKind_Pager,
	WndKind_Tab,
	WndKind_Tree,
	WndKind_SplitX,
	WndKind_SplitY,
	WndKind_ScrollX,
	WndKind_ScrollY,
};

enum ECtrlFlag
{
	CtrlFlag_AnchorLeft = 0x01,
	CtrlFlag_AnchorTop = 0x02,
	CtrlFlag_AnchorRight = 0x04,
	CtrlFlag_AnchorBottom = 0x08,
};

struct SWndBase
{
	SClass Class;
	EWndKind Kind;
	HWND WndHandle;
	WNDPROC DefaultWndProc;
	U64 CtrlFlag;
	U16 DefaultX;
	U16 DefaultY;
	U16 DefaultWidth;
	U16 DefaultHeight;
	void* Children;
};

struct SWnd
{
	SWndBase WndBase;
	U16 MinWidth;
	U16 MinHeight;
	U16 MaxWidth;
	U16 MaxHeight;
	void* OnClose;
	void* OnPushMenu;
	Bool ModalLock;
};

struct SDraw
{
	SWndBase WndBase;
	Bool EqualMagnification;
	Bool DrawTwice;
	Bool Enter;
	void* DrawBuf;
	void* OnPaint;
	void* OnMouseDownL;
	void* OnMouseDownR;
	void* OnMouseDownM;
	void* OnMouseUpL;
	void* OnMouseUpR;
	void* OnMouseUpM;
	void* OnMouseMove;
	void* OnMouseEnter;
	void* OnMouseLeave;
	void* OnMouseWheelX;
	void* OnMouseWheelY;
	void* OnFocus;
	void* OnKeyDown;
	void* OnKeyUp;
	void* OnKeyChar;
	void* OnScrollX;
	void* OnScrollY;
};

struct SBtn
{
	SWndBase WndBase;
	void* OnPush;
};

struct SChk
{
	SWndBase WndBase;
};

struct SRadio
{
	SWndBase WndBase;
};

struct SEdit
{
	SWndBase WndBase;
};

struct SEditMulti
{
	SWndBase WndBase;
};

struct SList
{
	SWndBase WndBase;
	void* OnSel;
};

struct SCombo
{
	SWndBase WndBase;
};

struct SComboList
{
	SWndBase WndBase;
};

struct SLabel
{
	SWndBase WndBase;
};

struct SGroup
{
	SWndBase WndBase;
};

struct SCalendar
{
	SWndBase WndBase;
};

struct SProgress
{
	SWndBase WndBase;
};

struct SRebar
{
	SWndBase WndBase;
};

struct SStatus
{
	SWndBase WndBase;
};

struct SToolbar
{
	SWndBase WndBase;
};

struct STrackbar
{
	SWndBase WndBase;
};

struct SLabelLink
{
	SWndBase WndBase;
};

struct SListView
{
	SWndBase WndBase;
};

struct SPager
{
	SWndBase WndBase;
};

struct STab
{
	SWndBase WndBase;
};

struct STree
{
	SWndBase WndBase;
};

struct SSplitX
{
	SWndBase WndBase;
};

struct SSplitY
{
	SWndBase WndBase;
};

struct SScroll
{
	SWndBase WndBase;
};

struct SMenu
{
	SClass Class;
	HMENU MenuHandle;
	void* Children;
};

static void* OnKeyPress;
static int WndCnt;
static Bool ExitAct;
static HFONT FontCtrl;

static const U8* NToRN(const Char* str);
static const U8* RNToN(const Char* str);
static void ParseAnchor(SWndBase* wnd, const SWndBase* parent, S64 anchor_x, S64 anchor_y, S64 x, S64 y, S64 width, S64 height);
static SWndBase* ToWnd(HWND wnd);
static void SetCtrlParam(SWndBase* wnd, SWndBase* parent, EWndKind kind, const Char* ctrl, DWORD style_ex, DWORD style, S64 x, S64 y, S64 width, S64 height, const Char* text, WNDPROC wnd_proc, S64 anchor_x, S64 anchor_y);
static BOOL CALLBACK ResizeCallback(HWND wnd, LPARAM l_param);
static void CommandAndNotify(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static Char* ParseFilter(const U8* filter);
static LRESULT CALLBACK WndProcWndNormal(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcWndFix(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcWndAspect(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcDraw(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcBtn(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcChk(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcRadio(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcEdit(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcEditMulti(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcList(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcCombo(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcComboList(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcLabel(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcGroup(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcCalendar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcProgress(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcRebar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcStatus(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcToolbar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcTrackbar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcLabelLink(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcListView(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcPager(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcTab(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcTree(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcSplitX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcSplitY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcScrollX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcScrollY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

SClass* IncWndRef(SClass* wnd)
{
	if (wnd != NULL)
		wnd->RefCnt++;
	return wnd;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : reinterpret_cast<const Char*>(app_name + 0x10);
	Instance = static_cast<HINSTANCE>(GetModuleHandle(NULL));

	WndCnt = 0;
	ExitAct = False;

	HICON icon = LoadIcon(Instance, reinterpret_cast<LPCWSTR>(static_cast<S64>(0x65))); // 0x65 is the resource ID of the application icon.
	{
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wnd_class.lpfnWndProc = WndProcWndNormal;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = icon;
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = L"KuinWndNormalClass";
		wnd_class.hIconSm = icon;
		RegisterClassEx(&wnd_class);
	}
	{
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wnd_class.lpfnWndProc = WndProcWndFix;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = icon;
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = L"KuinWndFixClass";
		wnd_class.hIconSm = icon;
		RegisterClassEx(&wnd_class);
	}
	{
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wnd_class.lpfnWndProc = WndProcWndAspect;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = icon;
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = L"KuinWndAspectClass";
		wnd_class.hIconSm = icon;
		RegisterClassEx(&wnd_class);
	}

	{
		HDC dc = GetDC(NULL);
		FontCtrl = CreateFont(-MulDiv(9, GetDeviceCaps(dc, LOGPIXELSY), 72), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
		ReleaseDC(NULL, dc);
	}

	Draw::Init();
	Snd::Init();
	Input::Init();

	OnKeyPress = NULL;
}

EXPORT_CPP void _fin()
{
	Input::Fin();
	Snd::Fin();
	Draw::Fin();

	DeleteObject(static_cast<HGDIOBJ>(FontCtrl));
}

EXPORT_CPP Bool _act()
{
	if (ExitAct || WndCnt == 0)
		return False;

	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
				case WM_QUIT:
					ExitAct = True;
					return False;
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					if (OnKeyPress != NULL)
					{
						U64 shiftCtrl = 0;
						shiftCtrl |= (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? 1 : 0;
						shiftCtrl |= (GetKeyState(VK_CONTROL) & 0x8000) != 0 ? 2 : 0;
						if (static_cast<Bool>(reinterpret_cast<U64>(Call2Asm(reinterpret_cast<void*>(static_cast<U64>(msg.wParam)), reinterpret_cast<void*>(shiftCtrl), OnKeyPress))))
							continue;
					}
					break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Input::Update();

	Sleep(1);

	return True;
}

EXPORT_CPP void _onKeyPress(void* onKeyPressFunc)
{
	OnKeyPress = onKeyPressFunc;
}

EXPORT_CPP S64 _msgBox(SClass* parent, const U8* text, const U8* title, S64 icon, S64 btn)
{
	return MessageBox(parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle, reinterpret_cast<const Char*>(text + 0x10), title == NULL ? AppName : reinterpret_cast<const Char*>(title + 0x10), static_cast<UINT>(icon | btn));
}

EXPORT_CPP void* _openFileDialog(SClass* parent, const U8* filter, S64 defaultFilter)
{
	Char path[MAX_PATH + 1];
	path[0] = L'\0';
	Char* filter_mem = ParseFilter(filter);
	OPENFILENAME open_file_name;
	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	open_file_name.lpstrFilter = filter_mem;
	open_file_name.nFilterIndex = static_cast<DWORD>(defaultFilter);
	open_file_name.lpstrFile = path;
	open_file_name.nMaxFile = MAX_PATH + 1;
	open_file_name.lpstrInitialDir = NULL;
	open_file_name.lpstrTitle = NULL;
	open_file_name.Flags = OFN_FILEMUSTEXIST;
	BOOL success = GetOpenFileName(&open_file_name);
	if (filter_mem != NULL)
		FreeMem(filter_mem);
	if (success == FALSE)
		return NULL;
	size_t len = wcslen(path);
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len + 1)));
	*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
	Char* dst = reinterpret_cast<Char*>(result + 0x10);
	for (size_t i = 0; i <= len; i++)
		dst[i] = path[i] == L'\\' ? L'/' : path[i];
	return result;
}

EXPORT_CPP void* _openFileDialogMulti(SClass* parent, const U8* filter, S64 defaultFilter)
{
	// TODO:
	return NULL;
}

EXPORT_CPP void* _saveFileDialog(SClass* parent, const U8* filter, S64 defaultFilter, const U8* defaultExt)
{
	Char path[MAX_PATH + 1];
	path[0] = L'\0';
	Char* filter_mem = ParseFilter(filter);
	OPENFILENAME open_file_name;
	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	open_file_name.lpstrFilter = filter_mem;
	open_file_name.nFilterIndex = static_cast<DWORD>(defaultFilter);
	open_file_name.lpstrFile = path;
	open_file_name.nMaxFile = MAX_PATH + 1;
	open_file_name.lpstrInitialDir = NULL;
	open_file_name.lpstrTitle = NULL;
	open_file_name.lpstrDefExt = defaultExt == NULL ? NULL : reinterpret_cast<const Char*>(defaultExt + 0x10);
	open_file_name.Flags = OFN_OVERWRITEPROMPT;
	BOOL success = GetSaveFileName(&open_file_name);
	if (filter_mem != NULL)
		FreeMem(filter_mem);
	if (success == FALSE)
		return NULL;
	size_t len = wcslen(path);
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len + 1)));
	*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
	Char* dst = reinterpret_cast<Char*>(result + 0x10);
	for (size_t i = 0; i <= len; i++)
		dst[i] = path[i] == L'\\' ? L'/' : path[i];
	return result;
}

EXPORT_CPP void _setClipboardStr(const U8* str)
{
	size_t len = static_cast<size_t>(*reinterpret_cast<const S64*>(str + 0x08));
	{
		const Char* ptr = reinterpret_cast<const Char*>(str + 0x10);
		while (*ptr != L'\0')
		{
			if (*ptr == L'\n')
				len++;
			ptr++;
		}
	}
	HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GHND, sizeof(Char) * (len + 1));
	if (handle == NULL)
		return;
	{
		const Char* ptr = reinterpret_cast<const Char*>(str + 0x10);
		Char* buf = static_cast<Char*>(GlobalLock(handle));
		if (buf == NULL)
		{
			GlobalFree(handle);
			return;
		}
		const Char* top = buf;
		UNUSED(top);
		while (*ptr != L'\0')
		{
			if (*ptr == L'\n')
			{
				*buf = L'\r';
				*buf++;
			}
			*buf = *ptr;
			*buf++;
			ptr++;
		}
		*buf = L'\0';
		ASSERT(top + len == buf);
		GlobalUnlock(handle);
	}
	if (OpenClipboard(NULL) == 0)
	{
		GlobalFree(handle);
		return;
	}
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, static_cast<HANDLE>(handle));
	CloseClipboard();
}

EXPORT_CPP void* _getClipboardStr()
{
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) == 0)
		return NULL;
	if (OpenClipboard(NULL) == 0)
		return NULL;
	HGLOBAL handle = GetClipboardData(CF_UNICODETEXT);
	if (handle == NULL)
	{
		CloseClipboard();
		return NULL;
	}
	U8* result = NULL;
	{
		const Char* buf = static_cast<Char*>(GlobalLock(handle));
		if (buf == NULL)
		{
			CloseClipboard();
			return NULL;
		}
		size_t len = 0;
		{
			const Char* ptr = buf;
			while (*ptr != L'\0')
			{
				if (*ptr != L'\r')
					len++;
				ptr++;
			}
		}
		result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
		*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
		*reinterpret_cast<S64*>(result + 0x08) = len;
		{
			const Char* src = buf;
			Char* dst = reinterpret_cast<Char*>(result + 0x10);
			const Char* top = dst;
			UNUSED(top);
			while (*src != L'\0')
			{
				if (*src != L'\r')
				{
					*dst = *src;
					dst++;
				}
				src++;
			}
			*dst = L'\0';
			ASSERT(top + len == dst);
		}
	}
	GlobalUnlock(handle);
	CloseClipboard();
	return result;
}

EXPORT_CPP void _target(SClass* me_)
{
	SDraw* me2 = reinterpret_cast<SDraw*>(me_);
	Draw::ActiveDrawBuf(me2->DrawBuf);
}

EXPORT_CPP SClass* _makeWnd(SClass* me_, SClass* parent, S64 style, S64 width, S64 height, const U8* text)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	me2->Kind = static_cast<EWndKind>(static_cast<S64>(WndKind_WndNormal) + style);
	ASSERT(width >= -1 && height >= -1);
	int width2 = width == -1 ? CW_USEDEFAULT : static_cast<int>(width);
	int height2 = height == -1 ? CW_USEDEFAULT : static_cast<int>(height);
	HWND parent2 = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	switch (me2->Kind)
	{
		case WndKind_WndNormal:
			me2->WndHandle = CreateWindowEx(0, L"KuinWndNormalClass", reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		case WndKind_WndFix:
			ASSERT(width >= 0 && height >= 0);
			me2->WndHandle = CreateWindowEx(0, L"KuinWndFixClass", reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		case WndKind_WndAspect:
			ASSERT(width >= 0 && height >= 0);
			me2->WndHandle = CreateWindowEx(0, L"KuinWndAspectClass", reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
			// TODO:
		default:
			ASSERT(False);
			break;
	}
	ASSERT(me2->WndHandle != NULL);
	me2->DefaultWndProc = NULL;
	me2->CtrlFlag = static_cast<U64>(CtrlFlag_AnchorLeft) | static_cast<U64>(CtrlFlag_AnchorTop);
	me2->DefaultX = 0;
	me2->DefaultY = 0;
	me2->DefaultWidth = static_cast<U16>(width);
	me2->DefaultHeight = static_cast<U16>(height);
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	{
		RECT rect;
		GetClientRect(me2->WndHandle, &rect);
		width += width - static_cast<S64>(rect.right - rect.left);
		height += height - static_cast<S64>(rect.bottom - rect.top);
		SetWindowPos(me2->WndHandle, NULL, 0, 0, static_cast<int>(width), static_cast<int>(height), SWP_NOMOVE | SWP_NOZORDER);
	}
	SetWindowLongPtr(me2->WndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me2));
	{
		SWnd* me3 = reinterpret_cast<SWnd*>(me_);
		me3->MinWidth = 128;
		me3->MinHeight = 128;
		me3->MaxWidth = static_cast<U16>(-1);
		me3->MaxHeight = static_cast<U16>(-1);
		me3->OnClose = NULL;
		me3->OnPushMenu = NULL;
		me3->ModalLock = False;
	}
	SendMessage(me2->WndHandle, WM_SETFONT, reinterpret_cast<WPARAM>(FontCtrl), static_cast<LPARAM>(FALSE));
	ShowWindow(me2->WndHandle, SW_SHOWNORMAL);
	WndCnt++;
	return me_;
}

EXPORT_CPP void _wndBaseDtor(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	DestroyWindow(me2->WndHandle);
}

EXPORT_CPP void _wndBaseGetSize(SClass* me_, S64* width, S64* height)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	RECT rect;
	GetClientRect(me2->WndHandle, &rect);
	*width = static_cast<S64>(rect.right - rect.left);
	*height = static_cast<S64>(rect.bottom - rect.top);
}

EXPORT_CPP void _wndBasePaint(SClass* me_)
{
	InvalidateRect(reinterpret_cast<SWndBase*>(me_)->WndHandle, NULL, FALSE);
}

EXPORT_CPP void _wndBaseFocus(SClass* me_)
{
	SetFocus(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP void _wndBaseEnable(SClass* me_, Bool is_enabled)
{
	EnableWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_enabled ? TRUE : FALSE);
}

EXPORT_CPP void _wndMinMax(SClass* me_, S64 minWidth, S64 minHeight, S64 maxWidth, S64 maxHeight)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->MinWidth = static_cast<U16>(minWidth);
	me2->MinHeight = static_cast<U16>(minHeight);
	me2->MaxWidth = static_cast<U16>(maxWidth);
	me2->MaxHeight = static_cast<U16>(maxHeight);
}

EXPORT_CPP void _wndClose(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, WM_CLOSE, 0, 0);
}

EXPORT_CPP void _wndExit(SClass* me_)
{
	DestroyWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP void _wndSetText(SClass* me_, const U8* text)
{
	const U8* text2 = NToRN(reinterpret_cast<const Char*>(text + 0x10));
	SetWindowText(reinterpret_cast<SWndBase*>(me_)->WndHandle, reinterpret_cast<const Char*>(text2 + 0x10));
	FreeMem(const_cast<U8*>(text2));
}

EXPORT_CPP const U8* _wndGetText(SClass* me_)
{
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	int len = GetWindowTextLength(wnd);
	Char* buf = static_cast<Char*>(AllocMem(sizeof(Char) * static_cast<size_t>(len + 1)));
	GetWindowText(wnd, buf, len + 1);
	const U8* result = RNToN(buf);
	FreeMem(buf);
	return result;
}

EXPORT_CPP void _wndReadonly(SClass* me_, Bool flag)
{
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	SendMessage(wnd, EM_SETREADONLY, flag ? TRUE : FALSE, 0);
}

EXPORT_CPP void _wndSetMenu(SClass* me_, SClass* menu)
{
	SetMenu(reinterpret_cast<SWndBase*>(me_)->WndHandle, menu == NULL ? NULL : reinterpret_cast<SMenu*>(menu)->MenuHandle);
}

EXPORT_CPP Bool _wndActive(SClass* me_)
{
	return GetActiveWindow() == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP void _wndSetModalLock(SClass* me_)
{
	HWND parent = GetWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, GW_OWNER);
	if (parent != NULL)
		EnableWindow(parent, FALSE);
	reinterpret_cast<SWnd*>(me_)->ModalLock = True;
}

EXPORT_CPP SClass* _makeDraw(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, Bool equalMagnification)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	SDraw* me3 = reinterpret_cast<SDraw*>(me_);
	SetCtrlParam(me2, reinterpret_cast<SWndBase*>(parent), WndKind_Draw, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY, x, y, width, height, L"", WndProcDraw, anchorX, anchorY);
	me3->EqualMagnification = equalMagnification;
	me3->DrawTwice = True;
	me3->Enter = False;
	me3->DrawBuf = Draw::MakeDrawBuf(static_cast<int>(width), static_cast<int>(height), me2->WndHandle);
	return me_;
}

EXPORT_CPP void _drawDtor(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	SDraw* me3 = reinterpret_cast<SDraw*>(me_);
	if (me3->DrawBuf != NULL)
		Draw::FinDrawBuf(me3->DrawBuf);
	DestroyWindow(me2->WndHandle);
}

EXPORT_CPP void _drawShowCaret(SClass* me_, S64 height, SClass* font)
{
	HWND wnd_handle = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	CreateCaret(wnd_handle, NULL, 2, static_cast<int>(height));
	ShowCaret(wnd_handle);
	if (font != NULL)
	{
		LOGFONT log_font;
		HIMC imc = ImmGetContext(wnd_handle);
		GetObject(Draw::ToFontHandle(font), sizeof(LOGFONT), &log_font);
		if (imc)
			ImmSetCompositionFont(imc, &log_font);
		ImmReleaseContext(wnd_handle, imc);
	}
}

EXPORT_CPP void _drawHideCaret(SClass* me_)
{
	UNUSED(me_);
	DestroyCaret();
}

EXPORT_CPP void _drawMoveCaret(SClass* me_, S64 x, S64 y)
{
	HWND wnd_handle = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	if (x == -1 && y == -1)
		SetCaretPos(-9999, -9999); // Hide the caret.
	else
		SetCaretPos(static_cast<int>(x), static_cast<int>(y));
	HIMC imc = ImmGetContext(wnd_handle);
	COMPOSITIONFORM form;
	form.dwStyle = CFS_POINT;
	if (x == -1 && y == -1)
	{
		form.ptCurrentPos.x = 0;
		form.ptCurrentPos.y = 0;
	}
	else
	{
		form.ptCurrentPos.x = static_cast<LONG>(x);
		form.ptCurrentPos.y = static_cast<LONG>(y);
	}
	if (imc)
		ImmSetCompositionWindow(imc, &form);
	ImmReleaseContext(wnd_handle, imc);
}

EXPORT_CPP SClass* _makeBtn(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SBtn* me2 = reinterpret_cast<SBtn*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Btn, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_NOTIFY, x, y, width, height, reinterpret_cast<const Char*>(text + 0x10), WndProcBtn, anchorX, anchorY);
	me2->OnPush = NULL;
	return me_;
}

EXPORT_CPP void _btnSetChk(SClass* me_, Bool chk)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	SendMessage(me2->WndHandle, BM_SETCHECK, static_cast<WPARAM>(chk ? BST_CHECKED : BST_UNCHECKED), 0);
}

EXPORT_CPP Bool _btnGetChk(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	return SendMessage(me2->WndHandle, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
}

EXPORT_CPP SClass* _makeChk(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Chk, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, width, height, reinterpret_cast<const Char*>(text + 0x10), WndProcChk, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeRadio(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Radio, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON, x, y, width, height, reinterpret_cast<const Char*>(text + 0x10), WndProcRadio, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeEdit(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Edit, WC_EDIT, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, x, y, width, height, L"", WndProcEdit, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeEditMulti(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_EditMulti, WC_EDIT, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN, x, y, width, height, L"", WndProcEditMulti, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_List, WC_LISTBOX, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_DISABLENOSCROLL | LBS_NOTIFY, x, y, width, height, L"", WndProcList, anchorX, anchorY);
	return me_;
}

EXPORT_CPP void _listClear(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_RESETCONTENT, 0, 0);
}

EXPORT_CPP void _listAdd(SClass* me_, const U8* text)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _listIns(SClass* me_, S64 idx, const U8* text)
{
#if defined(DBG)
	{
		S64 len = _listLen(me_);
		if (idx < 0 || len <= idx)
			THROW(0x1000, L"");
	}
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _listDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _listLen(me_);
		if (idx < 0 || len <= idx)
			THROW(0x1000, L"");
	}
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_DELETESTRING, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _listLen(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCOUNT, 0, 0));
}

EXPORT_CPP void _listSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _listLen(me_);
		if (idx < -1 || len <= idx)
			THROW(0x1000, L"");
	}
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _listGetSel(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCURSEL, 0, 0));
}

EXPORT_CPP void* _listGetText(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _listLen(me_);
		if (idx < 0 || len <= idx)
			THROW(0x1000, L"");
	}
#endif
	{
		size_t len = static_cast<size_t>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETTEXTLEN, static_cast<WPARAM>(idx), 0));
		U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
		*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
		*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETTEXT, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(result + 0x10));
		return result;
	}
}

EXPORT_CPP void _listSetText(SClass* me_, S64 idx, const U8* text)
{
#if defined(DBG)
	{
		S64 len = _listLen(me_);
		if (idx < 0 || len <= idx)
			THROW(0x1000, L"");
	}
#endif
	{
		int sel = static_cast<int>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCURSEL, 0, 0));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_DELETESTRING, static_cast<WPARAM>(idx + 1), 0);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_SETCURSEL, static_cast<WPARAM>(sel), 0);
	}
}

EXPORT_CPP SClass* _makeCombo(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Combo, WC_COMBOBOX, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWN, x, y, width, height, L"", WndProcCombo, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeComboList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_ComboList, WC_COMBOBOX, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST, x, y, width, height, L"", WndProcComboList, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeLabel(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Label, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_SIMPLE, x, y, width, height, reinterpret_cast<const Char*>(text + 0x10), WndProcLabel, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeGroup(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Group, WC_BUTTON, WS_EX_TRANSPARENT, WS_VISIBLE | WS_CHILD | BS_GROUPBOX, x, y, width, height, reinterpret_cast<const Char*>(text + 0x10), WndProcGroup, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeCalendar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Calendar, MONTHCAL_CLASS, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, width, height, L"", WndProcCalendar, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeProgress(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Progress, PROGRESS_CLASS, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcProgress, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeRebar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Rebar, REBARCLASSNAME, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcRebar, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeStatus(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Status, STATUSCLASSNAME, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcStatus, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeToolbar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Toolbar, TOOLBARCLASSNAME, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcToolbar, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeTrackbar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Trackbar, TRACKBAR_CLASS, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, width, height, L"", WndProcTrackbar, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeLabelLink(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_LabelLink, WC_LINK, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcLabelLink, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeListView(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_ListView, WC_LISTVIEW, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, width, height, L"", WndProcListView, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makePager(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Pager, WC_PAGESCROLLER, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcPager, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeTab(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Tab, WC_TABCONTROL, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, x, y, width, height, L"", WndProcTab, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeTree(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Tree, WC_TREEVIEW, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcTree, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeSplitX(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_SplitX, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY, x, y, width, height, L"", WndProcSplitX, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeSplitY(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_SplitY, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY, x, y, width, height, L"", WndProcSplitY, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeScrollX(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_ScrollX, WC_SCROLLBAR, 0, WS_VISIBLE | WS_CHILD | SBS_HORZ, x, y, width, height, L"", WndProcScrollX, anchorX, anchorY);
	_scrollSetState(me_, 0, 0, 1, 0);
	return me_;
}

EXPORT_CPP SClass* _makeScrollY(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_ScrollY, WC_SCROLLBAR, 0, WS_VISIBLE | WS_CHILD | SBS_VERT, x, y, width, height, L"", WndProcScrollY, anchorX, anchorY);
	_scrollSetState(me_, 0, 0, 1, 0);
	return me_;
}

EXPORT_CPP void _scrollSetState(SClass* me_, S64 min, S64 max, S64 page, S64 value)
{
	if (max < min)
		max = min;
	if (page < 1)
		page = 1;
	if (value < min)
		value = min;
	if (value > max)
		value = max;
	SCROLLINFO info;
	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	info.nMin = static_cast<int>(min);
	info.nMax = static_cast<int>(max);
	info.nPage = static_cast<int>(page);
	info.nPos = static_cast<int>(value);
	info.nTrackPos = 0;
	SetScrollInfo(reinterpret_cast<SWndBase*>(me_)->WndHandle, SB_CTL, &info, TRUE);
}

EXPORT_CPP SClass* _makeMenu(SClass* me_)
{
	SMenu* me2 = reinterpret_cast<SMenu*>(me_);
	me2->MenuHandle = CreateMenu();
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	return me_;
}

EXPORT_CPP void _menuDtor(SClass* me_)
{
	DestroyMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle);
}

EXPORT_CPP void _menuAdd(SClass* me_, S64 id, const U8* text)
{
	ASSERT(0x0001 <= id && id <= 0xffff);
	ASSERT(text != NULL);
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_STRING, static_cast<UINT_PTR>(id), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuAddLine(SClass* me_)
{
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_SEPARATOR, 0, NULL);
}

EXPORT_CPP void _menuAddPopup(SClass* me_, const U8* text, const U8* popup)
{
	ASSERT(text != NULL);
	ASSERT(popup != NULL);
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP SClass* _makePopup(SClass* me_)
{
	SMenu* me2 = reinterpret_cast<SMenu*>(me_);
	me2->MenuHandle = CreatePopupMenu();
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	return me_;
}

static const U8* NToRN(const Char* str)
{
	size_t len = 0;
	{
		const Char* ptr = str;
		Char back = L'\0';
		while (*ptr != L'\0')
		{
			len++;
			if (back != L'\r' && *ptr == L'\n')
				len++;
			back = *ptr;
			ptr++;
		}
	}
	U8* buf = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	*reinterpret_cast<S64*>(buf + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(buf + 0x08) = static_cast<S64>(len);
	{
		Char* dst = reinterpret_cast<Char*>(buf + 0x10);
		const Char* src = str;
		Char back = L'\0';
		while (*src != L'\0')
		{
			if (back != L'\r' && *src == L'\n')
			{
				*dst = L'\r';
				dst++;
				*dst = L'\n';
			}
			else
				*dst = *src;
			dst++;
			back = *src;
			src++;
		}
		ASSERT(reinterpret_cast<U8*>(dst) == buf + 0x10 + sizeof(Char) * len);
		*dst = L'\0';
	}
	return buf;
}

static const U8* RNToN(const Char* str)
{
	size_t len = 0;
	{
		const Char* ptr = str;
		Char back = L'\0';
		while (*ptr != L'\0')
		{
			len++;
			if (back == L'\r' && *ptr == L'\n')
				len--;
			back = *ptr;
			ptr++;
		}
	}
	U8* buf = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	*reinterpret_cast<S64*>(buf + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(buf + 0x08) = static_cast<S64>(len);
	{
		Char* dst = reinterpret_cast<Char*>(buf + 0x10);
		const Char* src = str;
		Char back = L'\0';
		while (*src != L'\0')
		{
			if (back == L'\r' && *src == L'\n')
			{
				dst--;
				*dst = L'\n';
			}
			else
				*dst = *src;
			dst++;
			back = *src;
			src++;
		}
		ASSERT(reinterpret_cast<U8*>(dst) == buf + 0x10 + sizeof(Char) * len);
		*dst = L'\0';
	}
	return buf;
}

static void ParseAnchor(SWndBase* wnd, const SWndBase* parent, S64 anchor_x, S64 anchor_y, S64 x, S64 y, S64 width, S64 height)
{
	ASSERT(x == static_cast<S64>(static_cast<U16>(x)) && y == static_cast<S64>(static_cast<U16>(y)) && width == static_cast<S64>(static_cast<U16>(width)) && height == static_cast<S64>(static_cast<U16>(height)));
	wnd->CtrlFlag = 0;
	switch (anchor_x)
	{
		case 0: // fix
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorLeft);
			break;
		case 1: // move
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorRight);
			break;
		case 2: // scale
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorLeft);
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorRight);
			break;
		default:
			ASSERT(False);
			break;
	}
	switch (anchor_y)
	{
		case 0: // fix
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorTop);
			break;
		case 1: // move
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorBottom);
			break;
		case 2: // scale
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorTop);
			wnd->CtrlFlag |= static_cast<U64>(CtrlFlag_AnchorBottom);
			break;
		default:
			ASSERT(False);
			break;
	}
	RECT parent_rect;
	if (parent->Kind < 0x80)
	{
		parent_rect.left = 0;
		parent_rect.top = 0;
		parent_rect.right = parent->DefaultWidth;
		parent_rect.bottom = parent->DefaultHeight;
	}
	else
		GetClientRect(parent->WndHandle, &parent_rect);
	if ((wnd->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorLeft)) != 0)
		wnd->DefaultX = static_cast<U16>(x);
	else
		wnd->DefaultX = static_cast<U16>(static_cast<U64>(parent_rect.right - parent_rect.left) - x);
	if ((wnd->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorTop)) != 0)
		wnd->DefaultY = static_cast<U16>(y);
	else
		wnd->DefaultY = static_cast<U16>(static_cast<U64>(parent_rect.bottom - parent_rect.top) - y);
	if ((wnd->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorRight)) != 0)
		wnd->DefaultWidth = static_cast<U16>(static_cast<U64>(parent_rect.right - parent_rect.left) - x - width);
	else
		wnd->DefaultWidth = static_cast<U16>(width);
	if ((wnd->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorBottom)) != 0)
		wnd->DefaultHeight = static_cast<U16>(static_cast<U64>(parent_rect.bottom - parent_rect.top) - y - height);
	else
		wnd->DefaultHeight = static_cast<U16>(height);
	ResizeCallback(wnd->WndHandle, 0);
}

static SWndBase* ToWnd(HWND wnd)
{
	return reinterpret_cast<SWndBase*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
}

static void SetCtrlParam(SWndBase* wnd, SWndBase* parent, EWndKind kind, const Char* ctrl, DWORD style_ex, DWORD style, S64 x, S64 y, S64 width, S64 height, const Char* text, WNDPROC wnd_proc, S64 anchor_x, S64 anchor_y)
{
	ASSERT(parent != NULL);
	ASSERT(width >= 0 && height >= 0);
	wnd->Kind = kind;
	wnd->WndHandle = CreateWindowEx(style_ex, ctrl, text, style, static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), parent->WndHandle, NULL, Instance, NULL);
	ASSERT(wnd->WndHandle != NULL);
	SetWindowLongPtr(wnd->WndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
	wnd->DefaultWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(wnd->WndHandle, GWLP_WNDPROC));
	wnd->Children = AllocMem(0x28);
	*(S64*)wnd->Children = 1;
	memset((U8*)wnd->Children + 0x08, 0x00, 0x20);
	SetWindowLongPtr(wnd->WndHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc));
	ParseAnchor(wnd, parent, anchor_x, anchor_y, x, y, width, height);
	SendMessage(wnd->WndHandle, WM_SETFONT, reinterpret_cast<WPARAM>(FontCtrl), static_cast<LPARAM>(FALSE));
}

static BOOL CALLBACK ResizeCallback(HWND wnd, LPARAM l_param)
{
	UNUSED(l_param);
	SWndBase* wnd2 = ToWnd(wnd);
	if (wnd2->CtrlFlag == (static_cast<U64>(CtrlFlag_AnchorLeft) | static_cast<U64>(CtrlFlag_AnchorTop)))
		return TRUE;
	RECT parent_rect = { 0 };
	{
		HWND parent = GetAncestor(wnd, GA_PARENT);
		Bool first = False;
		if (l_param == 0)
		{
			SWndBase* wnd3 = ToWnd(parent);
			if (wnd3->Kind < 0x80)
			{
				parent_rect.left = wnd3->DefaultX;
				parent_rect.top = wnd3->DefaultY;
				parent_rect.right = wnd3->DefaultWidth;
				parent_rect.bottom = wnd3->DefaultHeight;
				first = True;
			}
		}
		if (!first)
			GetClientRect(parent, &parent_rect);
	}
	int width = static_cast<int>(parent_rect.right - parent_rect.left);
	int height = static_cast<int>(parent_rect.bottom - parent_rect.top);
	int new_x = static_cast<int>(wnd2->DefaultX);
	int new_y = static_cast<int>(wnd2->DefaultY);
	int new_width = static_cast<int>(wnd2->DefaultWidth);
	int new_height = static_cast<int>(wnd2->DefaultHeight);
	if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorLeft)) == 0)
		new_x = width - new_x;
	if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorTop)) == 0)
		new_y = height - new_y;
	if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorRight)) != 0)
		new_width = width - new_x - new_width;
	if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorBottom)) != 0)
		new_height = height - new_y - new_height;
	SetWindowPos(wnd, NULL, new_x, new_y, new_width, new_height, SWP_NOZORDER);
	return TRUE;
}

static void CommandAndNotify(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	if (msg == WM_COMMAND)
	{
		if (l_param == 0)
		{
			// A menu item is clicked.
			SWnd* wnd2 = reinterpret_cast<SWnd*>(ToWnd(wnd));
			if (wnd2->OnPushMenu != NULL)
				Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<U64>(LOWORD(w_param))), wnd2->OnPushMenu);
			return;
		}
		HWND wnd_ctrl = reinterpret_cast<HWND>(l_param);
		SWndBase* wnd_ctrl2 = ToWnd(wnd_ctrl);
		switch (wnd_ctrl2->Kind)
		{
			case WndKind_Btn:
				{
					SBtn* btn = reinterpret_cast<SBtn*>(wnd_ctrl2);
					switch (HIWORD(w_param))
					{
						case BCN_HOTITEMCHANGE:
							// TODO:
							break;
						case BN_CLICKED:
							if (btn->OnPush != NULL)
								Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), btn->OnPush);
							break;
						case BN_DBLCLK:
							// TODO:
							break;
						case BN_KILLFOCUS:
							// TODO:
							break;
						case BN_PAINT:
							// TODO:
							break;
						case BN_SETFOCUS:
							// TODO:
							break;
					}
				}
				break;
			case WndKind_Edit:
				{
					SEdit* edit = reinterpret_cast<SEdit*>(wnd_ctrl2);
					switch (HIWORD(w_param))
					{
						case EN_CHANGE:
							// TODO:
							break;
						case EN_HSCROLL:
							// TODO:
							break;
						case EN_KILLFOCUS:
							// TODO:
							break;
						case EN_SETFOCUS:
							// TODO:
							break;
						case EN_UPDATE:
							// TODO:
							break;
						case EN_VSCROLL:
							// TODO:
							break;
					}
				}
				break;
			case WndKind_List:
				{
					SList* list = reinterpret_cast<SList*>(wnd_ctrl2);
					switch (HIWORD(w_param))
					{
						case LBN_KILLFOCUS:
							// TODO:
							break;
						case LBN_SELCHANGE:
							if (list->OnSel != NULL)
								Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), list->OnSel);
							break;
						case LBN_SETFOCUS:
							// TODO:
							break;
					}
				}
				break;
			// TODO:
		}
	}
	else
	{
		ASSERT(msg == WM_NOTIFY);
		HWND wnd_ctrl = reinterpret_cast<LPNMHDR>(l_param)->hwndFrom;
		SWndBase* wnd_ctrl2 = ToWnd(wnd_ctrl);
		if (wnd_ctrl2 != NULL)
		{
			switch (wnd_ctrl2->Kind)
			{
				case WndKind_Tab:
					{
						STab* tab = reinterpret_cast<STab*>(wnd_ctrl2);
						switch (reinterpret_cast<LPNMHDR>(l_param)->code)
						{
							case NM_CLICK:
								// TODO:
								break;
							case NM_DBLCLK:
								// TODO:
								break;
							case NM_RCLICK:
								// TODO:
								break;
							case NM_RELEASEDCAPTURE:
								// TODO:
								break;
							case TCN_FOCUSCHANGE:
								// TODO:
								break;
							case TCN_GETOBJECT:
								// TODO:
								break;
							case TCN_KEYDOWN:
								// TODO:
								break;
							case TCN_SELCHANGE:
								// TODO:
								break;
							case TCN_SELCHANGING:
								// TODO:
								break;
						}
					}
					break;
				// TODO:
			}
		}
	}
}

static Char* ParseFilter(const U8* filter)
{
	if (filter == NULL)
		return NULL;
	S64 len_parent = *reinterpret_cast<const S64*>(filter + 0x08);
	ASSERT(len_parent % 2 == 0);
	S64 total = 0;
	{
		const void*const* ptr = reinterpret_cast<const void*const*>(filter + 0x10);
		for (S64 i = 0; i < len_parent; i++)
		{
			S64 len = *reinterpret_cast<const S64*>(static_cast<const U8*>(*ptr) + 0x08);
			total += len + 1;
			ptr++;
		}
	}
	Char* result = static_cast<Char*>(AllocMem(sizeof(Char) * static_cast<size_t>(total + 1)));
	{
		const void*const* ptr = reinterpret_cast<const void*const*>(filter + 0x10);
		Char* ptr2 = result;
		for (S64 i = 0; i < len_parent; i++)
		{
			S64 len = *reinterpret_cast<const S64*>(static_cast<const U8*>(*ptr) + 0x08);
			memcpy(ptr2, static_cast<const U8*>(*ptr) + 0x10, sizeof(Char) * static_cast<size_t>(len + 1));
			ptr++;
			ptr2 += len + 1;
		}
		*ptr2 = L'\0';
	}
	return result;
}

static LRESULT CALLBACK WndProcWndNormal(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SWnd* wnd3 = reinterpret_cast<SWnd*>(wnd2);
	if (wnd2 == NULL)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndNormal);
	switch (msg)
	{
		case WM_CLOSE:
			if (wnd3->OnClose != NULL)
			{
				if (!static_cast<Bool>(reinterpret_cast<U64>(Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnClose))))
					return 0;
			}
			DestroyWindow(wnd);
			return 0;
		case WM_DESTROY:
			WndCnt--;
			if (wnd3->ModalLock)
			{
				HWND parent = GetWindow(wnd2->WndHandle, GW_OWNER);
				if (parent != NULL)
				{
					EnableWindow(parent, TRUE);
					SetActiveWindow(parent);
				}
				wnd3->ModalLock = False;
			}
			return 0;
		case WM_SIZE:
			EnumChildWindows(wnd, ResizeCallback, 1);
			return 0;
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO info = reinterpret_cast<LPMINMAXINFO>(l_param);
				if (wnd3->MinWidth != -1)
					info->ptMinTrackSize.x = static_cast<LONG>(wnd3->MinWidth);
				if (wnd3->MinHeight != -1)
					info->ptMinTrackSize.y = static_cast<LONG>(wnd3->MinHeight);
				if (wnd3->MaxWidth != -1)
					info->ptMaxTrackSize.x = static_cast<LONG>(wnd3->MaxWidth);
				if (wnd3->MaxHeight != -1)
					info->ptMaxTrackSize.y = static_cast<LONG>(wnd3->MaxHeight);
			}
			return 0;
		case WM_COMMAND:
		case WM_NOTIFY:
			CommandAndNotify(wnd, msg, w_param, l_param);
			return 0;
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcWndFix(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SWnd* wnd3 = reinterpret_cast<SWnd*>(wnd2);
	if (wnd2 == NULL)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndFix);
	switch (msg)
	{
		case WM_CLOSE:
			if (wnd3->OnClose != NULL)
			{
				if (!static_cast<Bool>(reinterpret_cast<U64>(Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnClose))))
					return 0;
			}
			DestroyWindow(wnd);
			return 0;
		case WM_DESTROY:
			WndCnt--;
			if (wnd3->ModalLock)
			{
				HWND parent = GetWindow(wnd2->WndHandle, GW_OWNER);
				if (parent != NULL)
				{
					EnableWindow(parent, TRUE);
					SetActiveWindow(parent);
				}
				wnd3->ModalLock = False;
			}
			return 0;
		case WM_COMMAND:
		case WM_NOTIFY:
			CommandAndNotify(wnd, msg, w_param, l_param);
			return 0;
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcWndAspect(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SWnd* wnd3 = reinterpret_cast<SWnd*>(wnd2);
	if (wnd2 == NULL)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndAspect);
	switch (msg)
	{
		case WM_CLOSE:
			if (wnd3->OnClose != NULL)
			{
				if (!static_cast<Bool>(reinterpret_cast<U64>(Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnClose))))
					return 0;
			}
			DestroyWindow(wnd);
			return 0;
		case WM_DESTROY:
			WndCnt--;
			if (wnd3->ModalLock)
			{
				HWND parent = GetWindow(wnd2->WndHandle, GW_OWNER);
				if (parent != NULL)
				{
					EnableWindow(parent, TRUE);
					SetActiveWindow(parent);
				}
				wnd3->ModalLock = False;
			}
			return 0;
		case WM_SIZE:
			EnumChildWindows(wnd, ResizeCallback, 1);
			return 0;
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO info = reinterpret_cast<LPMINMAXINFO>(l_param);
				if (wnd3->MinWidth != -1)
					info->ptMinTrackSize.x = static_cast<LONG>(wnd3->MinWidth);
				if (wnd3->MinHeight != -1)
					info->ptMinTrackSize.y = static_cast<LONG>(wnd3->MinHeight);
				if (wnd3->MaxWidth != -1)
					info->ptMaxTrackSize.x = static_cast<LONG>(wnd3->MaxWidth);
				if (wnd3->MaxHeight != -1)
					info->ptMaxTrackSize.y = static_cast<LONG>(wnd3->MaxHeight);
			}
			return 0;
		case WM_COMMAND:
		case WM_NOTIFY:
			CommandAndNotify(wnd, msg, w_param, l_param);
			return 0;
			// TODO: Aspect.
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcDraw(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SDraw* wnd3 = reinterpret_cast<SDraw*>(wnd2);
	ASSERT(wnd2->Kind == WndKind_Draw);
	switch (msg)
	{
		case WM_PAINT:
			if (wnd3->OnPaint != NULL)
			{
				RECT rect;
				GetClientRect(wnd, &rect);
				PAINTSTRUCT ps;
				BeginPaint(wnd, &ps);
				Draw::ActiveDrawBuf(wnd3->DrawBuf);
				if (wnd3->DrawTwice)
				{
					Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(rect.right - rect.left)), reinterpret_cast<void*>(static_cast<S64>(rect.bottom - rect.top)), wnd3->OnPaint);
					wnd3->DrawTwice = False;
				}
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(rect.right - rect.left)), reinterpret_cast<void*>(static_cast<S64>(rect.bottom - rect.top)), wnd3->OnPaint);
				EndPaint(wnd, &ps);
				return 0;
			}
			break;
		case WM_LBUTTONDOWN:
			SetFocus(wnd);
			if (wnd3->OnMouseDownL != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseDownL);
			return 0;
		case WM_LBUTTONUP:
			if (wnd3->OnMouseUpL != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseUpL);
			return 0;
		case WM_RBUTTONDOWN:
			if (wnd3->OnMouseDownR != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseDownR);
			return 0;
		case WM_RBUTTONUP:
			if (wnd3->OnMouseUpR != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseUpR);
			return 0;
		case WM_MBUTTONDOWN:
			if (wnd3->OnMouseDownM != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseDownM);
			return 0;
		case WM_MBUTTONUP:
			if (wnd3->OnMouseUpM != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseUpM);
			return 0;
		case WM_MOUSEMOVE:
			if (!wnd3->Enter)
			{
				wnd3->Enter = True;
				if (wnd3->OnMouseEnter != NULL)
					Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseEnter);
			}
			if (wnd3->OnMouseMove != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(l_param))), reinterpret_cast<void*>(static_cast<S64>(HIWORD(l_param))), wnd3->OnMouseMove);
			{
				TRACKMOUSEEVENT track_mouse_event;
				track_mouse_event.cbSize = sizeof(track_mouse_event);
				track_mouse_event.dwFlags = TME_LEAVE;
				track_mouse_event.dwHoverTime = HOVER_DEFAULT;
				track_mouse_event.hwndTrack = wnd;
				TrackMouseEvent(&track_mouse_event);
			}
			return 0;
		case WM_MOUSELEAVE:
			wnd3->Enter = False;
			if (wnd3->OnMouseLeave != NULL)
				Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnMouseLeave);
			return 0;
		case WM_SETFOCUS:
			if (wnd3->OnFocus != NULL)
				Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<U64>(True)), wnd3->OnFocus);
			return 0;
		case WM_KILLFOCUS:
			if (wnd3->OnFocus != NULL)
				Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<U64>(False)), wnd3->OnFocus);
			return 0;
		case WM_KEYDOWN:
			{
				U64 shiftCtrl = 0;
				shiftCtrl |= (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? 1 : 0;
				shiftCtrl |= (GetKeyState(VK_CONTROL) & 0x8000) != 0 ? 2 : 0;
				if (wnd3->OnKeyDown != NULL)
					Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<U64>(w_param)), reinterpret_cast<void*>(shiftCtrl), wnd3->OnKeyDown);
			}
			return 0;
		case WM_KEYUP:
			{
				U64 shiftCtrl = 0;
				shiftCtrl |= (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? 1 : 0;
				shiftCtrl |= (GetKeyState(VK_CONTROL) & 0x8000) != 0 ? 2 : 0;
				if (wnd3->OnKeyUp != NULL)
					Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<U64>(w_param)), reinterpret_cast<void*>(shiftCtrl), wnd3->OnKeyUp);
			}
			return 0;
		case WM_CHAR:
			if (wnd3->OnKeyChar != NULL)
				Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<U64>(w_param)), wnd3->OnKeyChar);
			return 0;
		case WM_SIZE:
			{
				int width = static_cast<int>(LOWORD(l_param));
				int height = static_cast<int>(HIWORD(l_param));
				if (wnd3->EqualMagnification && width > 0 && height > 0)
				{
					if (wnd3->DrawBuf != NULL)
						Draw::FinDrawBuf(wnd3->DrawBuf);
					wnd3->DrawBuf = Draw::MakeDrawBuf(width, height, wnd2->WndHandle);
					wnd3->DrawTwice = True;
				}
			}
			return 0;
		case WM_HSCROLL:
		case WM_VSCROLL:
			{
				HWND scroll = reinterpret_cast<HWND>(l_param);
				SCROLLINFO info;
				info.cbSize = sizeof(SCROLLINFO);
				info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
				info.nTrackPos = 0;
				GetScrollInfo(scroll, SB_CTL, &info);
				switch (LOWORD(w_param))
				{
					case SB_LINEUP:
						info.nPos--;
						break;
					case SB_LINEDOWN:
						info.nPos++;
						break;
					case SB_PAGEUP:
						info.nPos -= info.nPage;
						break;
					case SB_PAGEDOWN:
						info.nPos += info.nPage;
						break;
					case SB_TOP:
						info.nPos = info.nMin;
						break;
					case SB_BOTTOM:
						info.nPos = info.nMax;
						break;
					case SB_THUMBPOSITION:
					case SB_THUMBTRACK:
						info.nPos = (int)HIWORD(w_param);
						break;
				}
				if (info.nPos < info.nMin)
					info.nPos = info.nMin;
				if (info.nPos > info.nMax)
					info.nPos = info.nMax;
				SetScrollInfo(scroll, SB_CTL, &info, TRUE);
				if (msg == WM_HSCROLL)
				{
					if (wnd3->OnScrollX != NULL)
						Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(info.nPos)), wnd3->OnScrollX);
				}
				else
				{
					if (wnd3->OnScrollY != NULL)
						Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(info.nPos)), wnd3->OnScrollY);
				}
			}
			return 0;
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcBtn(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Btn);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcChk(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Chk);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcRadio(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Radio);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcEdit(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Edit);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcEditMulti(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_EditMulti);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcList(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_List);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcCombo(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Combo);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcComboList(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_ComboList);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcLabel(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Label);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcGroup(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Group);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcCalendar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Calendar);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcProgress(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Progress);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcRebar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Rebar);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcStatus(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Status);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcToolbar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Toolbar);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcTrackbar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Trackbar);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcLabelLink(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_LabelLink);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcListView(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_ListView);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcPager(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Pager);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcTab(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Tab);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcTree(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_Tree);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcSplitX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_SplitX);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcSplitY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_SplitY);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcScrollX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_ScrollX);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcScrollY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_ScrollY);
	switch (msg)
	{
		// TODO:
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}
