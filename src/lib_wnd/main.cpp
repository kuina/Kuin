// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include <commctrl.h>

#include "draw.h"
#include "snd.h"
#include "input.h"
#include "png_decoder.h"

#pragma comment(lib, "Comctl32.lib")

enum EWndKind
{
	WndKind_WndNormal = 0x00,
	WndKind_WndFix,
	WndKind_WndAspect,
	WndKind_WndPopup,
	WndKind_WndDialog,
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
	WndKind_WndLayered = 0x10000,
	WndKind_WndNoMinimize = 0x20000,
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
	U8* Name;
	EWndKind Kind;
	HWND WndHandle;
	WNDPROC DefaultWndProc;
	U64 CtrlFlag;
	U16 DefaultX;
	U16 DefaultY;
	U16 DefaultWidth;
	U16 DefaultHeight;
	Bool RedrawEnabled;
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
	void* OnActivate;
	void* OnPushMenu;
	void* OnDropFiles;
	void* OnResize;
	Bool ModalLock;
};

struct SDraw
{
	SWndBase WndBase;
	Bool EqualMagnification;
	Bool DrawTwice;
	Bool Enter;
	Bool Editable;
	S16 WheelX;
	S16 WheelY;
	void* DrawBuf;
	void* OnPaint;
	void* OnMouseDownL;
	void* OnMouseDownR;
	void* OnMouseDownM;
	void* OnMouseDoubleClick;
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
	void* OnSetMouseImg;
};

struct SBtn
{
	SWndBase WndBase;
	void* OnPush;
};

struct SChk
{
	SWndBase WndBase;
	void* OnPush;
};

struct SRadio
{
	SWndBase WndBase;
	void* OnPush;
};

struct SEditBase
{
	SWndBase WndBase;
	void* OnChange;
	void* OnFocus;
};

struct SEdit
{
	SEditBase EditBase;
};

struct SEditMulti
{
	SEditBase EditBase;
};

struct SList
{
	SWndBase WndBase;
	void* OnSel;
	void* OnMouseDoubleClick;
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
	void* OnSel;
	void* OnMouseDoubleClick;
	void* OnMouseClick;
	void* OnMoveNode;
	Bool Draggable;
	Bool Dragging;
	HIMAGELIST DraggingImage;
};

struct SPager
{
	SWndBase WndBase;
};

struct STab
{
	SWndBase WndBase;
	void* OnSel;
};

struct STree
{
	SWndBase WndBase;
	Bool Draggable;
	Bool AllowDraggingToRoot;
	HTREEITEM DraggingItem;
	void* OnSel;
	void* OnMoveNode;
};

