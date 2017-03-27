#include "editor.h"

static const Char* Reserved[] =
{
	L"alias",
	L"assert",
	L"bit16",
	L"bit32",
	L"bit64",
	L"bit8",
	L"block",
	L"bool",
	L"break",
	L"case",
	L"catch",
	L"char",
	L"class",
	L"const",
	L"dbg",
	L"default",
	L"dict",
	L"do",
	L"elif",
	L"else",
	L"end",
	L"enum",
	L"false",
	L"finally",
	L"float",
	L"for",
	L"foreach",
	L"func",
	L"if",
	L"ifdef",
	L"inf",
	L"int",
	L"list",
	L"me",
	L"null",
	L"queue",
	L"ret",
	L"rls",
	L"skip",
	L"stack",
	L"switch",
	L"throw",
	L"to",
	L"true",
	L"try",
	L"var",
	L"while",
};

typedef struct SWndBase
{
	SClass Class;
	S64 Kind;
	HWND WndHandle;
} SWndBase;

static const COLORREF ColorBack = RGB(255, 245, 245);
static const COLORREF ColorLineNumber = RGB(255, 127, 127);
static const COLORREF ColorAreaBack = RGB(127, 127, 127);
static const COLORREF ColorAreaText = RGB(255, 255, 255);
static const COLORREF ColorStr = RGB(0, 161, 40); // 'H' = 135.
static const COLORREF ColorChar = RGB(161, 120, 0); // 'H' = 45.
static const COLORREF ColorLineComment = RGB(80, 0, 161); // 'H' = 270.
static const COLORREF ColorComment = RGB(80, 161, 0); // 'H' = 90.
static const COLORREF ColorGlobal = RGB(161, 0, 120); // 'H' = 315.
static const COLORREF ColorReserved = RGB(0, 40, 161); // 'H' = 225.
static const COLORREF ColorIdentifier = RGB(161, 0, 0); // 'H' = 0.
static const COLORREF ColorNumber = RGB(0, 161, 161); // 'H' = 180.
static const COLORREF ColorOperator = RGB(120, 120, 120); // 'S' = 0.

