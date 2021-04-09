#include "wnd_wnd.h"

#include <ShlObj.h>

static int WndCnt;
static Bool ExitAct;
static void* OnKeyPress;
static Char FileDialogDir[KUIN_MAX_PATH + 1];

static LRESULT CALLBACK CommonWndProc(HWND wnd, SWndBase* wnd2, SWnd* wnd3, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcWndNormal(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcWndFix(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcWndAspect(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static Char* ParseFilter(const U8* filter, int* num);
static void NormPath(Char* path, Bool dir);

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

	WndCnt = 0;
	ExitAct = False;

	Instance = (HINSTANCE)GetModuleHandle(nullptr);
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
		wnd_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = nullptr;
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
		wnd_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = nullptr;
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
		wnd_class.hIcon = nullptr;
		wnd_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = nullptr;
		wnd_class.lpszClassName = L"KuinWndDialogClass";
		wnd_class.hIconSm = nullptr;
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
		wnd_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wnd_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wnd_class.lpszMenuName = nullptr;
		wnd_class.lpszClassName = L"KuinWndAspectClass";
		wnd_class.hIconSm = icon;
		RegisterClassEx(&wnd_class);
	}

	{
		HDC dc = GetDC(nullptr);
		FontCtrl = CreateFont(-MulDiv(9, GetDeviceCaps(dc, LOGPIXELSY), 72), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
		ReleaseDC(nullptr, dc);
	}

	OnKeyPress = nullptr;
	FileDialogDir[0] = L'\0';
}

EXPORT_CPP void _fin()
{
	DeleteObject(static_cast<HGDIOBJ>(FontCtrl));
}

EXPORT_CPP void _menuDtor(SClass* me_)
{
	DestroyMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle);
}

EXPORT_CPP void _menuAdd(SClass* me_, S64 id, const U8* text)
{
	THROWDBG(id < 0x0001 || 0xffff < id, 0xe9170006);
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_STRING, static_cast<UINT_PTR>(id), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuAddLine(SClass* me_)
{
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_SEPARATOR, 0, nullptr);
}

EXPORT_CPP void _menuAddPopup(SClass* me_, const U8* text, const U8* popup)
{
	THROWDBG(popup == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	AppendMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, MF_ENABLED | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuIns(SClass* me_, S64 targetId, S64 id, const U8* text)
{
	THROWDBG(targetId < 0x0001 || 0xffff < targetId, 0xe9170006);
	THROWDBG(id < 0x0001 || 0xffff < id, 0xe9170006);
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	InsertMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(targetId), MF_ENABLED | MF_STRING, static_cast<UINT_PTR>(id), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuInsPopup(SClass* me_, const U8* target, const U8* text, const U8* popup)
{
	THROWDBG(target == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(popup == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	// This cast is due to bad specifications of Windows API.
	InsertMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(target)->MenuHandle)), MF_ENABLED | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle), reinterpret_cast<const Char*>(text + 0x10));
}

EXPORT_CPP void _menuDel(SClass* me_, S64 id)
{
	THROWDBG(id < 0x0001 || 0xffff < id, 0xe9170006);
	RemoveMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(id), MF_BYCOMMAND);
}

EXPORT_CPP void _menuDelPopup(SClass* me_, const U8* popup)
{
	THROWDBG(popup == nullptr, EXCPT_ACCESS_VIOLATION);
	// This cast is due to bad specifications of Windows API.
	RemoveMenu(reinterpret_cast<SMenu*>(me_)->MenuHandle, static_cast<UINT>(reinterpret_cast<UINT_PTR>(reinterpret_cast<const SMenu*>(popup)->MenuHandle)), MF_BYCOMMAND);
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
				if (ptr[idx] != nullptr)
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

EXPORT_CPP void _wndAcceptDraggedFiles(SClass* me_, Bool is_accepted)
{
	DragAcceptFiles(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_accepted ? TRUE : FALSE);
}

EXPORT_CPP void _wndActivate(SClass* me_)
{
	SetActiveWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP Bool _wndActivated(SClass* me_)
{
	return GetActiveWindow() == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP void _wndClose(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, WM_CLOSE, 0, 0);
}

EXPORT_CPP void _wndExit(SClass* me_)
{
	DestroyWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP Bool _wndFocusedWnd(SClass* me_)
{
	HWND wnd = GetFocus();
	if (wnd == nullptr)
		return False;
	SWndBase* wnd2 = ToWnd(wnd);
	while (wnd2 != nullptr && static_cast<int>(wnd2->Kind) >= 0x80)
	{
		wnd = GetParent(wnd);
		if (wnd == nullptr)
			return False;
		wnd2 = ToWnd(wnd);
	}
	if (wnd2 == nullptr)
		return False;
	return wnd == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP S64 _wndGetAlpha(SClass* me_)
{
	BYTE alpha;
	GetLayeredWindowAttributes(reinterpret_cast<SWndBase*>(me_)->WndHandle, nullptr, &alpha, nullptr);
	return static_cast<S64>(alpha);
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

EXPORT_CPP void _wndMinMax(SClass* me_, S64 minWidth, S64 minHeight, S64 maxWidth, S64 maxHeight)
{
	THROWDBG(minWidth != -1 && minWidth <= 0 || minHeight != -1 && minHeight <= 0 || maxWidth != -1 && maxWidth < minWidth || maxHeight != -1 && maxHeight < minHeight, 0xe9170006);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->MinWidth = static_cast<U16>(minWidth);
	me2->MinHeight = static_cast<U16>(minHeight);
	me2->MaxWidth = static_cast<U16>(maxWidth);
	me2->MaxHeight = static_cast<U16>(maxHeight);
}

EXPORT_CPP void _wndSetModalLock(SClass* me_)
{
	HWND parent = GetWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, GW_OWNER);
	if (parent != nullptr)
		EnableWindow(parent, FALSE);
	reinterpret_cast<SWnd*>(me_)->ModalLock = True;
}

EXPORT_CPP void _wndSetAlpha(SClass* me_, S64 alpha)
{
	THROWDBG(alpha < 0 || 255 < alpha, 0xe9170006);
	SetLayeredWindowAttributes(reinterpret_cast<SWndBase*>(me_)->WndHandle, 0, static_cast<BYTE>(alpha), LWA_ALPHA);
}

EXPORT_CPP void _wndSetMenu(SClass* me_, SClass* menu)
{
	SetMenu(reinterpret_cast<SWndBase*>(me_)->WndHandle, menu == nullptr ? nullptr : reinterpret_cast<SMenu*>(menu)->MenuHandle);
}

EXPORT_CPP void _wndSetText(SClass* me_, const U8* text)
{
	const U8* text2 = NToRN(text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10));
	SetWindowText(reinterpret_cast<SWndBase*>(me_)->WndHandle, reinterpret_cast<const Char*>(text2 + 0x10));
	FreeMem(const_cast<U8*>(text2));
}

EXPORT_CPP void _wndUpdateMenu(SClass* me_)
{
	DrawMenuBar(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP void _wndBaseDtor(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	DestroyWindow(me2->WndHandle);
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

EXPORT_CPP void _wndBaseFocus(SClass* me_)
{
	SetFocus(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP Bool _wndBaseFocused(SClass* me_)
{
	return GetFocus() == reinterpret_cast<SWndBase*>(me_)->WndHandle;
}

EXPORT_CPP Bool _wndBaseGetEnabled(SClass* me_)
{
	return IsWindowEnabled(reinterpret_cast<SWndBase*>(me_)->WndHandle) != FALSE;
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

EXPORT_CPP Bool _wndBaseGetVisible(SClass* me_)
{
	return IsWindowVisible(reinterpret_cast<SWndBase*>(me_)->WndHandle) != FALSE;
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

EXPORT_CPP void _wndBaseSetEnabled(SClass* me_, Bool is_enabled)
{
	EnableWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_enabled ? TRUE : FALSE);
}

EXPORT_CPP void _wndBaseSetPos(SClass* me_, S64 x, S64 y, S64 width, S64 height)
{
	THROWDBG(width < 0 || height < 0, 0xe9170006);
	SetWindowPos(reinterpret_cast<SWndBase*>(me_)->WndHandle, nullptr, (int)x, (int)y, (int)width, (int)height, SWP_NOZORDER);
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
			InvalidateRect(wnd, nullptr, TRUE);
		}
		else
			SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
	}
}

EXPORT_CPP void _wndBaseSetVisible(SClass* me_, Bool is_visible)
{
	ShowWindow(reinterpret_cast<SWndBase*>(me_)->WndHandle, is_visible ? SW_SHOW : SW_HIDE);
}

EXPORT_CPP Bool _act()
{
	if (ExitAct || WndCnt == 0)
		return False;
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
				case WM_QUIT:
					ExitAct = True;
					return False;
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					if (OnKeyPress != nullptr)
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
	Sleep(1);
	return True;
}

EXPORT_CPP S64 _colorDialog(SClass* parent, S64 default_color)
{
	THROWDBG(default_color < 0 || 0xffffff < default_color, 0xe9170006);
	CHOOSECOLOR choose_color = { 0 };
	choose_color.lStructSize = sizeof(CHOOSECOLOR);
	choose_color.hwndOwner = parent == nullptr ? nullptr : reinterpret_cast<SWndBase*>(parent)->WndHandle;
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

EXPORT_CPP void* _exeDir()
{
	Char path[KUIN_MAX_PATH];
	Char* ptr;
	GetModuleFileName(nullptr, path, KUIN_MAX_PATH);
	ptr = wcsrchr(path, L'\\');
	if (ptr == nullptr)
		return nullptr;
	*(ptr + 1) = L'\0';
	ptr = path;
	while (*ptr != L'\0')
	{
		if (*ptr == L'\\')
			*ptr = L'/';
		ptr++;
	}
	size_t len = wcslen(path);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	*(S64*)(result + 0x00) = DefaultRefCntFunc;
	*(S64*)(result + 0x08) = len;
	memcpy(result + 0x10, path, sizeof(Char) * (len + 1));
	return result;
}

EXPORT_CPP void _fileDialogDir(const U8* defaultDir)
{
	if (defaultDir == nullptr)
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

EXPORT_CPP void* _getClipboardStr()
{
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) == 0)
		return nullptr;
	if (OpenClipboard(nullptr) == 0)
		return nullptr;
	HGLOBAL handle = GetClipboardData(CF_UNICODETEXT);
	if (handle == nullptr)
	{
		CloseClipboard();
		return nullptr;
	}
	U8* result = nullptr;
	{
		const Char* buf = static_cast<Char*>(GlobalLock(handle));
		if (buf == nullptr)
		{
			CloseClipboard();
			return nullptr;
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

EXPORT_CPP void* _getOnKeyPress()
{
	return OnKeyPress;
}

EXPORT_CPP Bool _key(S64 key)
{
	return (GetKeyState(static_cast<int>(key)) & 0x8000) != 0;
}

EXPORT_CPP SClass* _makeMenu(SClass* me_)
{
	SMenu* me2 = reinterpret_cast<SMenu*>(me_);
	me2->MenuHandle = CreateMenu();
	if (me2->MenuHandle == nullptr)
		THROW(0xe9170009);
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	return me_;
}

EXPORT_CPP SClass* _makePopup(SClass* me_)
{
	SMenu* me2 = reinterpret_cast<SMenu*>(me_);
	me2->MenuHandle = CreatePopupMenu();
	if (me2->MenuHandle == nullptr)
		THROW(0xe9170009);
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	return me_;
}

EXPORT_CPP SClass* _makeTabOrder(SClass* me_, U8* ctrls)
{
	STabOrder* me2 = reinterpret_cast<STabOrder*>(me_);
	THROWDBG(ctrls == nullptr, EXCPT_ACCESS_VIOLATION);
	S64 len = *reinterpret_cast<S64*>(ctrls + 0x08);
	void** ptr = reinterpret_cast<void**>(ctrls + 0x10);
	void* result = AllocMem(0x10 + sizeof(void*) * static_cast<size_t>(len));
	static_cast<S64*>(result)[0] = 1;
	static_cast<S64*>(result)[1] = len;
	void** result2 = reinterpret_cast<void**>(static_cast<U8*>(result) + 0x10);
	for (S64 i = 0; i < len; i++)
	{
		if (ptr[i] == nullptr)
			result2[i] = nullptr;
		else
		{
			(*static_cast<S64*>(ptr[i]))++;
			result2[i] = ptr[i];
		}
	}
	me2->Ctrls = result;
	return me_;
}

EXPORT_CPP SClass* _makeWnd(SClass* me_, SClass* parent, S64 style, S64 width, S64 height, const U8* text)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	me2->Kind = static_cast<EWndKind>(static_cast<S64>(WndKind_WndNormal) + (style & 0xffff));
	THROWDBG(width < 0 || height < 0, 0xe9170006);
	int width2 = static_cast<int>(width);
	int height2 = static_cast<int>(height);
	HWND parent2 = parent == nullptr ? nullptr : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	DWORD ex_style = 0;
	if ((style & static_cast<S64>(WndKind_WndLayered)) != 0)
		ex_style |= WS_EX_LAYERED;
	switch (me2->Kind)
	{
		case WndKind_WndNormal:
			me2->WndHandle = CreateWindowEx(ex_style, L"KuinWndNormalClass", text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, nullptr, Instance, nullptr);
			break;
		case WndKind_WndFix:
			me2->WndHandle = CreateWindowEx(ex_style, L"KuinWndFixClass", text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, nullptr, Instance, nullptr);
			break;
		case WndKind_WndAspect:
			me2->WndHandle = CreateWindowEx(ex_style, L"KuinWndAspectClass", text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), (WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, nullptr, Instance, nullptr);
			break;
		case WndKind_WndPopup:
			me2->WndHandle = CreateWindowEx(ex_style | WS_EX_TOOLWINDOW, L"KuinWndDialogClass", text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_POPUP | WS_BORDER | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, nullptr, Instance, nullptr);
			break;
		case WndKind_WndDialog:
			me2->WndHandle = CreateWindowEx(ex_style | WS_EX_TOOLWINDOW, L"KuinWndDialogClass", text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, width2, height2, parent2, nullptr, Instance, nullptr);
			break;
		default:
			THROWDBG(True, 0xe9170006);
			break;
	}
	if (me2->WndHandle == nullptr)
		THROW(0xe9170009);
	if ((style & static_cast<S64>(WndKind_WndLayered)) != 0)
		SetLayeredWindowAttributes(me2->WndHandle, 0, 255, LWA_ALPHA);
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
	me2->Name = nullptr;
	me2->DefaultWndProc = nullptr;
	me2->CtrlFlag = static_cast<U64>(CtrlFlag_AnchorLeft) | static_cast<U64>(CtrlFlag_AnchorTop);
	me2->DefaultX = 0;
	me2->DefaultY = 0;
	me2->DefaultWidth = static_cast<U16>(width);
	me2->DefaultHeight = static_cast<U16>(height);
	me2->RedrawEnabled = True;
	me2->Children = AllocMem(0x28);
	*(S64*)me2->Children = 1;
	memset((U8*)me2->Children + 0x08, 0x00, 0x20);
	SetWindowPos(me2->WndHandle, nullptr, 0, 0, static_cast<int>(width) + border_x, static_cast<int>(height) + border_y, SWP_NOMOVE | SWP_NOZORDER);
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
		SetWindowPos(me2->WndHandle, nullptr, 0, 0, static_cast<int>(w) + border_x, static_cast<int>(h) + border_y, SWP_NOMOVE | SWP_NOZORDER);
	}
	SetWindowLongPtr(me2->WndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me2));
	{
		SWnd* me3 = reinterpret_cast<SWnd*>(me_);
		me3->MinWidth = 128;
		me3->MinHeight = 128;
		me3->MaxWidth = static_cast<U16>(-1);
		me3->MaxHeight = static_cast<U16>(-1);
		me3->OnClose = nullptr;
		me3->OnActivate = nullptr;
		me3->OnPushMenu = nullptr;
		me3->OnDropFiles = nullptr;
		me3->OnResize = nullptr;
		me3->ModalLock = False;
	}
	SendMessage(me2->WndHandle, WM_SETFONT, reinterpret_cast<WPARAM>(FontCtrl), static_cast<LPARAM>(FALSE));
	ShowWindow(me2->WndHandle, SW_SHOWNORMAL);
	WndCnt++;
	return me_;
}

EXPORT_CPP S64 _msgBox(SClass* parent, const U8* text, const U8* title, S64 icon, S64 btn)
{
	return MessageBox(parent == nullptr ? nullptr : reinterpret_cast<SWndBase*>(parent)->WndHandle, text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), title == nullptr ? L"" : reinterpret_cast<const Char*>(title + 0x10), static_cast<UINT>(icon | btn));
}

EXPORT_CPP void* _openFileDialog(SClass* parent, const U8* filter, S64 defaultFilter)
{
	Char path[KUIN_MAX_PATH + 1];
	path[0] = L'\0';
	int filter_num;
	Char* filter_mem = ParseFilter(filter, &filter_num);
	THROWDBG(!(filter_num == 0 && defaultFilter == 0 || filter_num != 0 && 0 <= defaultFilter && defaultFilter < filter_num), 0xe9170006);
	OPENFILENAME open_file_name;
	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = parent == nullptr ? nullptr : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	open_file_name.lpstrFilter = filter_mem;
	open_file_name.nFilterIndex = filter_num == 0 ? 0 : static_cast<DWORD>(defaultFilter + 1);
	open_file_name.lpstrFile = path;
	open_file_name.nMaxFile = KUIN_MAX_PATH + 1;
	open_file_name.lpstrInitialDir = FileDialogDir[0] == L'\0' ? nullptr : FileDialogDir;
	open_file_name.lpstrTitle = nullptr;
	open_file_name.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	BOOL success = GetOpenFileName(&open_file_name);
	if (filter_mem != nullptr)
		FreeMem(filter_mem);
	if (success == FALSE)
		return nullptr;
	size_t len = wcslen(path);
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len + 1)));
	*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
	Char* dst = reinterpret_cast<Char*>(result + 0x10);
	for (size_t i = 0; i <= len; i++)
		dst[i] = path[i] == L'\\' ? L'/' : path[i];
	return result;
}

EXPORT_CPP void* _saveFileDialog(SClass* parent, const U8* filter, S64 defaultFilter, const U8* defaultExt)
{
	Char path[KUIN_MAX_PATH + 1];
	path[0] = L'\0';
	int filter_num;
	Char* filter_mem = ParseFilter(filter, &filter_num);
	THROWDBG(!(filter_num == 0 && defaultFilter == 0 || filter_num != 0 && 0 <= defaultFilter && defaultFilter < filter_num), 0xe9170006);
	OPENFILENAME open_file_name;
	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = parent == nullptr ? nullptr : reinterpret_cast<SWndBase*>(parent)->WndHandle;
	open_file_name.lpstrFilter = filter_mem;
	open_file_name.nFilterIndex = filter_num == 0 ? 0 : static_cast<DWORD>(defaultFilter + 1);
	open_file_name.lpstrFile = path;
	open_file_name.nMaxFile = KUIN_MAX_PATH + 1;
	open_file_name.lpstrInitialDir = FileDialogDir[0] == L'\0' ? nullptr : FileDialogDir;
	open_file_name.lpstrTitle = nullptr;
	open_file_name.lpstrDefExt = defaultExt == nullptr ? nullptr : reinterpret_cast<const Char*>(defaultExt + 0x10);
	open_file_name.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	BOOL success = GetSaveFileName(&open_file_name);
	if (filter_mem != nullptr)
		FreeMem(filter_mem);
	if (success == FALSE)
		return nullptr;
	size_t len = wcslen(path);
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(len + 1)));
	*reinterpret_cast<S64*>(result + 0x00) = DefaultRefCntFunc;
	*reinterpret_cast<S64*>(result + 0x08) = static_cast<S64>(len);
	Char* dst = reinterpret_cast<Char*>(result + 0x10);
	for (size_t i = 0; i <= len; i++)
		dst[i] = path[i] == L'\\' ? L'/' : path[i];
	return result;
}

EXPORT_CPP void _screenSize(S64* width, S64* height)
{
	*width = static_cast<S64>(GetSystemMetrics(SM_CXSCREEN));
	*height = static_cast<S64>(GetSystemMetrics(SM_CYSCREEN));
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
	if (handle == nullptr)
		return;
	{
		const Char* ptr = reinterpret_cast<const Char*>(str + 0x10);
		Char* buf = static_cast<Char*>(GlobalLock(handle));
		if (buf == nullptr)
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
	if (OpenClipboard(nullptr) == 0)
	{
		GlobalFree(handle);
		return;
	}
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, static_cast<HANDLE>(handle));
	CloseClipboard();
}

EXPORT_CPP void _setOnKeyPress(void* onKeyPressFunc)
{
	OnKeyPress = onKeyPressFunc;
}

EXPORT_CPP void* _sysDir(S64 kind)
{
	Char path[KUIN_MAX_PATH + 2];
	if (!SHGetSpecialFolderPath(nullptr, path, (int)kind, TRUE))
		return NULL;
	NormPath(path, True);
	size_t len = wcslen(path);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	*(S64*)(result + 0x00) = DefaultRefCntFunc;
	*(S64*)(result + 0x08) = (S64)len;
	wcscpy((Char*)(result + 0x10), path);
	return result;
}

static LRESULT CALLBACK CommonWndProc(HWND wnd, SWndBase* wnd2, SWnd* wnd3, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
		case WM_CLOSE:
			if (wnd3->OnClose != nullptr)
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
				if (parent != nullptr)
				{
					EnableWindow(parent, TRUE);
					SetActiveWindow(parent);
				}
				wnd3->ModalLock = False;
			}
			return 0;
		case WM_ACTIVATE:
			if (wnd3->OnActivate != nullptr)
				Call3Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), reinterpret_cast<void*>(static_cast<S64>(LOWORD(w_param) != 0)), reinterpret_cast<void*>(static_cast<S64>(HIWORD(w_param) != 0)), wnd3->OnActivate);
			return 0;
		case WM_DROPFILES:
			if (wnd3->OnDropFiles != nullptr)
			{
				HDROP drop = reinterpret_cast<HDROP>(w_param);
				UINT num = DragQueryFile(drop, 0xffffffff, nullptr, 0);
				void* buf = AllocMem(0x10 + sizeof(void*) * static_cast<size_t>(num));
				(static_cast<S64*>(buf))[0] = 1;
				(static_cast<S64*>(buf))[1] = static_cast<S64>(num);
				void** ptr = reinterpret_cast<void**>(static_cast<U8*>(buf) + 0x10);
				for (UINT i = 0; i < num; i++)
				{
					UINT len = DragQueryFile(drop, i, nullptr, 0);
					void* buf2 = AllocMem(0x10 + sizeof(Char) * (static_cast<size_t>(len) + 1));
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
	if (wnd2 == nullptr)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndNormal);
	switch (msg)
	{
		case WM_SIZE:
			EnumChildWindows(wnd, ResizeCallback, 0);
			if (wnd3->OnResize != nullptr)
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
	if (wnd2 == nullptr)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndFix || wnd2->Kind == WndKind_WndPopup || wnd2->Kind == WndKind_WndDialog);
	return CommonWndProc(wnd, wnd2, wnd3, msg, w_param, l_param);
}

static LRESULT CALLBACK WndProcWndAspect(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SWnd* wnd3 = reinterpret_cast<SWnd*>(wnd2);
	if (wnd2 == nullptr)
		return DefWindowProc(wnd, msg, w_param, l_param);
	ASSERT(wnd2->Kind == WndKind_WndAspect);
	switch (msg)
	{
		case WM_SIZE:
			EnumChildWindows(wnd, ResizeCallback, 0);
			if (wnd3->OnResize != nullptr)
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

static Char* ParseFilter(const U8* filter, int* num)
{
	if (filter == nullptr)
	{
		*num = 0;
		return nullptr;
	}
	S64 len_parent = *reinterpret_cast<const S64*>(filter + 0x08);
	THROWDBG(len_parent % 2 != 0, 0xe9170006);
	S64 total = 0;
	{
		const void* const* ptr = reinterpret_cast<const void* const*>(filter + 0x10);
		for (S64 i = 0; i < len_parent; i++)
		{
			S64 len = *reinterpret_cast<const S64*>(static_cast<const U8*>(*ptr) + 0x08);
			total += len + 1;
			ptr++;
		}
	}
	Char* result = static_cast<Char*>(AllocMem(sizeof(Char) * static_cast<size_t>(total + 1)));
	{
		const void* const* ptr = reinterpret_cast<const void* const*>(filter + 0x10);
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

static void NormPath(Char* path, Bool dir)
{
	if (*path == L'\0')
		return;
	do
	{
		if (*path == L'\\')
			*path = L'/';
		path++;
	} while (*path != L'\0');
	if (dir && path[-1] != L'/')
	{
		path[0] = L'/';
		path[1] = L'\0';
	}
}