struct STreeNode
{
	SClass Class;
	HWND WndHandle;
	HTREEITEM Item;
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

struct STabOrder
{
	SClass Class;
	void* Ctrls;
};

static void* OnKeyPress;
static int WndCnt;
static Bool ExitAct;
static HFONT FontCtrl;
static Char FileDialogDir[KUIN_MAX_PATH + 1];
static HINSTANCE Instance;

static const U8* NToRN(const Char* str);
static const U8* RNToN(const Char* str);
static void ParseAnchor(SWndBase* wnd, S64 anchor_x, S64 anchor_y, S64 x, S64 y, S64 width, S64 height);
static SWndBase* ToWnd(HWND wnd);
static void SetCtrlParam(SWndBase* wnd, SWndBase* parent, EWndKind kind, const Char* ctrl, DWORD style_ex, DWORD style, S64 x, S64 y, S64 width, S64 height, const Char* text, WNDPROC wnd_proc, S64 anchor_x, S64 anchor_y);
static BOOL CALLBACK ResizeCallback(HWND wnd, LPARAM l_param);
static void CommandAndNotify(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static Char* ParseFilter(const U8* filter, int* num);
static void TreeExpandAllRecursion(HWND wnd_handle, HTREEITEM node, int flag);
static void CopyTreeNodeRecursion(HWND tree_wnd, HTREEITEM dst, HTREEITEM src, Char* buf);
static void ListViewAdjustWidth(HWND wnd);
static SClass* MakeDrawImpl(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchor_x, S64 anchor_y, Bool equal_magnification, Bool editable, int split);
static HIMAGELIST CreateImageList(const void* imgs);
static LRESULT CALLBACK CommonWndProc(HWND wnd, SWndBase* wnd2, SWnd* wnd3, UINT msg, WPARAM w_param, LPARAM l_param);
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

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (!InitEnvVars(heap, heap_cnt, app_code, use_res_flags))
		return;

	WndCnt = 0;
	ExitAct = False;

	Instance = (HINSTANCE)GetModuleHandle(NULL);
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
		wnd_class.lpfnWndProc = WndProcWndFix;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = NULL;
		wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = L"KuinWndDialogClass";
		wnd_class.hIconSm = NULL;
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
	FileDialogDir[0] = L'\0';
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

EXPORT_CPP void _setOnKeyPress(void* onKeyPressFunc)
{
	OnKeyPress = onKeyPressFunc;
}

EXPORT_CPP void* _getOnKeyPress()
{
	return OnKeyPress;
}

EXPORT_CPP S64 _msgBox(SClass* parent, const U8* text, const U8* title, S64 icon, S64 btn)
{
	return MessageBox(parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle, text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), title == NULL ? L"" : reinterpret_cast<const Char*>(title + 0x10), static_cast<UINT>(icon | btn));
}

EXPORT_CPP void* _openFileDialog(SClass* parent, const U8* filter, S64 defaultFilter)
{
	Char path[KUIN_MAX_PATH + 1];
	path[0] = L'\0';
	int filter_num;
	Char* filter_mem = ParseFilter(filter, &filter_num);
	THROWDBG(!(filter_num == 0 && defaultFilter == 0 || filter_num != 0 && 0 <= defaultFilter && defaultFilter < filter_num), EXCPT_DBG_ARG_OUT_DOMAIN);
	OPENFILENAME open_file_name;
	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	open_file_name.lpstrFilter = filter_mem;
	open_file_name.nFilterIndex = filter_num == 0 ? 0 : static_cast<DWORD>(defaultFilter + 1);
	open_file_name.lpstrFile = path;
	open_file_name.nMaxFile = KUIN_MAX_PATH + 1;
	open_file_name.lpstrInitialDir = FileDialogDir[0] == L'\0' ? NULL : FileDialogDir;
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
	Char path[KUIN_MAX_PATH + 1];
	path[0] = L'\0';
	int filter_num;
	Char* filter_mem = ParseFilter(filter, &filter_num);
	THROWDBG(!(filter_num == 0 && defaultFilter == 0 || filter_num != 0 && 0 <= defaultFilter && defaultFilter < filter_num), EXCPT_DBG_ARG_OUT_DOMAIN);
	OPENFILENAME open_file_name;
	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	open_file_name.lpstrFilter = filter_mem;
	open_file_name.nFilterIndex = filter_num == 0 ? 0 : static_cast<DWORD>(defaultFilter + 1);
	open_file_name.lpstrFile = path;
	open_file_name.nMaxFile = KUIN_MAX_PATH + 1;
	open_file_name.lpstrInitialDir = FileDialogDir[0] == L'\0' ? NULL : FileDialogDir;
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

EXPORT_CPP void _fileDialogDir(const U8* defaultDir)
{
	if (defaultDir == NULL)
	{
		FileDialogDir[0] = L'\0';
		return;
	}
	const Char* path = reinterpret_cast<const Char*>(defaultDir + 0x10);
	size_t len = wcslen(path);
	if (len > KUIN_MAX_PATH)
	{
		FileDialogDir[0] = L'\0';
		return;
	}
	for (size_t i = 0; i < len; i++)
		FileDialogDir[i] = path[i] == L'/' ? L'\\' : path[i];
	FileDialogDir[len] = L'\0';
}

EXPORT_CPP S64 _colorDialog(SClass* parent, S64 default_color)
{
	CHOOSECOLOR choose_color = { 0 };
	choose_color.lStructSize = sizeof(CHOOSECOLOR);
	choose_color.hwndOwner = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	choose_color.rgbResult = static_cast<COLORREF>(((default_color & 0xff) << 16) | (default_color & 0xff00) | ((default_color & 0xff0000) >> 16));
	choose_color.Flags = CC_FULLOPEN | CC_RGBINIT;

	DWORD colors[16];
	for (int i = 0; i < 16; i++)
		colors[i] = 0xffffff;
	choose_color.lpCustColors = colors;

	if (ChooseColor(&choose_color))
	{
		S64 result = static_cast<S64>(choose_color.rgbResult);
		return ((result & 0xff) << 16) | (result & 0xff00) | ((result & 0xff0000) >> 16);
	}
	return -1;
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
				buf++;
			}
			*buf = *ptr;
			buf++;
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

EXPORT_CPP void _getCaretPos(S64* x, S64* y)
{
	POINT point;
	if (!GetCaretPos(&point))
	{
		*x = -1;
		*y = -1;
	}
	else
	{
		*x = static_cast<S64>(point.x);
		*y = static_cast<S64>(point.y);
	}
}

EXPORT_CPP void _screenSize(S64* width, S64* height)
{
	*width = static_cast<S64>(GetSystemMetrics(SM_CXSCREEN));
	*height = static_cast<S64>(GetSystemMetrics(SM_CYSCREEN));
}

EXPORT_CPP void _target(SClass* draw_ctrl)
{
	SDraw* draw_ctrl2 = reinterpret_cast<SDraw*>(draw_ctrl);
	Draw::ActiveDrawBuf(draw_ctrl2->DrawBuf);
}

EXPORT_CPP Bool _key(S64 key)
{
	return (GetKeyState(static_cast<int>(key)) & 0x8000) != 0;
}

EXPORT_CPP SClass* _makeWnd(SClass* me_, SClass* parent, S64 style, S64 width, S64 height, const U8* text)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	me2->Kind = static_cast<EWndKind>(static_cast<S64>(WndKind_WndNormal) + (style & 0xffff));
	THROWDBG(width <= 0 || height <= 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	int width2 = static_cast<int>(width);
	int height2 = static_cast<int>(height);
	HWND parent2 = parent == NULL ? NULL : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	DWORD ex_style = 0;
	if ((style & static_cast<S64>(WndKind_WndLayered)) != 0)
		ex_style |= WS_EX_LAYERED;
	switch (me2->Kind)
	{
		case WndKind_WndNormal:
			me2->WndHandle = CreateWindowEx(ex_style, L"KuinWndNormalClass", text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		case WndKind_WndFix:
			me2->WndHandle = CreateWindowEx(ex_style, L"KuinWndFixClass", text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		case WndKind_WndAspect:
			me2->WndHandle = CreateWindowEx(ex_style, L"KuinWndAspectClass", text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), (WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		case WndKind_WndPopup:
			me2->WndHandle = CreateWindowEx(ex_style | WS_EX_TOOLWINDOW, L"KuinWndDialogClass", text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_POPUP | WS_BORDER | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		case WndKind_WndDialog:
			me2->WndHandle = CreateWindowEx(ex_style | WS_EX_TOOLWINDOW, L"KuinWndDialogClass", text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, NULL, Instance, NULL);
			break;
		default:
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
			break;
	}
	if (me2->WndHandle == NULL)
		THROW(EXCPT_DEVICE_INIT_FAILED);
	if ((style & static_cast<S64>(WndKind_WndLayered)) != 0)
		SetLayeredWindowAttributes(me2->WndHandle, NULL, 255, LWA_ALPHA);
	if ((style & static_cast<S64>(WndKind_WndNoMinimize)) != 0)
		SetWindowLongPtr(me2->WndHandle, GWL_STYLE, GetWindowLongPtr(me2->WndHandle, GWL_STYLE) & ~WS_MINIMIZEBOX);
	int border_x;
	int border_y;
	{
		RECT window;
		RECT client;
		GetWindowRect(me2->WndHandle, &window);
		GetClientRect(me2->WndHandle, &client);
		border_x = static_cast<int>((window.right - window.left) - (client.right - client.left));
		border_y = static_cast<int>((window.bottom - window.top) - (client.bottom - client.top));
	}
	me2->Name = NULL;
	me2->DefaultWndProc = NULL;
	me2->CtrlFlag = static_cast<U64>(CtrlFlag_AnchorLeft) | static_cast<U64>(CtrlFlag_AnchorTop);
	me2->DefaultX = 0;
	me2->DefaultY = 0;
	me2->DefaultWidth = static_cast<U16>(width);
	me2->DefaultHeight = static_cast<U16>(height);
	me2->RedrawEnabled = True;
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	SetWindowPos(me2->WndHandle, NULL, 0, 0, static_cast<int>(width) + border_x, static_cast<int>(height) + border_y, SWP_NOMOVE | SWP_NOZORDER);
	if (me2->Kind == WndKind_WndAspect)
	{
		RECT rect;
		GetClientRect(me2->WndHandle, &rect);
		double w = static_cast<double>(rect.right) - static_cast<double>(rect.left);
		double h = static_cast<double>(rect.bottom) - static_cast<double>(rect.top);
		if (w / h > static_cast<double>(width) / static_cast<double>(height))
			w = h * static_cast<double>(width) / static_cast<double>(height);
		else
			h = w * static_cast<double>(height) / static_cast<double>(width);
		SetWindowPos(me2->WndHandle, NULL, 0, 0, static_cast<int>(w) + border_x, static_cast<int>(h) + border_y, SWP_NOMOVE | SWP_NOZORDER);
	}
	SetWindowLongPtr(me2->WndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me2));
	{
		SWnd* me3 = reinterpret_cast<SWnd*>(me_);
		me3->MinWidth = 128;
		me3->MinHeight = 128;
		me3->MaxWidth = static_cast<U16>(-1);
		me3->MaxHeight = static_cast<U16>(-1);
		me3->OnClose = NULL;
		me3->OnActivate = NULL;
		me3->OnPushMenu = NULL;
		me3->OnDropFiles = NULL;
		me3->OnResize = NULL;
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

EXPORT_CPP void _wndBaseGetPos(SClass* me_, S64* x, S64* y, S64* width, S64* height)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	RECT rect;
	GetClientRect(me2->WndHandle, &rect);
	*x = static_cast<S64>(rect.left);
	*y = static_cast<S64>(rect.top);
	*width = static_cast<S64>(rect.right - rect.left);
	*height = static_cast<S64>(rect.bottom - rect.top);
}

EXPORT_CPP void _wndBaseGetPosScreen(SClass* me_, S64* x, S64* y, S64* width, S64* height)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	RECT rect;
	GetWindowRect(me2->WndHandle, &rect);
	*x = static_cast<S64>(rect.left);
	*y = static_cast<S64>(rect.top);
	*width = static_cast<S64>(rect.right - rect.left);
	*height = static_cast<S64>(rect.bottom - rect.top);
}

EXPORT_CPP void _wndBaseFocus(SClass* me_)
{
	SetFocus(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP Bool _wndBaseFocused(SClass* me_)
{
	return GetFocus() == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP void _wndBaseSetEnabled(SClass* me_, Bool is_enabled)
{
	EnableWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_enabled ? TRUE : FALSE);
}

EXPORT_CPP Bool _wndBaseGetEnabled(SClass* me_)
{
	return IsWindowEnabled(reinterpret_cast<SWndBase*>(me_)->WndHandle) != FALSE;
}

EXPORT_CPP void _wndBaseSetPos(SClass* me_, S64 x, S64 y, S64 width, S64 height)
{
	SetWindowPos(reinterpret_cast<SWndBase*>(me_)->WndHandle, NULL, (int)x, (int)y, (int)width, (int)height, SWP_NOZORDER);
}

EXPORT_CPP void _wndBaseSetRedraw(SClass* me_, Bool is_enabled)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	HWND wnd = me2->WndHandle;
	if (me2->RedrawEnabled != is_enabled)
	{
		me2->RedrawEnabled = is_enabled;
		if (is_enabled)
		{
			SendMessage(wnd, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(wnd, NULL, TRUE);
		}
		else
			SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
	}
}

EXPORT_CPP void _wndBaseSetVisible(SClass* me_, Bool is_visible)
{
	ShowWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_visible ? SW_SHOW : SW_HIDE);
}

EXPORT_CPP Bool _wndBaseGetVisible(SClass* me_)
{
	return IsWindowVisible(reinterpret_cast<SWndBase*>(me_)->WndHandle) != FALSE;
}

EXPORT_CPP void _wndBaseClientToScreen(SClass* me_, S64* screenX, S64* screenY, S64 clientX, S64 clientY)
{
	POINT point;
	point.x = static_cast<LONG>(clientX);
	point.y = static_cast<LONG>(clientY);
	ClientToScreen(reinterpret_cast<SWndBase*>(me_)->WndHandle, &point);
	*screenX = static_cast<S64>(point.x);
	*screenY = static_cast<S64>(point.y);
}

EXPORT_CPP void _wndBaseScreenToClient(SClass* me_, S64* clientX, S64* clientY, S64 screenX, S64 screenY)
{
	POINT point;
	point.x = static_cast<LONG>(screenX);
	point.y = static_cast<LONG>(screenY);
	ScreenToClient(reinterpret_cast<SWndBase*>(me_)->WndHandle, &point);
	*clientX = static_cast<S64>(point.x);
	*clientY = static_cast<S64>(point.y);
}

EXPORT_CPP void _wndMinMax(SClass* me_, S64 minWidth, S64 minHeight, S64 maxWidth, S64 maxHeight)
{
	THROWDBG(minWidth != -1 && minWidth <= 0 || minHeight != -1 && minHeight <= 0 || maxWidth != -1 && maxWidth < minWidth || maxHeight != -1 && maxHeight < minHeight, EXCPT_DBG_ARG_OUT_DOMAIN);
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
	const U8* text2 = NToRN(text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10));
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

EXPORT_CPP void _wndSetMenu(SClass* me_, SClass* menu)
{
	SetMenu(reinterpret_cast<SWndBase*>(me_)->WndHandle, menu == NULL ? NULL : reinterpret_cast<SMenu*>(menu)->MenuHandle);
}

EXPORT_CPP void _wndActivate(SClass* me_)
{
	SetActiveWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP Bool _wndActivated(SClass* me_)
{
	return GetActiveWindow() == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP Bool _wndFocusedWnd(SClass* me_)
{
	HWND wnd = GetFocus();
	if (wnd == NULL)
		return False;
	SWndBase* wnd2 = ToWnd(wnd);
	while (wnd2 != NULL && static_cast<int>(wnd2->Kind) >= 0x80)
	{
		wnd = GetParent(wnd);
		if (wnd == NULL)
			return False;
		wnd2 = ToWnd(wnd);
	}
	if (wnd2 == NULL)
		return False;
	return wnd == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP void _wndSetAlpha(SClass* me_, S64 alpha)
{
	THROWDBG(alpha < 0 || 255 < alpha, EXCPT_DBG_ARG_OUT_DOMAIN);
	SetLayeredWindowAttributes(reinterpret_cast<SWndBase*>(me_)->WndHandle, NULL, static_cast<BYTE>(alpha), LWA_ALPHA);
}

EXPORT_CPP S64 _wndGetAlpha(SClass* me_)
{
	BYTE alpha;
	GetLayeredWindowAttributes(reinterpret_cast<SWndBase*>(me_)->WndHandle, NULL, &alpha, NULL);
	return static_cast<S64>(alpha);
}

EXPORT_CPP void _wndAcceptDraggedFiles(SClass* me_, Bool is_accepted)
{
	DragAcceptFiles(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_accepted ? TRUE : FALSE);
}

EXPORT_CPP void _wndUpdateMenu(SClass* me_)
{
	DrawMenuBar(reinterpret_cast<SWndBase*>(me_)->WndHandle);
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
	return MakeDrawImpl(me_, parent, x, y, width, height, anchorX, anchorY, equalMagnification, False, 1);
}

EXPORT_CPP SClass* _makeDrawReduced(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, Bool equalMagnification, S64 split)
{
	return MakeDrawImpl(me_, parent, x, y, width, height, anchorX, anchorY, equalMagnification, False, static_cast<int>(split));
}

EXPORT_CPP SClass* _makeDrawEditable(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height)
{
	return MakeDrawImpl(me_, parent, x, y, width, height, 0, 0, False, True, 1);
}

EXPORT_CPP void _drawDtor(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	SDraw* me3 = reinterpret_cast<SDraw*>(me_);
	if (me3->DrawBuf != NULL)
		Draw::FinDrawBuf(me3->DrawBuf);
	DestroyWindow(me2->WndHandle);
}

EXPORT_CPP void _drawPaint(SClass* me_)
{
	InvalidateRect(reinterpret_cast<SWndBase*>(me_)->WndHandle, NULL, FALSE);
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

EXPORT_CPP void _drawMouseCapture(SClass* me_, Bool enabled)
{
	if (enabled)
		SetCapture(reinterpret_cast<SWndBase*>(me_)->WndHandle);
	else
		ReleaseCapture();
}

EXPORT_CPP SClass* _makeBtn(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SBtn* me2 = reinterpret_cast<SBtn*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Btn, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, width, height, text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcBtn, anchorX, anchorY);
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
	SChk* me2 = reinterpret_cast<SChk*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Chk, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, width, height, text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcChk, anchorX, anchorY);
	me2->OnPush = NULL;
	return me_;
}

EXPORT_CPP SClass* _makeRadio(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SRadio* me2 = reinterpret_cast<SRadio*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Radio, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON, x, y, width, height, text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcRadio, anchorX, anchorY);
	me2->OnPush = NULL;
	return me_;
}

EXPORT_CPP SClass* _makeEdit(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SEdit* me2 = reinterpret_cast<SEdit*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Edit, WC_EDIT, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NOHIDESEL, x, y, width, height, L"", WndProcEdit, anchorX, anchorY);
	reinterpret_cast<SEditBase*>(me2)->OnChange = NULL;
	reinterpret_cast<SEditBase*>(me2)->OnFocus = NULL;
	return me_;
}

EXPORT_CPP void _editReadonly(SClass* me_, Bool enabled)
{
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	SendMessage(wnd, EM_SETREADONLY, enabled ? TRUE : FALSE, 0);
}

EXPORT_CPP void _editRightAligned(SClass* me_, Bool enabled)
{
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	DWORD dw_style = GetWindowLong(wnd, GWL_STYLE);
	if(enabled)
		SetWindowLong(wnd, GWL_STYLE, dw_style | ES_RIGHT);
	else
		SetWindowLong(wnd, GWL_STYLE, dw_style & ~ES_RIGHT);
	InvalidateRect(wnd, NULL, TRUE);
}

EXPORT_CPP void _editSetSel(SClass* me_, S64 start, S64 len)
{
	THROWDBG(!(len == -1 && (start == -1 || start == 0) || 0 <= start && 0 <= len), EXCPT_DBG_ARG_OUT_DOMAIN);
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	S64 first;
	S64 last;
	if (len == -1)
	{
		if (start == -1)
		{
			first = -1;
			last = -1;
		}
		else
		{
			first = 0;
			last = -1;
		}
	}
	else
	{
		first = start;
		last = start + len;
	}
	SendMessage(me2->WndHandle, EM_SETSEL, static_cast<WPARAM>(first), static_cast<LPARAM>(last));
}

EXPORT_CPP SClass* _makeEditMulti(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SEditMulti* me2 = reinterpret_cast<SEditMulti*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_EditMulti, WC_EDIT, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL, x, y, width, height, L"", WndProcEditMulti, anchorX, anchorY);
	reinterpret_cast<SEditBase*>(me2)->OnChange = NULL;
	reinterpret_cast<SEditBase*>(me2)->OnFocus = NULL;
	return me_;
}

EXPORT_CPP void _editMultiAddText(SClass* me_, const U8* text)
{
	const U8* text2 = NToRN(text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10));
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	HWND wnd = me2->WndHandle;
	Bool redraw_enabled = me2->RedrawEnabled;
	if (redraw_enabled)
		SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
	SendMessage(wnd, EM_SETSEL, 0, static_cast<LPARAM>(-1)); // Select all.
	SendMessage(wnd, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1)); // Unselect.
	SendMessage(wnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(wnd, NULL, TRUE);
	SendMessage(wnd, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(const_cast<U8*>(text2 + 0x10)));
	if (!redraw_enabled)
		SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
	FreeMem(const_cast<U8*>(text2));
}

EXPORT_CPP SClass* _makeList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SList* me2 = reinterpret_cast<SList*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_List, WC_LISTBOX, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_DISABLENOSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, x, y, width, height, L"", WndProcList, anchorX, anchorY);
	me2->OnSel = NULL;
	me2->OnMouseDoubleClick = NULL;
	return me_;
}

EXPORT_CPP void _listClear(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_RESETCONTENT, 0, 0);
}

EXPORT_CPP void _listAdd(SClass* me_, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _listIns(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _listDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
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
	S64 len = _listLen(me_);
	THROWDBG(idx < -1 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _listGetSel(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCURSEL, 0, 0));
}

EXPORT_CPP void _listSetText(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	{
		int sel = static_cast<int>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCURSEL, 0, 0));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_DELETESTRING, static_cast<WPARAM>(idx + 1), 0);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_SETCURSEL, static_cast<WPARAM>(sel), 0);
	}
}