static HWND WndHandle;
static WNDPROC DefaultWndProc;
static void* FuncIns;
static void* FuncCmd;
static void* FuncReplace;
static const void* Src;
static int PageX, PageY;
static int ScrWidth, ScrHeight;
static int CellWidth, CellHeight;
static int CharHeight;
static int CursorX, CursorY;
static int AreaX, AreaY;
static Bool Drag;
static int LineNumberWidth;
static int HighlightLen;
static COLORREF HighlightColor;
static HFONT FontSrc;
static HPEN PenLineNumber;
static HWND ScrollX, ScrollY;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static LRESULT CALLBACK WndProcEditor(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static int MouseXToCharX(int mouse_x);
static void RecordArea(Bool shift);
static int GetAreaLen(int* begin_x, int* begin_y, Bool crlf);
static void DelStrInArea(void);
static void CopyStrInArea(void);
static Bool SelectArea(void);
static void PasteStr(void);
static void InsertChar(Char c);
static void CallCmd(int cmd, int len);
static int Log10(int n);
static int SrcLen(void);
static const void* YPtrGet(int y);
static const void* YPtrNext(const void* y_ptr);
static const void* YPtrPrev(const void* y_ptr);
static const void* YPtrItem(const void* y_ptr);
static int YPtrItemLen(const void* y_ptr);
static const Char* YPtrItemStr(const void* y_ptr);
static int Wide(Char c, int x);
static void RefreshScrSize(void);
static void RefreshCursor(Bool right, Bool refreshScroll);
static void Draw(HDC dc, const RECT* rect);
static int BinSearch(const Char** hay_stack, int num, const Char* needle);
static Bool IsReserved(const Char* word);
static void ResetHighlight(void);
static COLORREF InterpretHighlight(const Char* str);

EXPORT void EditorInit(SClass* me_, void* func_ins, void* func_cmd, void* func_replace, SClass* scroll_x, SClass* scroll_y)
{
	WndHandle = (HWND)*(void**)((U8*)me_ + sizeof(SClass) + sizeof(S64));
	DefaultWndProc = (WNDPROC)(U64)*(void**)((U8*)me_ + sizeof(SClass) + sizeof(S64) + sizeof(void*));
	SetWindowLongPtr(WndHandle, GWLP_WNDPROC, (LONG_PTR)WndProcEditor);
	SetCursor(LoadCursor(NULL, IDC_IBEAM)); // TODO:

	FuncIns = func_ins;
	FuncCmd = func_cmd;
	FuncReplace = func_replace;
	ScrollX = ((SWndBase*)scroll_x)->WndHandle;
	ScrollY = ((SWndBase*)scroll_y)->WndHandle;

	RefreshScrSize();
	{
		HDC dc = GetDC(NULL);
		CharHeight = MulDiv(10, GetDeviceCaps(dc, LOGPIXELSY), 72);
		ReleaseDC(NULL, dc);
	}

	FontSrc = CreateFont(-CharHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, L"Consolas");
	PenLineNumber = CreatePen(PS_SOLID, 1, ColorLineNumber);

	{
		int len = (int)(sizeof(Reserved) / sizeof(Char*));
		int i;
		for (i = 0; i < len - 1; i++)
			ASSERT(wcscmp(Reserved[i], Reserved[i + 1]) < 0);
	}
}

EXPORT void EditorFin(void)
{
	DeleteObject((HGDIOBJ)PenLineNumber);
	DeleteObject((HGDIOBJ)FontSrc);
}

EXPORT void EditorSetSrc(const void* src)
{
	Src = src;
	PageX = 0;
	PageY = 0;
	CellWidth = 8;
	CellHeight = 18;
	CursorX = 0;
	CursorY = 0;
	AreaX = -1;
	AreaY = -1;
	Drag = False;
	LineNumberWidth = 0;
	ResetHighlight();

	RefreshCursor(False, True);
	InvalidateRect(WndHandle, NULL, False);
}

EXPORT void EditorSetCursor(S64* newX, S64* newY, S64 x, S64 y, Bool refresh)
{
	CursorX = (int)x;
	CursorY = (int)y;
	if (refresh)
		RefreshCursor(False, True);
	*newX = (S64)CursorX;
	*newY = (S64)CursorY;
}

static LRESULT CALLBACK WndProcEditor(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	ASSERT(wnd == WndHandle);
	switch (msg)
	{
		case WM_PAINT:
			if (Src != NULL)
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(WndHandle, &ps);
				Draw(dc, &ps.rcPaint);
				EndPaint(WndHandle, &ps);
			}
			return 0;
		case WM_LBUTTONDOWN:
			SetFocus(wnd);
			{
				int mouse_x = (int)LOWORD(l_param);
				int mouse_y = (int)HIWORD(l_param);
				RECT rect;
				GetClientRect(wnd, &rect);
				RecordArea((GetKeyState(VK_SHIFT) & 0x8000) != 0);
				CursorY = PageY + (mouse_y - (int)rect.top) / CellHeight;
				CursorX = MouseXToCharX(mouse_x);
				Drag = True;
				RefreshCursor(False, True);
				if (!SelectArea())
				{
					AreaX = CursorX;
					AreaY = CursorY;
				}
				InvalidateRect(WndHandle, NULL, False);
			}
			return 0;
		case WM_LBUTTONUP:
			if (Drag)
				Drag = False;
			return 0;
		case WM_MOUSEMOVE:
			if (Drag)
			{
				int mouse_x = (int)LOWORD(l_param);
				int mouse_y = (int)HIWORD(l_param);
				RECT rect;
				GetClientRect(wnd, &rect);
				CursorY = PageY + (mouse_y - (int)rect.top) / CellHeight;
				CursorX = MouseXToCharX(mouse_x);
				RefreshCursor(False, True);
				InvalidateRect(WndHandle, NULL, False);
			}
			return 0;
		case WM_MOUSEWHEEL:
			// TODO:
			return 0;
		case WM_MOUSEHWHEEL:
			// TODO:
			return 0;
		case WM_SETFOCUS:
			CreateCaret(WndHandle, NULL, 2, CharHeight);
			ShowCaret(WndHandle);
			return 0;
		case WM_KILLFOCUS:
			DestroyCaret();
			return 0;
		case WM_KEYDOWN:
			{
				Bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
				Bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
				Bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
				switch (w_param)
				{
					case 'A':
						if (!shift && ctrl && !alt)
						{
							AreaX = 0;
							AreaY = 0;
							CursorX = INT_MAX;
							CursorY = INT_MAX;
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case 'C':
						if (!shift && ctrl && !alt && SelectArea())
							CopyStrInArea();
						break;
					case 'V':
						if (!shift && ctrl && !alt)
						{
							if (SelectArea())
								DelStrInArea();
							AreaX = -1;
							PasteStr();
						}
						break;
					case 'X':
						if (!shift && ctrl && !alt && SelectArea())
						{
							CopyStrInArea();
							DelStrInArea();
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case 'Z':
						if (ctrl && !alt)
						{
							if (SelectArea())
								AreaX = -1;
							if (shift)
								CallCmd(4, 1);
							else
								CallCmd(3, 1);
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						// TODO:
						break;
					case VK_BACK:
						if (!shift && !ctrl && !alt)
						{
							if (!SelectArea())
							{
								U8 pos[0x10 + 0x10];
								*(S64*)(pos + 0x00) = 2;
								*(S64*)(pos + 0x08) = 2;
								*(S64*)(pos + 0x10) = (S64)CursorX;
								*(S64*)(pos + 0x18) = (S64)CursorY;
								Call3Asm(pos, (void*)(S64)1, (void*)(U64)0, FuncCmd);
								ASSERT(*(S64*)pos == 1);
								RefreshCursor(False, True);
								InvalidateRect(WndHandle, NULL, False);
							}
							else
							{
								DelStrInArea();
								RefreshCursor(False, True);
								InvalidateRect(WndHandle, NULL, False);
							}
						}
						break;
					case VK_TAB:
						if (!SelectArea())
						{
							InsertChar(L'\t');
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						else
						{
							// TODO: Indent.
						}
						break;
					case VK_RETURN:
						if (!shift && !ctrl && !alt)
						{
							if (SelectArea())
								DelStrInArea();
							AreaX = -1;
							CallCmd(2, 1);
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_PRIOR: // Page up.
						// TODO:
						break;
					case VK_NEXT: // Page down.
						// TODO:
						break;
					case VK_END:
						if (!alt)
						{
							RecordArea(shift);
							CursorX = INT_MAX;
							if (ctrl)
								CursorY = INT_MAX;
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_HOME:
						if (!alt)
						{
							RecordArea(shift);
							CursorX = 0;
							if (ctrl)
								CursorY = 0;
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_LEFT:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorX--;
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_UP:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorY--;
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_RIGHT:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorX++;
							RefreshCursor(True, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_DOWN:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorY++;
							RefreshCursor(False, True);
							InvalidateRect(WndHandle, NULL, False);
						}
						break;
					case VK_INSERT:
						// TODO:
						break;
					case VK_DELETE:
						if (!shift && !ctrl && !alt)
						{
							if (!SelectArea())
							{
								CallCmd(1, 1);
								RefreshCursor(False, True);
								InvalidateRect(WndHandle, NULL, False);
							}
							else
							{
								DelStrInArea();
								RefreshCursor(False, True);
								InvalidateRect(WndHandle, NULL, False);
							}
						}
						break;
					case VK_APPS:
						// TODO:
						break;
				}
			}
			return 0;
		case WM_CHAR:
			if ((Char)w_param == L'\t' || Wide((Char)w_param, 0) == 0)
				return 0;
			if (SelectArea())
				DelStrInArea();
			AreaX = -1;
			InsertChar((Char)w_param);
			RefreshCursor(False, True);
			InvalidateRect(WndHandle, NULL, False);
			return 0;
		case WM_SIZE:
			RefreshScrSize();
			RefreshCursor(False, True);
			InvalidateRect(WndHandle, NULL, False);
			return 0;
		case WM_HSCROLL:
		case WM_VSCROLL:
			{
				HWND scroll = (HWND)l_param;
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
				if (msg == WM_HSCROLL)
					PageX = info.nPos;
				else
					PageY = info.nPos;
				SetScrollInfo(scroll, SB_CTL, &info, TRUE);
				RefreshCursor(False, False);
				InvalidateRect(WndHandle, NULL, False);
			}
			break;
		// TODO:
	}
	return CallWindowProc(DefaultWndProc, wnd, msg, w_param, l_param);
}

static int MouseXToCharX(int mouse_x)
{
	if (CursorY >= SrcLen())
		return 0;
	else
	{
		int left = LineNumberWidth - PageX * CellWidth;
		int x = 0;
		const Char* str = YPtrItemStr(YPtrGet(CursorY));
		const Char* ptr = str;
		int i = 0;
		while (*ptr != L'\0')
		{
			int wide = Wide(*ptr, x);
			if (left + x * CellWidth + wide * CellWidth > mouse_x)
				break;
			x += wide;
			ptr++;
			i++;
		}
		return i;
	}
}

static void RecordArea(Bool shift)
{
	if (shift)
	{
		if (!SelectArea())
		{
			AreaX = CursorX;
			AreaY = CursorY;
		}
	}
	else
		AreaX = -1;
}

static int GetAreaLen(int* begin_x, int* begin_y, Bool crlf)
{
	int x1 = AreaX, y1 = AreaY;
	int x2 = CursorX, y2 = CursorY;
	int result;
	const void* ptr;
	int i;
	ASSERT(SelectArea());
	if (y1 > y2 || y1 == y2 && x1 > x2)
	{
		int tmp;
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	ptr = YPtrGet(y1);
	if (y1 != y2)
	{
		result = YPtrItemLen(ptr) - x1 + (crlf ? 2 : 1);
		for (i = y1 + 1; i <= y2 - 1; i++)
		{
			ptr = YPtrNext(ptr);
			result += YPtrItemLen(ptr) + (crlf ? 2 : 1);
		}
		result += x2;
	}
	else
		result = x2 - x1;
	*begin_x = x1;
	*begin_y = y1;
	return result;
}

static void DelStrInArea(void)
{
	ASSERT(SelectArea());
	CallCmd(1, GetAreaLen(&CursorX, &CursorY, False));
	AreaX = -1;
}

static void CopyStrInArea(void)
{
	ASSERT(SelectArea());
	{
		int x, y;
		int len = GetAreaLen(&x, &y, True);
		HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE |GHND , sizeof(Char) * (len + 1));
		if (handle == NULL)
			return;
		{
			Char* mem = (Char*)GlobalLock(handle);
			Char* dst = mem;
			const void* ptr = YPtrGet(y);
			const Char* s = YPtrItemStr(ptr) + x;
			int i;
			for (i = 0; i < len; i++)
			{
				Char c = *s;
				if (c == L'\0')
				{
					c = L'\r';
					*dst = c;
					dst++;
					i++;
					c = L'\n';
					ptr = YPtrNext(ptr);
					s = YPtrItemStr(ptr);
				}
				else
					s++;
				*dst = c;
				dst++;
			}
			*dst = L'\0';
			ASSERT(dst == mem + len);
			GlobalUnlock(handle);
		}
		if (OpenClipboard(WndHandle) == 0)
		{
			GlobalFree(handle);
			return;
		}
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, (HANDLE)handle);
		CloseClipboard();
	}
}

static Bool SelectArea(void)
{
	return AreaX != -1 && (AreaX != CursorX || AreaY != CursorY);
}

static void PasteStr(void)
{
	HGLOBAL handle;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) == 0)
		return;
	if (OpenClipboard(WndHandle) == 0)
		return;
	handle = GetClipboardData(CF_UNICODETEXT);
	if (handle == NULL)
	{
		CloseClipboard();
		return;
	}
	{
		Char* begin = (Char*)GlobalLock(handle);
		Char* end = begin;
		if (begin == NULL)
		{
			CloseClipboard();
			return;
		}
		for (; ; )
		{
			if (*end == L'\r' || *end == L'\n' || *end == L'\0' || end - begin == 512)
			{
				if (end != begin)
				{
					U8 pos[0x10 + 0x10];
					size_t len = end - begin;
					U8 str[0x10 + sizeof(Char) * (512 + 1)];
					*(S64*)(pos + 0x00) = 2;
					*(S64*)(pos + 0x08) = 2;
					*(S64*)(pos + 0x10) = (S64)CursorX;
					*(S64*)(pos + 0x18) = (S64)CursorY;
					*(S64*)(str + 0x00) = 2;
					*(S64*)(str + 0x08) = (S64)len;
					memcpy(str + 0x10, begin, sizeof(Char) * len);
					*(Char*)(str + 0x10 + sizeof(Char) * len) = L'\0';
					Call2Asm(pos, str, FuncIns);
					ASSERT(*(S64*)pos == 1 && *(S64*)str == 1);
					if (end - begin == 512)
						begin = end;
					else
						begin = end + 1;
				}
				if (*end == L'\r' || *end == L'\n')
				{
					CallCmd(2, 1);
					if (*end == L'\r' && *(end + 1) == L'\n')
					{
						end++;
						begin = end + 1;
					}
				}
				else if (*end == L'\0')
					break;
			}
			end++;
		}
	}
	GlobalUnlock(handle);
	CloseClipboard();
	RefreshCursor(False, True);
	InvalidateRect(WndHandle, NULL, False);
}

static void InsertChar(Char c)
{
	U8 pos[0x10 + 0x10];
	U8 str[0x10 + sizeof(Char) * 2];
	*(S64*)(pos + 0x00) = 2;
	*(S64*)(pos + 0x08) = 2;
	*(S64*)(pos + 0x10) = (S64)CursorX;
	*(S64*)(pos + 0x18) = (S64)CursorY;
	*(S64*)(str + 0x00) = 2;
	*(S64*)(str + 0x08) = 1;
	*(Char*)(str + 0x10) = c;
	*(Char*)(str + 0x12) = L'\0';
	Call2Asm(pos, str, FuncIns);
	ASSERT(*(S64*)pos == 1 && *(S64*)str == 1);
}

static void CallCmd(int cmd, int len)
{
	U8 pos[0x10 + 0x10];
	*(S64*)(pos + 0x00) = 2;
	*(S64*)(pos + 0x08) = 2;
	*(S64*)(pos + 0x10) = (S64)CursorX;
	*(S64*)(pos + 0x18) = (S64)CursorY;
	Call3Asm(pos, (void*)(S64)len, (void*)(U64)cmd, FuncCmd);
	ASSERT(*(S64*)pos == 1);
}

static int Log10(int n)
{
	int result = 0;
	int m = 1;
	while (n >= m)
	{
		m *= 10;
		result++;
	}
	return result;
}

static int SrcLen(void)
{
	return (int)*(S64*)((U8*)Src + 0x08);
}

static const void* YPtrGet(int y)
{
	int len = (int)*(S64*)((U8*)Src + 0x08);
	int i;
	const void* y_ptr;
	if (y < len / 2)
	{
		y_ptr = *(void**)((U8*)Src + 0x10);
		for (i = 0; i < y; i++)
			y_ptr = YPtrNext(y_ptr);
	}
	else
	{
		y_ptr = *(void**)((U8*)Src + 0x18);
		ASSERT(y < len);
		for (i = len - 1; i > y; i--)
			y_ptr = YPtrPrev(y_ptr);
	}
	return y_ptr;
}

static const void* YPtrNext(const void* y_ptr)
{
	return *(void**)((U8*)y_ptr + 0x08);
}

static const void* YPtrPrev(const void* y_ptr)
{
	return *(void**)y_ptr;
}

static const void* YPtrItem(const void* y_ptr)
{
	return *(void**)((U8*)y_ptr + 0x10);
}

static int YPtrItemLen(const void* y_ptr)
{
	return (int)*(S64*)((U8*)*(void**)((U8*)YPtrItem(y_ptr) + 0x10) + 0x08);
}

static const Char* YPtrItemStr(const void* y_ptr)
{
	return (const Char*)((U8*)*(void**)((U8*)YPtrItem(y_ptr) + 0x10) + 0x10);
}

static int Wide(Char c, int x)
{
	if (c == L'\t')
		return 4 - (x % 4);
	if (c <= 0x1f)
		return 0;
	if (c <= 0x7e)
		return 1;
	if (c <= 0xa0)
		return 0;
	return 2;
}

static void RefreshScrSize(void)
{
	RECT rect;
	GetClientRect(WndHandle, &rect);
	ScrWidth = (int)(rect.right - rect.left);
	ScrHeight = (int)(rect.bottom - rect.top);
}

static void RefreshCursor(Bool right, Bool refreshScroll)
{
	int src_len = SrcLen();
	if (CursorX < 0)
	{
		CursorX = INT_MAX;
		CursorY--;
	}
	if (CursorY < 0)
	{
		CursorX = 0;
		CursorY = 0;
	}
	if (CursorY > src_len - 1)
	{
		CursorY = src_len - 1;
		CursorX = YPtrItemLen(YPtrGet(CursorY));
	}
	else
	{
		int len = YPtrItemLen(YPtrGet(CursorY));
		if (CursorX > len)
		{
			if (right && CursorY != src_len - 1)
			{
				CursorX = 0;
				CursorY++;
			}
			else
				CursorX = len;
		}
	}

	if (refreshScroll)
	{
		SCROLLINFO info;
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
		info.nMin = 0;
		info.nMax = src_len - 1;
		info.nPage = ScrHeight / CellHeight;
		info.nPos = PageY;
		info.nTrackPos = 0;
		SetScrollInfo(ScrollY, SB_CTL, &info, TRUE);

		if (PageY > CursorY)
			PageY = CursorY;
		if (PageY < CursorY - ScrHeight / CellHeight + 1)
			PageY = CursorY - ScrHeight / CellHeight + 1;
	}

	LineNumberWidth = CellWidth * (Log10(SrcLen()) + 1);
	{
		int left = LineNumberWidth - PageX * CellWidth;
		int x = 0;
		const Char* str = YPtrItemStr(YPtrGet(CursorY));
		const Char* ptr = str;
		int i;
		for (i = 0; i < CursorX; i++)
		{
			x += Wide(*ptr, x);
			ptr++;
		}
		SetCaretPos(left + x * CellWidth, (CursorY - PageY) * CellHeight);
	}
}

static void Draw(HDC dc, const RECT* rect)
{
	HGDIOBJ font_old = SelectObject(dc, (HGDIOBJ)FontSrc);
	SetBkMode(dc, OPAQUE);
	SetBkColor(dc, ColorBack);
	ExtTextOut(dc, rect->left, rect->top, ETO_CLIPPED | ETO_OPAQUE, rect, L"", 0, NULL);

	{
		HGDIOBJ pen_old = SelectObject(dc, (HGDIOBJ)PenLineNumber);
		LONG x = (LONG)(LineNumberWidth - CellWidth / 2);
		POINT points[2];
		points[0].x = x;
		points[0].y = rect->top;
		points[1].x = x;
		points[1].y = rect->bottom;
		Polyline(dc, points, 2);
		SelectObject(dc, pen_old);
	}

	{
		int i;
		int len = SrcLen();
		Char str[33];
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, ColorLineNumber);
		for (i = 0; i < len; i++)
		{
			int str_len = swprintf(str, 33, L"%d", PageY + i + 1);
			ExtTextOut(dc, LineNumberWidth - (str_len + 1) * CellWidth + CellWidth / 2, i * CellHeight, ETO_CLIPPED, rect, str, (UINT)str_len, NULL);
		}
	}

	{
		const void* y_ptr = YPtrGet(PageY);
		int i, j;
		int x1 = AreaX, y1 = AreaY;
		int x2 = CursorX, y2 = CursorY;
		if (SelectArea() && (y1 > y2 || y1 == y2 && x1 > x2))
		{
			int tmp;
			tmp = x1;
			x1 = x2;
			x2 = tmp;
			tmp = y1;
			y1 = y2;
			y2 = tmp;
		}
		for (i = 0; i < ScrHeight / CellHeight; i++)
		{
			int y = i * CellHeight;
			if (y >= (int)rect->bottom)
				break;
			if (y >= (int)rect->top)
			{
				const Char* str = YPtrItemStr(y_ptr);
				int left = -PageX * CellWidth + LineNumberWidth;
				int x = 0;
				ResetHighlight();
				for (j = 0; j < ScrWidth / CellWidth; j++)
				{
					COLORREF color = InterpretHighlight(str);
					ASSERT(*str != L'\r' && *str != L'\n');
					if (left + x * CellWidth + CellWidth >= (int)rect->right)
						break;
					if (left + x * CellWidth >= (int)rect->left + LineNumberWidth)
					{
						Bool in_area = False;
						if (SelectArea())
						{
							int x3 = j, y3 = PageY + i;
							if (y3 < y1 || y2 < y3)
								in_area = False;
							else if (y1 < y3 && y3 < y2)
								in_area = True;
							else if (y1 == y2)
								in_area = x1 <= x3 && x3 < x2;
							else if (y3 == y1)
								in_area = x1 <= x3;
							else
							{
								ASSERT(y3 == y2);
								in_area = x3 < x2;
							}
						}
						if (in_area)
						{
							if (*str == L'\0')
							{
								// TODO:
								SetBkMode(dc, OPAQUE);
								SetBkColor(dc, ColorAreaBack);
								SetTextColor(dc, ColorAreaText);
								ExtTextOut(dc, left + x * CellWidth, i * CellHeight, ETO_CLIPPED, rect, L" ", 1, NULL);
							}
							else
							{
								SetBkMode(dc, OPAQUE);
								SetBkColor(dc, ColorAreaBack);
								SetTextColor(dc, ColorAreaText);
								ExtTextOut(dc, left + x * CellWidth, i * CellHeight, ETO_CLIPPED, rect, str, 1, NULL);
							}
						}
						else
						{
							if (*str == L'\0')
							{
								// TODO:
							}
							else
							{
								SetBkMode(dc, TRANSPARENT);
								SetTextColor(dc, color);
								ExtTextOut(dc, left + x * CellWidth, i * CellHeight, ETO_CLIPPED, rect, str, 1, NULL);
							}
						}
					}
					if (*str == L'\0')
						break;
					{
						Char c = *str;
						int wide = Wide(c, x);
						if (wide == 0)
						{
							c = L'?';
							wide = 1;
						}
						x += wide;
					}
					str++;
				}
			}
			y_ptr = YPtrNext(y_ptr);
			if (y_ptr == NULL)
				break;
		}
	}
	SelectObject(dc, font_old);
}

static int BinSearch(const Char** hay_stack, int num, const Char* needle)
{
	int min = 0;
	int max = num - 1;
	while (min <= max)
	{
		int mid = (min + max) / 2;
		int cmp = wcscmp(needle, hay_stack[mid]);
		if (cmp < 0)
			max = mid - 1;
		else if (cmp > 0)
			min = mid + 1;
		else
			return mid;
	}
	return -1;
}

static Bool IsReserved(const Char* word)
{
	return BinSearch(Reserved, (int)(sizeof(Reserved) / sizeof(Char*)), word) != -1;
}

static void ResetHighlight(void)
{
	HighlightLen = 0;
	HighlightColor = 0;
}

static COLORREF InterpretHighlight(const Char* str)
{
	if (HighlightLen > 0)
	{
		HighlightLen--;
		if (HighlightLen != 0)
			return HighlightColor;
	}
	{
		Char buf[1024];
		Bool at = False;
		const Char* src = str;
		Char* dst = buf;
		if (*src == L'"')
		{
			do
			{
				*dst = *src;
				src++;
				dst++;
				if (dst - buf == 1023)
					break;
			} while (*src != L'"' && *src != L'\0');
			if (dst - buf != 1023 && *src == L'"')
			{
				*dst = *src;
				src++;
				dst++;
			}
			*dst = L'\0';
			HighlightLen = (int)(dst - buf);
			HighlightColor = ColorStr;
			return HighlightColor;
		}
		if (L'\'' == *src)
		{
			do
			{
				*dst = *src;
				src++;
				dst++;
				if (dst - buf == 1023)
					break;
			} while (*src != L'\'' && *src != L'\0');
			if (dst - buf != 1023 && *src == L'\'')
			{
				*dst = *src;
				src++;
				dst++;
			}
			*dst = L'\0';
			HighlightLen = (int)(dst - buf);
			HighlightColor = ColorChar;
			return HighlightColor;
		}
		if (L';' == *src)
		{
			do
			{
				*dst = *src;
				src++;
				dst++;
				if (dst - buf == 1023)
					break;
			} while (*src != L'\0');
			*dst = L'\0';
			HighlightLen = (int)(dst - buf);
			HighlightColor = ColorLineComment;
			return HighlightColor;
		}
		if (L'{' == *src)
		{
			int level = 0;
			do
			{
				if (*src == L'{')
					level++;
				else if (*src == L'}')
					level--;
				*dst = *src;
				src++;
				dst++;
				if (dst - buf == 1023)
					break;
			} while (level > 0 && *src != L'\0');
			*dst = L'\0';
			HighlightLen = (int)(dst - buf);
			HighlightColor = ColorComment;
			return HighlightColor;
		}
		if (L'a' <= *src && *src <= L'z' || L'A' <= *src && *src <= L'Z' || *src == L'_' || *src == L'@' || *src == L'\\')
		{
			do
			{
				if (*src == L'@')
					at = True;
				*dst = *src;
				src++;
				dst++;
				if (dst - buf == 1023)
					break;
			} while (L'a' <= *src && *src <= L'z' || L'A' <= *src && *src <= L'Z' || *src == L'_' || L'0' <= *src && *src <= L'9' || *src == L'@' || *src == L'\\');
			*dst = L'\0';

			HighlightLen = (int)(dst - buf);
			if (at)
			{
				HighlightColor = ColorGlobal;
				return HighlightColor;
			}
			if (IsReserved(buf))
			{
				HighlightColor = ColorReserved;
				return HighlightColor;
			}
			HighlightColor = ColorIdentifier;
			return HighlightColor;
		}
		if (L'0' <= *src && *src <= L'9')
		{
			do
			{
				if (*src == L'@')
					at = True;
				*dst = *src;
				src++;
				dst++;
				if (dst - buf == 1023)
					break;
			} while (L'0' <= *src && *src <= L'9' || L'A' <= *src && *src <= L'F' || *src == L'#' || *src == L'.');
			if (dst - buf != 1023)
			{
				if (*src == L'e')
				{
					*dst = *src;
					src++;
					dst++;
					if (dst - buf != 1023 && (*src == L'+' || *src == L'-'))
					{
						*dst = *src;
						src++;
						dst++;
						while (dst - buf != 1023 && (L'0' <= *src && *src <= L'9'))
						{
							*dst = *src;
							src++;
							dst++;
						}
					}
				}
				else if (*src == L'b')
				{
					*dst = *src;
					src++;
					dst++;
					while (dst - buf != 1023 && (L'0' <= *src && *src <= L'9'))
					{
						*dst = *src;
						src++;
						dst++;
					}
				}
			}
			*dst = L'\0';
			HighlightLen = (int)(dst - buf);
			HighlightColor = ColorNumber;
			return HighlightColor;
		}
	}
	return ColorOperator;
}