EXPORT_CPP void* _listGetText(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _listLen(me_);
		THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
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

EXPORT_CPP SClass* _makeCombo(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Combo, WC_COMBOBOX, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST, x, y, width, height, L"", WndProcCombo, anchorX, anchorY);
	return me_;
}

EXPORT_CPP void _comboClear(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_RESETCONTENT, 0, 0);
}

EXPORT_CPP void _comboAdd(SClass* me_, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _comboIns(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _comboDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_DELETESTRING, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _comboLen(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETCOUNT, 0, 0));
}

EXPORT_CPP void _comboSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < -1 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _comboGetSel(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETCURSEL, 0, 0));
}

EXPORT_CPP void _comboSetText(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	{
		int sel = static_cast<int>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETCURSEL, 0, 0));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_DELETESTRING, static_cast<WPARAM>(idx + 1), 0);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_SETCURSEL, static_cast<WPARAM>(sel), 0);
	}
}

EXPORT_CPP void* _comboGetText(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _comboLen(me_);
		THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
	}
#endif
	{
		size_t len = static_cast<size_t>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETLBTEXTLEN, static_cast<WPARAM>(idx), 0));
		U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
		*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
		*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETLBTEXT, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(result + 0x10));
		return result;
	}
}

EXPORT_CPP SClass* _makeLabel(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Label, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_SIMPLE, x, y, width, height, text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcLabel, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeGroup(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Group, WC_BUTTON, WS_EX_TRANSPARENT, WS_VISIBLE | WS_CHILD | BS_GROUPBOX, x, y, width, height, text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcGroup, anchorX, anchorY);
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

EXPORT_CPP SClass* _makeListView(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, Bool multi_sel, const void* small_imgs, const void* large_imgs)
{
	SListView* me2 = reinterpret_cast<SListView*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_ListView, WC_LISTVIEW, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_NOSORTHEADER | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | (multi_sel ? 0 : LVS_SINGLESEL), x, y, width, height, L"", WndProcListView, anchorX, anchorY);
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	DWORD ex = ListView_GetExtendedListViewStyle(wnd);
	ListView_SetExtendedListViewStyle(wnd, ex | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_SUBITEMIMAGES);
	SetWindowLongPtr(ListView_GetHeader(wnd), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me_));
	me2->OnSel = NULL;
	me2->OnMouseDoubleClick = NULL;
	me2->OnMouseClick = NULL;
	me2->OnMoveNode = NULL;
	me2->Draggable = False;
	me2->Dragging = False;
	me2->DraggingImage = NULL;

	if (small_imgs != NULL)
		ListView_SetImageList(wnd, CreateImageList(small_imgs), LVSIL_SMALL);
	if (large_imgs != NULL)
		ListView_SetImageList(wnd, CreateImageList(large_imgs), LVSIL_NORMAL);

	return me_;
}

EXPORT_CPP void _listViewClear(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	ListView_DeleteAllItems(me2->WndHandle);
}

EXPORT_CPP void _listViewAdd(SClass* me_, const U8* text, S64 img)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(img < -1, EXCPT_DBG_ARG_OUT_DOMAIN);
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = ListView_GetItemCount(wnd);
	item.iSubItem = 0;
	item.iImage = static_cast<int>(img);
	item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertItem(wnd, &item);
}

EXPORT_CPP void _listViewIns(SClass* me_, S64 idx, const U8* text, S64 img)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(img < -1, EXCPT_DBG_ARG_OUT_DOMAIN);
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = static_cast<int>(idx);
	item.iSubItem = 0;
	item.iImage = static_cast<int>(img);
	item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, &item);
}

EXPORT_CPP void _listViewDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	ListView_DeleteItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx));
}

EXPORT_CPP S64 _listViewLen(SClass* me_)
{
	return static_cast<S64>(ListView_GetItemCount(reinterpret_cast<SWndBase*>(me_)->WndHandle));
}

EXPORT_CPP void _listViewAddColumn(SClass* me_, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	LVCOLUMN lvcolumn;
	lvcolumn.mask = LVCF_TEXT;
	lvcolumn.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertColumn(wnd, Header_GetItemCount(ListView_GetHeader(wnd)), &lvcolumn);
	ListViewAdjustWidth(wnd);
}

EXPORT_CPP void _listViewInsColumn(SClass* me_, S64 column, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	LVCOLUMN lvcolumn;
	lvcolumn.mask = LVCF_TEXT;
	lvcolumn.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertColumn(wnd, static_cast<int>(column), &lvcolumn);
	ListViewAdjustWidth(wnd);
}

EXPORT_CPP void _listViewDelColumn(SClass* me_, S64 column)
{
#if defined(DBG)
	S64 len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	ListView_DeleteColumn(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(column));
}

EXPORT_CPP S64 _listViewLenColumn(SClass* me_)
{
	S64 len = static_cast<S64>(Header_GetItemCount(ListView_GetHeader(reinterpret_cast<SWndBase*>(me_)->WndHandle)));
	if (len == 0)
		len = 1;
	return len;
}

EXPORT_CPP void _listViewClearAll(SClass* me_)
{
	S64 len = _listViewLenColumn(me_);
	for (S64 i = 0; i < len; i++)
		_listViewDelColumn(me_, 0);
	_listViewClear(me_);
}

EXPORT_CPP void _listViewSetText(SClass* me_, S64 idx, S64 column, const U8* text, S64 img)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(img < -1, EXCPT_DBG_ARG_OUT_DOMAIN);
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
	len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = static_cast<int>(idx);
	item.iSubItem = static_cast<int>(column);
	item.iImage = static_cast<int>(img);
	item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_SetItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, &item);
}

EXPORT_CPP void* _listViewGetText(SClass* me_, S64* img, S64 idx, S64 column)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
	len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	Char buf[1025];
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = static_cast<int>(idx);
	item.iSubItem = static_cast<int>(column);
	item.pszText = buf;
	item.cchTextMax = 1025;
	item.iImage = -1;
	ListView_GetItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, &item);
	buf[1024] = L'\0';
	size_t len2 = wcslen(buf);
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len2 + 1)));
	*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len2);
	memcpy(result + 0x10, buf, sizeof(Char) * static_cast<size_t>(len2 + 1));
	*img = static_cast<S64>(item.iImage);
	return result;
}

EXPORT_CPP void _listViewAdjustWidth(SClass* me_)
{
	ListViewAdjustWidth(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP void _listViewSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	ListView_SetItemState(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx), LVIS_SELECTED, LVIS_SELECTED);
}

EXPORT_CPP S64 _listViewGetSel(SClass* me_)
{
	return static_cast<S64>(ListView_GetNextItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, -1, LVNI_ALL | LVNI_SELECTED));
}

EXPORT_CPP void _listViewSetSelMulti(SClass* me_, S64 idx, Bool value)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	ListView_SetItemState(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx), LVIS_SELECTED, value ? LVIS_SELECTED : 0);
}

EXPORT_CPP Bool _listViewGetSelMulti(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	return ListView_GetItemState(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx), LVIS_SELECTED) != 0;
}

EXPORT_CPP void _listViewStyle(SClass* me_, S64 list_view_style)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	DWORD dw_style = GetWindowLong(me2->WndHandle, GWL_STYLE);
	const LONG normal_mask = LVS_TYPEMASK | LVS_NOCOLUMNHEADER;
	SetWindowLong(me2->WndHandle, GWL_STYLE, (dw_style & ~normal_mask) | (static_cast<LONG>(list_view_style) & normal_mask));
	DWORD ex = ListView_GetExtendedListViewStyle(me2->WndHandle);
	const LONG ex_mask = LVS_EX_CHECKBOXES;
	ListView_SetExtendedListViewStyle(me2->WndHandle, (ex & ~ex_mask) | (static_cast<LONG>(list_view_style) & ex_mask));
	InvalidateRect(me2->WndHandle, NULL, TRUE);
}

EXPORT_CPP void _listViewDraggable(SClass* me_, bool enabled)
{
	reinterpret_cast<SListView*>(me_)->Draggable = enabled;
}

EXPORT_CPP void _listViewSetChk(SClass* me_, S64 idx, bool value)
{
	ListView_SetCheckState(reinterpret_cast<SWndBase*>(me_)->WndHandle, idx, value ? TRUE : FALSE);
}

EXPORT_CPP Bool _listViewGetChk(SClass* me_, S64 idx)
{
	return ListView_GetCheckState(reinterpret_cast<SWndBase*>(me_)->WndHandle, idx) != 0;
}

EXPORT_CPP SClass* _makePager(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Pager, WC_PAGESCROLLER, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcPager, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeTab(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Tab, WC_TABCONTROL, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN | TCS_BUTTONS | TCS_FLATBUTTONS, x, y, width, height, L"", WndProcTab, anchorX, anchorY);
	return me_;
}

EXPORT_CPP void _tabAdd(SClass* me_, const U8* text)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	S64 cnt = _tabLen(me_);
	TC_ITEM item = { 0 };
	item.mask = TCIF_TEXT;
	item.pszText = text == NULL ? L"" : reinterpret_cast<const Char*>(text + 0x10);
	SendMessage(me2->WndHandle, TCM_INSERTITEM, static_cast<WPARAM>(cnt), reinterpret_cast<LPARAM>(&item));
}

EXPORT_CPP S64 _tabLen(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	return static_cast<S64>(SendMessage(me2->WndHandle, TCM_GETITEMCOUNT, 0, 0));
}

EXPORT_CPP void _tabSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _tabLen(me_);
		THROWDBG(idx < -1 || len <= idx, EXCPT_DBG_ARG_OUT_DOMAIN);
	}
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, TCM_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _tabGetSel(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	return static_cast<S64>(SendMessage(me2->WndHandle, TCM_GETCURSEL, 0, 0));
}

EXPORT_CPP void _tabGetPosInner(SClass* me_, S64* x, S64* y, S64* width, S64* height)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	RECT rect;
	GetClientRect(me2->WndHandle, &rect);
	SendMessage(me2->WndHandle, TCM_ADJUSTRECT, 0, reinterpret_cast<LPARAM>(&rect));
	*x = static_cast<S64>(rect.left);
	*y = static_cast<S64>(rect.top);
	*width = static_cast<S64>(rect.right - rect.left);
	*height = static_cast<S64>(rect.bottom - rect.top);
}

EXPORT_CPP SClass* _makeTree(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	STree* me2 = reinterpret_cast<STree*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Tree, WC_TREEVIEW, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_LINESATROOT, x, y, width, height, L"", WndProcTree, anchorX, anchorY);
	me2->Draggable = False;
	me2->AllowDraggingToRoot = False;
	me2->DraggingItem = NULL;
	me2->OnSel = NULL;
	me2->OnMoveNode = NULL;
	return me_;
}

EXPORT_CPP void _treeClear(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	TreeView_DeleteAllItems(me2->WndHandle);
}

EXPORT_CPP void _treeExpand(SClass* me_, Bool expand)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	HTREEITEM root = TreeView_GetRoot(me2->WndHandle);
	if (root != NULL)
		TreeExpandAllRecursion(me2->WndHandle, root, expand ? TVE_EXPAND : TVE_COLLAPSE);
}

EXPORT_CPP SClass* _treeRoot(SClass* me_, SClass* me2)
{
	SWndBase* me3 = reinterpret_cast<SWndBase*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	me4->WndHandle = me3->WndHandle;
	me4->Item = NULL;
	return me2;
}

EXPORT_CPP void _treeDraggable(SClass* me_, Bool enabled)
{
	reinterpret_cast<STree*>(me_)->Draggable = enabled;
}

EXPORT_CPP void _treeAllowDraggingToRoot(SClass* me_, Bool enabled)
{
	reinterpret_cast<STree*>(me_)->AllowDraggingToRoot = enabled;
}

EXPORT_CPP void _treeSetSel(SClass* me_, SClass* node)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	STreeNode* node2 = reinterpret_cast<STreeNode*>(node);
	THROWDBG(me2->WndHandle != node2->WndHandle, EXCPT_DBG_ARG_OUT_DOMAIN);
	TreeView_Select(me2->WndHandle, node2->Item, TVGN_CARET);
}

EXPORT_CPP SClass* _treeGetSel(SClass* me_, SClass* me2)
{
	SWndBase* me3 = reinterpret_cast<SWndBase*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	me4->WndHandle = me3->WndHandle;
	me4->Item = TreeView_GetSelection(me3->WndHandle);
	if (me4->Item == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _treeNodeAddChild(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	TVINSERTSTRUCT tvis;
	tvis.hParent = me3->Item == NULL ? TVI_ROOT : me3->Item;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(name + 0x10));
	me4->Item = TreeView_InsertItem(me3->WndHandle, &tvis);
	if (me4->Item == NULL)
		return NULL;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _treeNodeInsChild(SClass* me_, SClass* me2, SClass* node, const U8* name)
{
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	STreeNode* node2 = reinterpret_cast<STreeNode*>(node);
	TVINSERTSTRUCT tvis;
	tvis.hParent = me3->Item == NULL ? TVI_ROOT : me3->Item;
	HTREEITEM prev = TreeView_GetPrevSibling(me3->WndHandle, node2->Item);
	tvis.hInsertAfter = prev == NULL ? TVI_FIRST : prev;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(name + 0x10));
	me4->Item = TreeView_InsertItem(me3->WndHandle, &tvis);
	if (me4->Item == NULL)
		return NULL;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP void _treeNodeDelChild(SClass* me_, SClass* node)
{
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* node2 = reinterpret_cast<STreeNode*>(node);
	if (node2->Item == NULL)
		return;
	if (me3->Item == NULL)
		TreeView_DeleteItem(me3->WndHandle, node2->Item);
}

EXPORT_CPP SClass* _treeNodeFirstChild(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == NULL)
		me4->Item = TreeView_GetRoot(me3->WndHandle);
	else
		me4->Item = TreeView_GetChild(me3->WndHandle, me3->Item);
	if (me4->Item == NULL)
		return NULL;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP void* _treeNodeGetName(SClass* me_)
{
	STreeNode* me2 = reinterpret_cast<STreeNode*>(me_);
	if (me2->Item == NULL)
		return NULL;
	TVITEM ti;
	Char buf[1025];
	ti.mask = TVIF_TEXT;
	ti.hItem = me2->Item;
	ti.pszText = buf;
	ti.cchTextMax = 1025;
	if (!TreeView_GetItem(me2->WndHandle, &ti))
		buf[0] = L'\0';
	else
		buf[1024] = L'\0';
	size_t len = wcslen(buf);
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len + 1)));
	*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
	memcpy(result + 0x10, buf, sizeof(Char) * static_cast<size_t>(len + 1));
	return result;
}

EXPORT_CPP SClass* _treeNodeNext(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == NULL)
		return NULL;
	me4->Item = TreeView_GetNextSibling(me3->WndHandle, me3->Item);
	if (me4->Item == NULL)
		return NULL;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _treeNodePrev(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == NULL)
		return NULL;
	me4->Item = TreeView_GetPrevSibling(me3->WndHandle, me3->Item);
	if (me4->Item == NULL)
		return NULL;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _treeNodeParent(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == NULL)
		return NULL;
	me4->Item = TreeView_GetParent(me3->WndHandle, me3->Item);
	if (me4->Item == NULL)
		return NULL;
	me4->WndHandle = me3->WndHandle;
	return me2;
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

EXPORT_CPP void _scrollSetState(SClass* me_, S64 min, S64 max, S64 page, S64 pos)
{
	if (max < min)
		max = min;
	if (page < 1)
		page = 1;
	if (pos < min)
		pos = min;
	if (pos > max)
		pos = max;
	SCROLLINFO info;
	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	info.nMin = static_cast<int>(min);
	info.nMax = static_cast<int>(max);
	info.nPage = static_cast<int>(page);
	info.nPos = static_cast<int>(pos);
	info.nTrackPos = 0;
	SetScrollInfo(reinterpret_cast<SWndBase*>(me_)->WndHandle, SB_CTL, &info, TRUE);
}

EXPORT_CPP void _scrollSetScrollPos(SClass* me_, S64 pos)
{
	SCROLLINFO info;
	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_POS | SIF_DISABLENOSCROLL;
	info.nMin = 0;
	info.nMax = 0;
	info.nPage = 0;
	info.nPos = static_cast<int>(pos);
	info.nTrackPos = 0;
	SetScrollInfo(reinterpret_cast<SWndBase*>(me_)->WndHandle, SB_CTL, &info, TRUE);
}

EXPORT_CPP SClass* _makeMenu(SClass* me_)
{
	SMenu* me2 = reinterpret_cast<SMenu*>(me_);
	me2->MenuHandle = CreateMenu();
	if (me2->MenuHandle == NULL)
		THROW(EXCPT_DEVICE_INIT_FAILED);
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
	THROWDBG(id < 0x0001 || 0xffff < id, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_STRING, static_cast<UINT_PTR>(id), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuAddLine(SClass* me_)
{
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_SEPARATOR, 0, NULL);
}

EXPORT_CPP void _menuAddPopup(SClass* me_, const U8* text, const U8* popup)
{
	THROWDBG(popup == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuIns(SClass* me_, S64 targetId, S64 id, const U8* text)
{
	THROWDBG(targetId < 0x0001 || 0xffff < targetId, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(id < 0x0001 || 0xffff < id, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	InsertMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(targetId), MF_ENABLED | MF_STRING, static_cast<UINT_PTR>(id), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuInsPopup(SClass* me_, const U8* target, const U8* text, const U8* popup)
{
	THROWDBG(target == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(popup == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	// This cast is due to bad specifications of Windows API.
	InsertMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(target)->MenuHandle)), MF_ENABLED | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuDel(SClass* me_, S64 id)
{
	THROWDBG(id < 0x0001 || 0xffff < id, EXCPT_DBG_ARG_OUT_DOMAIN);
	RemoveMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(id), MF_BYCOMMAND);
}

EXPORT_CPP void _menuDelPopup(SClass* me_, const U8* popup)
{
	THROWDBG(popup == NULL, EXCPT_ACCESS_VIOLATION);
	// This cast is due to bad specifications of Windows API.
	RemoveMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle)), MF_BYCOMMAND);
}

EXPORT_CPP SClass* _makePopup(SClass* me_)
{
	SMenu* me2 = reinterpret_cast<SMenu*>(me_);
	me2->MenuHandle = CreatePopupMenu();
	if (me2->MenuHandle == NULL)
		THROW(EXCPT_DEVICE_INIT_FAILED);
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	return me_;
}

EXPORT_CPP SClass* _makeTabOrder(SClass* me_, U8* ctrls)
{
	STabOrder* me2 = reinterpret_cast<STabOrder*>(me_);
	THROWDBG(ctrls == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *reinterpret_cast<S64*>(ctrls + 0x08);
	void** ptr = reinterpret_cast<void**>(ctrls + 0x10);
	void* result = AllocMem(0x10 + sizeof(void*) * static_cast<size_t>(len));
	static_cast<S64*>(result)[0] = 1;
	static_cast<S64*>(result)[1] = len;
	void** result2 = reinterpret_cast<void**>(static_cast<U8*>(result) + 0x10);
	for (S64 i = 0; i < len; i++)
	{
		if (ptr[i] == NULL)
			result2[i] = NULL;
		else
		{
			(*static_cast<S64*>(ptr[i]))++;
			result2[i] = ptr[i];
		}
	}
	me2->Ctrls = result;
	return me_;
}

EXPORT_CPP Bool _tabOrderChk(SClass* me_, S64 key, S64 shiftCtrl)
{
	if (key != VK_TAB || (shiftCtrl & 0x02) != 0)
		return False;
	STabOrder* me2 = reinterpret_cast<STabOrder*>(me_);
	U8* ctrls = static_cast<U8*>(me2->Ctrls);
	S64 len = *reinterpret_cast<S64*>(ctrls + 0x08);
	if (len == 0)
		return False;
	HWND wnd = GetFocus();
	void** ptr = reinterpret_cast<void**>(ctrls + 0x10);
	for (S64 i = 0; i < len; i++)
	{
		SWndBase* wnd2 = static_cast<SWndBase*>(ptr[i]);
		if (wnd2->WndHandle == wnd)
		{
			S64 step = shiftCtrl == 0 ? 1 : len - 1;
			S64 idx = (i + step) % len;
			while (idx != i)
			{
				if (ptr[idx] != NULL)
				{
					SWndBase* wnd3 = static_cast<SWndBase*>(ptr[idx]);
					if (IsWindowEnabled(wnd3->WndHandle))
					{
						SetFocus(wnd3->WndHandle);
						return True;
					}
				}
				idx = (idx + step) % len;
			}
			break;
		}
	}
	return False;
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

static void ParseAnchor(SWndBase* wnd, S64 anchor_x, S64 anchor_y, S64 x, S64 y, S64 width, S64 height)
{
	THROWDBG(x != static_cast<S64>(static_cast<U16>(x)) || y != static_cast<S64>(static_cast<U16>(y)) || width != static_cast<S64>(static_cast<U16>(width)) || height != static_cast<S64>(static_cast<U16>(height)), EXCPT_DBG_ARG_OUT_DOMAIN);
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
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
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
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
			break;
	}
	wnd->DefaultX = static_cast<U16>(x);
	wnd->DefaultY = static_cast<U16>(y);
	wnd->DefaultWidth = static_cast<U16>(width);
	wnd->DefaultHeight = static_cast<U16>(height);
	ResizeCallback(wnd->WndHandle, NULL);
}

static SWndBase* ToWnd(HWND wnd)
{
	return reinterpret_cast<SWndBase*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
}

static void SetCtrlParam(SWndBase* wnd, SWndBase* parent, EWndKind kind, const Char* ctrl, DWORD style_ex, DWORD style, S64 x, S64 y, S64 width, S64 height, const Char* text, WNDPROC wnd_proc, S64 anchor_x, S64 anchor_y)
{
	THROWDBG(parent == NULL, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(x < 0 || y < 0 || width < 0 || height < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	wnd->Kind = kind;
	wnd->WndHandle = CreateWindowEx(style_ex, ctrl, text, style, static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), parent->WndHandle, NULL, Instance, NULL);
	if (wnd->WndHandle == NULL)
		THROW(EXCPT_DEVICE_INIT_FAILED);
	SetWindowLongPtr(wnd->WndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
	wnd->Name = NULL;
	wnd->DefaultWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(wnd->WndHandle, GWLP_WNDPROC));
	wnd->RedrawEnabled = True;
	wnd->Children = AllocMem(0x28);
	*(S64*)wnd->Children = 1;
	memset((U8*)wnd->Children + 0x08, 0x00, 0x20);
	SetWindowLongPtr(wnd->WndHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc));
	ParseAnchor(wnd, anchor_x, anchor_y, x, y, width, height);
	SendMessage(wnd->WndHandle, WM_SETFONT, reinterpret_cast<WPARAM>(FontCtrl), static_cast<LPARAM>(FALSE));
}

static BOOL CALLBACK ResizeCallback(HWND wnd, LPARAM l_param)
{
	UNUSED(l_param);
	SWndBase* wnd2 = ToWnd(wnd);
	if (wnd2->CtrlFlag == (static_cast<U64>(CtrlFlag_AnchorLeft) | static_cast<U64>(CtrlFlag_AnchorTop)))
		return TRUE;
	RECT parent_rect = { 0 };
	HWND parent_handle = GetAncestor(wnd, GA_PARENT);
	SWndBase* parent = ToWnd(parent_handle);
	GetClientRect(parent_handle, &parent_rect);
	const int parent_width_diff = static_cast<int>(parent_rect.right - parent_rect.left) - static_cast<int>(parent->DefaultWidth);
	const int parent_height_diff = static_cast<int>(parent_rect.bottom - parent_rect.top) - static_cast<int>(parent->DefaultHeight);
	int new_x;
	int new_y;
	int new_width;
	int new_height;
	if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorLeft)) == 0)
	{
		new_x = static_cast<int>(wnd2->DefaultX) + parent_width_diff;
		ASSERT((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorRight)) != 0);
		new_width = static_cast<int>(wnd2->DefaultWidth); // move
	}
	else
	{
		new_x = static_cast<int>(wnd2->DefaultX);
		if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorRight)) == 0)
			new_width = static_cast<int>(wnd2->DefaultWidth); // fix
		else
			new_width = static_cast<int>(wnd2->DefaultWidth) + parent_width_diff; // scale
	}
	if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorTop)) == 0)
	{
		new_y = static_cast<int>(wnd2->DefaultY) + parent_height_diff;
		ASSERT((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorBottom)) != 0);
		new_height = static_cast<int>(wnd2->DefaultHeight); // move
	}
	else
	{
		new_y = static_cast<int>(wnd2->DefaultY);
		if ((wnd2->CtrlFlag & static_cast<U64>(CtrlFlag_AnchorBottom)) == 0)
			new_height = static_cast<int>(wnd2->DefaultHeight); // fix
		else
			new_height = static_cast<int>(wnd2->DefaultHeight) + parent_height_diff; // scale
	}
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
			case WndKind_Chk:
				{
					SChk* chk = reinterpret_cast<SChk*>(wnd_ctrl2);
					switch (HIWORD(w_param))
					{
						case BN_CLICKED:
							if (chk->OnPush != NULL)
								Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), chk->OnPush);
							break;
					}
				}
				break;
			case WndKind_Radio:
				{
					SRadio* radio = reinterpret_cast<SRadio*>(wnd_ctrl2);
					switch (HIWORD(w_param))
					{
						case BN_CLICKED:
							if (radio->OnPush != NULL)
								Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), radio->OnPush);
							break;
					}
				}
				break;
			case WndKind_Edit:
			case WndKind_EditMulti:
				{
					SEdit* edit = reinterpret_cast<SEdit*>(wnd_ctrl2);
					switch (HIWORD(w_param))
					{
						case EN_CHANGE:
							if (reinterpret_cast<SEditBase*>(edit)->OnChange != NULL)
								Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), reinterpret_cast<SEditBase*>(edit)->OnChange);
							break;
						case EN_HSCROLL:
							// TODO:
							break;
						case EN_KILLFOCUS:
							if (reinterpret_cast<SEditBase*>(edit)->OnFocus != NULL)
								Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), reinterpret_cast<void*>(static_cast<U64>(False)), reinterpret_cast<SEditBase*>(edit)->OnFocus);
							break;
						case EN_SETFOCUS:
							if (reinterpret_cast<SEditBase*>(edit)->OnFocus != NULL)
								Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), reinterpret_cast<void*>(static_cast<U64>(True)), reinterpret_cast<SEditBase*>(edit)->OnFocus);
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
								if (tab->OnSel != NULL)
									Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), tab->OnSel);
								break;
							case TCN_SELCHANGING:
								// TODO:
								break;
						}
					}
					break;
				case WndKind_Tree:
					{
						STree* tree = reinterpret_cast<STree*>(wnd_ctrl2);
						switch (reinterpret_cast<LPNMHDR>(l_param)->code)
						{
							case TVN_BEGINDRAG:
								if (tree->Draggable)
								{
									LPNMTREEVIEW param = reinterpret_cast<LPNMTREEVIEW>(l_param);
									HIMAGELIST img_drag = TreeView_CreateDragImage(wnd_ctrl, param->itemNew.hItem);
									ImageList_BeginDrag(img_drag, 0, 0, 0);
									ClientToScreen(wnd_ctrl, &param->ptDrag);
									ImageList_DragEnter(NULL, param->ptDrag.x, param->ptDrag.y);
									tree->DraggingItem = param->itemNew.hItem;
								}
								break;
							case TVN_SELCHANGED:
								if (tree->OnSel != NULL)
									Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), tree->OnSel);
								break;
						}
					}
					break;
				case WndKind_ListView:
					{
						SListView* list_view = reinterpret_cast<SListView*>(wnd_ctrl2);
						switch (reinterpret_cast<LPNMHDR>(l_param)->code)
						{
							case LVN_ITEMCHANGED:
								if (list_view->OnSel != NULL)
								{
									NMLISTVIEW* param = reinterpret_cast<NMLISTVIEW*>(l_param);
									if ((param->uOldState & LVIS_SELECTED) != (param->uNewState & LVIS_SELECTED))
										Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), list_view->OnSel);
								}
								break;
							case NM_CLICK:
								if (list_view->OnMouseClick != NULL)
									Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), list_view->OnMouseClick);
								break;
							case LVN_BEGINDRAG:
								if (list_view->Draggable)
								{
									NMLISTVIEW* param = reinterpret_cast<NMLISTVIEW*>(l_param);
									POINT pt_pos = { 0 };
									list_view->DraggingImage = ListView_CreateDragImage(wnd_ctrl, param->iItem, &pt_pos);
									POINT pt;
									GetCursorPos(&pt);
									ScreenToClient(wnd_ctrl, &pt);
									ImageList_BeginDrag(list_view->DraggingImage, 0, pt.x - pt_pos.x, pt.y - pt_pos.y);
									ImageList_DragEnter(wnd_ctrl, 0, 0);
									SetCapture(wnd_ctrl);
									list_view->Dragging = True;
								}
								break;
						}
					}
					break;
			}
		}
	}
}

static Char* ParseFilter(const U8* filter, int* num)
{
	if (filter == NULL)
	{
		*num = 0;
		return NULL;
	}
	S64 len_parent = *reinterpret_cast<const S64*>(filter + 0x08);
	THROWDBG(len_parent % 2 != 0, EXCPT_DBG_ARG_OUT_DOMAIN);
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
	*num = (int)(len_parent / 2);
	return result;
}

static void TreeExpandAllRecursion(HWND wnd_handle, HTREEITEM node, int flag)
{
	TreeView_Expand(wnd_handle, node, flag);

	HTREEITEM child = TreeView_GetChild(wnd_handle, node);
	while (child != NULL)
	{
		TreeExpandAllRecursion(wnd_handle, child, flag);
		child = TreeView_GetNextSibling(wnd_handle, node);
	}
}

static void CopyTreeNodeRecursion(HWND tree_wnd, HTREEITEM dst, HTREEITEM src, Char* buf)
{
	TVITEM tvitem;
	tvitem.mask = TVIF_TEXT;
	tvitem.hItem = src;
	tvitem.pszText = buf;
	tvitem.cchTextMax = 1025;
	if (!TreeView_GetItem(tree_wnd, &tvitem))
		return;
	buf[1024] = L'\0';
	TVINSERTSTRUCT tvis;
	memset(&tvis, 0, sizeof(tvis));
	tvis.hParent = dst;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = buf;
	HTREEITEM new_item = TreeView_InsertItem(tree_wnd, &tvis);
	if (new_item == NULL)
		return;
	HTREEITEM child = TreeView_GetChild(tree_wnd, src);
	while (child != NULL)
	{
		CopyTreeNodeRecursion(tree_wnd, new_item, child, buf);
		child = TreeView_GetNextSibling(tree_wnd, child);
	}
}

static void ListViewAdjustWidth(HWND wnd)
{
	int cnt = Header_GetItemCount(ListView_GetHeader(wnd));
	for (int i = 0; i < cnt; i++)
		ListView_SetColumnWidth(wnd, i, LVSCW_AUTOSIZE_USEHEADER);
}

static SClass* MakeDrawImpl(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchor_x, S64 anchor_y, Bool equal_magnification, Bool editable, int split)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	SDraw* me3 = reinterpret_cast<SDraw*>(me_);
	SetCtrlParam(me2, reinterpret_cast<SWndBase*>(parent), WndKind_Draw, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY | WS_CLIPCHILDREN, x, y, width, height, L"", WndProcDraw, anchor_x, anchor_y);
	me3->EqualMagnification = equal_magnification;
	me3->DrawTwice = True;
	me3->Enter = False;
	me3->Editable = editable;
	me3->WheelX = 0;
	me3->WheelY = 0;
	if (equal_magnification)
	{
		RECT rect;
		GetWindowRect(me2->WndHandle, &rect);
		int width2 = static_cast<int>(rect.right - rect.left);
		int height2 = static_cast<int>(rect.bottom - rect.top);
		me3->DrawBuf = Draw::MakeDrawBuf(width2, height2, 1, me2->WndHandle, NULL, editable);
	}
	else
		me3->DrawBuf = Draw::MakeDrawBuf(static_cast<int>(width), static_cast<int>(height), split, me2->WndHandle, NULL, editable);
	me3->OnPaint = NULL;
	me3->OnMouseDownL = NULL;
	me3->OnMouseDownR = NULL;
	me3->OnMouseDownM = NULL;
	me3->OnMouseDoubleClick = NULL;
	me3->OnMouseUpL = NULL;
	me3->OnMouseUpR = NULL;
	me3->OnMouseUpM = NULL;
	me3->OnMouseMove = NULL;
	me3->OnMouseEnter = NULL;
	me3->OnMouseLeave = NULL;
	me3->OnMouseWheelX = NULL;
	me3->OnMouseWheelY = NULL;
	me3->OnFocus = NULL;
	me3->OnKeyDown = NULL;
	me3->OnKeyUp = NULL;
	me3->OnKeyChar = NULL;
	me3->OnScrollX = NULL;
	me3->OnScrollY = NULL;
	me3->OnSetMouseImg = NULL;
	return me_;
}

static HIMAGELIST CreateImageList(const void* imgs)
{
	S64 len = static_cast<const S64*>(imgs)[1];
	HIMAGELIST result = NULL;
	const void*const* ptr = reinterpret_cast<void*const*>(static_cast<const U8*>(imgs) + 0x10);
	for (S64 i = 0; i < len; i++)
	{
		const Char* path = reinterpret_cast<const Char*>(static_cast<const U8*>(ptr[i]) + 0x10);
		size_t size;
		void* bin = LoadFileAll(path, &size);
		int width;
		int height;
		void* img = DecodePng(size, bin, &width, &height);
		if (result == NULL)
			result = ImageList_Create(width, height, ILC_COLOR32, static_cast<int>(len), 0);
		BITMAPINFO info = { 0 };
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biWidth = static_cast<LONG>(width);
		info.bmiHeader.biHeight = static_cast<LONG>(height);
		void* raw;
		HBITMAP bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &raw, NULL, 0);
		U32* dst = static_cast<U32*>(raw);
		const U32* src = static_cast<U32*>(img);
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				const U32 color = src[(height - 1 - j) * width + k];
				dst[j * width + k] = (color & 0xff000000) | ((color & 0xff0000) >> 16) | (color & 0xff00) | ((color & 0xff) << 16);
			}
		}
		ImageList_Add(result, bitmap, NULL);
		DeleteObject(bitmap);
		FreeMem(img);
		FreeMem(bin);
	}
	return result;
}

static LRESULT CALLBACK CommonWndProc(HWND wnd, SWndBase* wnd2, SWnd* wnd3, UINT msg, WPARAM w_param, LPARAM l_param)
{
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
		case WM_ACTIVATE:
			if (wnd3->OnActivate != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(w_param) != 0)), reinterpret_cast<void*>(static_cast<S64>(HIWORD(w_param) != 0)), wnd3->OnActivate);
			return 0;
		case WM_DROPFILES:
			if (wnd3->OnDropFiles != NULL)
			{
				HDROP drop = reinterpret_cast<HDROP>(w_param);
				UINT num = DragQueryFile(drop, 0xffffffff, NULL, 0);
				void* buf = AllocMem(0x10 + sizeof(void*) * static_cast<size_t>(num));
				(static_cast<S64*>(buf))[0] = 1;
				(static_cast<S64*>(buf))[1] = static_cast<S64>(num);
				void** ptr = reinterpret_cast<void**>(static_cast<U8*>(buf) + 0x10);
				for (UINT i = 0; i < num; i++)
				{
					UINT len = DragQueryFile(drop, i, NULL, 0);
					void* buf2 = AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len + 1));
					(static_cast<S64*>(buf2))[0] = 1;
					(static_cast<S64*>(buf2))[1] = static_cast<S64>(len);
					Char* str = static_cast<Char*>(buf2) + 0x08;
					DragQueryFile(drop, i, str, len + 1);
					for (UINT j = 0; j < len; j++)
						str[j] = str[j] == L'\\' ? L'/' : str[j];
					ptr[i] = buf2;
				}
				DragFinish(drop);
				Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), buf, wnd3->OnDropFiles);
			}
			return 0;
		case WM_COMMAND:
		case WM_NOTIFY:
			CommandAndNotify(wnd, msg, w_param, l_param);
			return 0;
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
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
		case WM_SIZE:
			EnumChildWindows(wnd, ResizeCallback, NULL);
			if (wnd3->OnResize != NULL)
			{
				Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnResize);
			}
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
	}
	return CommonWndProc(wnd, wnd2, wnd3, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcWndFix(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SWnd* wnd3 = reinterpret_cast<SWnd*>(wnd2);
	if (wnd2 == NULL)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndFix || wnd2->Kind == WndKind_WndPopup || wnd2->Kind == WndKind_WndDialog);
	return CommonWndProc(wnd, wnd2, wnd3, msg, w_param, l_param);
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
		case WM_SIZE:
			EnumChildWindows(wnd, ResizeCallback, NULL);
			if (wnd3->OnResize != NULL)
			{
				Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnResize);
			}
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
		case WM_SIZING:
			{
				RECT* r = reinterpret_cast<RECT*>(l_param);
				double caption = static_cast<double>(GetSystemMetrics(SM_CYCAPTION));
				double border = static_cast<double>(GetSystemMetrics(SM_CYFRAME));
				double w = static_cast<double>(r->right) - static_cast<double>(r->left) - border * 2.0;
				double h = static_cast<double>(r->bottom) - static_cast<double>(r->top) - caption - border * 2.0;
				switch (w_param)
				{
					case WMSZ_TOP:
					case WMSZ_BOTTOM:
						r->right = static_cast<LONG>(static_cast<double>(r->left) + h * static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight) + border * 2.0);
						return 0;
					case WMSZ_LEFT:
					case WMSZ_RIGHT:
						r->bottom = static_cast<LONG>(static_cast<double>(r->top) + w * static_cast<double>(wnd2->DefaultHeight) / static_cast<double>(wnd2->DefaultWidth) + caption + border * 2.0);
						return 0;
					case WMSZ_TOPLEFT:
						if (w / h < static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight))
							r->left = static_cast<LONG>(static_cast<double>(r->right) - h * static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight) - border * 2.0);
						else
							r->top = static_cast<LONG>(static_cast<double>(r->bottom) - w * static_cast<double>(wnd2->DefaultHeight) / static_cast<double>(wnd2->DefaultWidth) - caption - border * 2.0);
						return 0;
					case WMSZ_TOPRIGHT:
						if (w / h < static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight))
							r->right = static_cast<LONG>(static_cast<double>(r->left) + h * static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight) + border * 2.0);
						else
							r->top = static_cast<LONG>(static_cast<double>(r->bottom) - w * static_cast<double>(wnd2->DefaultHeight) / static_cast<double>(wnd2->DefaultWidth) - caption - border * 2.0);
						return 0;
					case WMSZ_BOTTOMLEFT:
						if (w / h < static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight))
							r->left = static_cast<LONG>(static_cast<double>(r->right) - h * static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight) - border * 2.0);
						else
							r->bottom = static_cast<LONG>(static_cast<double>(r->top) + w * static_cast<double>(wnd2->DefaultHeight) / static_cast<double>(wnd2->DefaultWidth) + caption + border * 2.0);
						return 0;
					case WMSZ_BOTTOMRIGHT:
						if (w / h < static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight))
							r->right = static_cast<LONG>(static_cast<double>(r->left) + h * static_cast<double>(wnd2->DefaultWidth) / static_cast<double>(wnd2->DefaultHeight) + border * 2.0);
						else
							r->bottom = static_cast<LONG>(static_cast<double>(r->top) + w * static_cast<double>(wnd2->DefaultHeight) / static_cast<double>(wnd2->DefaultWidth) + caption + border * 2.0);
						return 0;
				}
			}
			break;
	}
	return CommonWndProc(wnd, wnd2, wnd3, msg, w_param, l_param);
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
			}
			else
				ValidateRect(wnd, NULL);
			return 0;
		case WM_LBUTTONDOWN:
			SetFocus(wnd);
			if (wnd3->OnMouseDownL != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseDownL);
			return 0;
		case WM_LBUTTONDBLCLK:
			SetFocus(wnd);
			if (wnd3->OnMouseDownL != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseDownL);
			if (wnd3->OnMouseDoubleClick != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseDoubleClick);
			return 0;
		case WM_LBUTTONUP:
			if (wnd3->OnMouseUpL != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseUpL);
			return 0;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			if (wnd3->OnMouseDownR != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseDownR);
			return 0;
		case WM_RBUTTONUP:
			if (wnd3->OnMouseUpR != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseUpR);
			return 0;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			if (wnd3->OnMouseDownM != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseDownM);
			return 0;
		case WM_MBUTTONUP:
			if (wnd3->OnMouseUpM != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseUpM);
			return 0;
		case WM_MOUSEMOVE:
			if (!wnd3->Enter)
			{
				wnd3->Enter = True;
				if (wnd3->OnMouseEnter != NULL)
					Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseEnter);
			}
			if (wnd3->OnMouseMove != NULL)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(LOWORD(l_param)))), reinterpret_cast<void*>(static_cast<S64>(static_cast<S16>(HIWORD(l_param)))), wnd3->OnMouseMove);
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
		case WM_MOUSEWHEEL:
			if (wnd3->OnMouseWheelY != NULL)
			{
				S64 wheel = 0;
				wnd3->WheelY += static_cast<S16>(HIWORD(w_param));
				while (wnd3->WheelY <= -WHEEL_DELTA)
				{
					wheel--;
					wnd3->WheelY += WHEEL_DELTA;
				}
				while (wnd3->WheelY >= WHEEL_DELTA)
				{
					wheel++;
					wnd3->WheelY -= WHEEL_DELTA;
				}
				if (wheel != 0)
					Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(-wheel), wnd3->OnMouseWheelY);
			}
			return 0;
		case WM_MOUSEHWHEEL:
			if (wnd3->OnMouseWheelX != NULL)
			{
				S64 wheel = 0;
				wnd3->WheelX += static_cast<S16>(HIWORD(w_param));
				while (wnd3->WheelX <= -WHEEL_DELTA)
				{
					wheel--;
					wnd3->WheelX += WHEEL_DELTA;
				}
				while (wnd3->WheelX >= WHEEL_DELTA)
				{
					wheel++;
					wnd3->WheelX -= WHEEL_DELTA;
				}
				if (wheel != 0)
					Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(wheel), wnd3->OnMouseWheelX);
			}
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
			if (wnd3->EqualMagnification)
			{
				int width = static_cast<int>(static_cast<S16>(LOWORD(l_param)));
				int height = static_cast<int>(static_cast<S16>(HIWORD(l_param)));
				if (width > 0 && height > 0)
				{
					wnd3->DrawBuf = Draw::MakeDrawBuf(width, height, 1, wnd2->WndHandle, wnd3->DrawBuf, wnd3->Editable);
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
		case WM_SETCURSOR:
			if (wnd3->OnSetMouseImg)
			{
				S64 img = (S64)Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnSetMouseImg);
				SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(img)));
				return 1;
			}
			break;
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
	SList* wnd3 = reinterpret_cast<SList*>(wnd2);
	ASSERT(wnd2->Kind == WndKind_List);
	switch (msg)
	{
		case WM_LBUTTONDBLCLK:
			if (wnd3->OnMouseDoubleClick != NULL)
				Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnMouseDoubleClick);
			return 0;
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
		case WM_COMMAND:
		case WM_NOTIFY:
			CommandAndNotify(wnd, msg, w_param, l_param);
			return 0;
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
	SListView* wnd3 = reinterpret_cast<SListView*>(wnd2);
	ASSERT(wnd2->Kind == WndKind_ListView);
	switch (msg)
	{
		case WM_LBUTTONDBLCLK:
			if (wnd3->OnMouseDoubleClick != NULL)
				Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnMouseDoubleClick);
			return 0;
		case WM_MOUSEMOVE:
			if (wnd3->Dragging && GetCapture() == wnd)
			{
				POINT pt = { LOWORD(l_param), HIWORD(l_param) };
				ClientToScreen(wnd, &pt);
				RECT wnd_rect;
				GetWindowRect(wnd, &wnd_rect);
				ImageList_DragMove(pt.x - wnd_rect.left, pt.y - wnd_rect.top);
				return 0;
			}
			break;
		case WM_LBUTTONUP:
			if (wnd3->Dragging && GetCapture() == wnd)
			{
				ImageList_DragLeave(wnd);
				ImageList_EndDrag();
				ImageList_Destroy(wnd3->DraggingImage);
				wnd3->DraggingImage = NULL;
				ReleaseCapture();
				wnd3->Dragging = False;

				LV_HITTESTINFO info;
				info.pt.x = LOWORD(l_param);
				info.pt.y = HIWORD(l_param);
				ListView_HitTest(wnd, &info);
				Bool invalid = False;
				if (info.iItem == -1)
					info.iItem = ListView_GetItemCount(wnd);
				else
				{
					LV_ITEM item;
					item.iItem = info.iItem;
					item.iSubItem = 0;
					item.mask = LVIF_STATE;
					item.stateMask = LVIS_SELECTED;
					ListView_GetItem(wnd, &item);
					if ((item.state & LVIS_SELECTED) != 0)
						invalid = True;
				}
				if (!invalid)
				{
					int column_len = Header_GetItemCount(ListView_GetHeader(wnd));
					int id = ListView_GetNextItem(wnd, -1, LVNI_SELECTED);
					LV_ITEM item;
					Char buf[1025];
					item.cchTextMax = 1025;
					item.pszText = buf;
					item.mask = LVIF_STATE | LVIF_IMAGE | LVIF_TEXT;
					item.stateMask = ~static_cast<UINT>(LVIS_SELECTED);
					SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
					while (id != -1)
					{
						item.iItem = id;
						item.iSubItem = 0;
						ListView_GetItem(wnd, &item);
						item.iItem = info.iItem;
						int new_id = ListView_InsertItem(wnd, &item);
						if (info.iItem < id)
							info.iItem++;
						if (new_id <= id)
							id++;
						for (int i = 1; i < column_len; i++)
						{
							item.iItem = id;
							item.iSubItem = i;
							ListView_GetItem(wnd, &item);
							item.iItem = new_id;
							ListView_SetItem(wnd, &item);
						}
						UINT chk = ListView_GetCheckState(wnd, id);
						ListView_SetCheckState(wnd, new_id, chk);
						ListView_DeleteItem(wnd, id);
						id = ListView_GetNextItem(wnd, -1, LVNI_SELECTED);
					}
					SendMessage(wnd, WM_SETREDRAW, TRUE, 0);
					if (wnd3->OnMoveNode)
						Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd3)), wnd3->OnMoveNode);
				}
				return 0;
			}
			break;
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
		case WM_COMMAND:
		case WM_NOTIFY:
			CommandAndNotify(wnd, msg, w_param, l_param);
			return 0;
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcTree(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	STree* wnd3 = reinterpret_cast<STree*>(wnd2);
	switch (msg)
	{
		case WM_MOUSEMOVE:
			if (wnd3->DraggingItem != NULL)
			{
				POINT point;
				TVHITTESTINFO hit_test = { 0 };
				ImageList_DragLeave(NULL);
				GetCursorPos(&point);
				hit_test.flags = TVHT_ONITEM;
				hit_test.pt.x = point.x;
				hit_test.pt.y = point.y;
				ScreenToClient(wnd, &hit_test.pt);
				HTREEITEM item = reinterpret_cast<HTREEITEM>(SendMessage(wnd, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hit_test)));
				if (item != NULL)
				{
					ImageList_DragShowNolock(FALSE);
					SendMessage(wnd, TVM_SELECTITEM, TVGN_DROPHILITE, reinterpret_cast<LPARAM>(item));
					ImageList_DragShowNolock(TRUE);
				}
				ImageList_DragMove(point.x, point.y);
				ImageList_DragEnter(NULL, point.x, point.y);
			}
			return 0;
		case WM_LBUTTONUP:
			if (wnd3->DraggingItem != NULL)
			{
				POINT point;
				TVHITTESTINFO hit_test = { 0 };
				ImageList_DragShowNolock(FALSE);
				GetCursorPos(&point);
				hit_test.flags = TVHT_ONITEM;
				hit_test.pt.x = point.x;
				hit_test.pt.y = point.y;
				ScreenToClient(wnd, &hit_test.pt);
				HTREEITEM item = reinterpret_cast<HTREEITEM>(SendMessage(wnd, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hit_test)));
				if (wnd3->DraggingItem != item && (wnd3->AllowDraggingToRoot || item != NULL))
				{
					Bool success = True;
					{
						// Prohibit dropping a node on one of its children.
						HTREEITEM item2 = item;
						while (item2 != NULL)
						{
							if (item2 == wnd3->DraggingItem)
							{
								success = False;
								break;
							}
							item2 = TreeView_GetParent(wnd, item2);
						}
					}
					if (success)
					{
						Char buf[1025];
						CopyTreeNodeRecursion(wnd, item, wnd3->DraggingItem, buf);
						TreeView_DeleteItem(wnd, wnd3->DraggingItem);
						if (wnd3->OnMoveNode)
							Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd3)), wnd3->OnMoveNode);
					}
				}
				ImageList_DragShowNolock(TRUE);
				ImageList_DragLeave(NULL);
				ImageList_EndDrag();
				wnd3->DraggingItem = NULL;
			}
			return 0;
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
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 1;
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcScrollY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_ScrollY);
	switch (msg)
	{
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 1;
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}
