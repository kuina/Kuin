#include "editor.h"

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
static HFONT FontSrc;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static LRESULT CALLBACK WndProcEditor(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
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
static int Wide(Char c);
static void RefreshScrSize(void);
static void RefreshCursor(Bool right);
static void Draw(HDC dc, const RECT* rect);

EXPORT void EditorInit(SClass* me_, void* func_ins, void* func_cmd, void* func_replace)
{
	WndHandle = (HWND)*(void**)((U8*)me_ + sizeof(SClass) + sizeof(S64));
	DefaultWndProc = (WNDPROC)(U64)*(void**)((U8*)me_ + sizeof(SClass) + sizeof(S64) + sizeof(void*));
	SetWindowLongPtr(WndHandle, GWLP_WNDPROC, (LONG_PTR)WndProcEditor);
	SetCursor(LoadCursor(NULL, IDC_IBEAM)); // TODO:

	FuncIns = func_ins;
	FuncCmd = func_cmd;
	FuncReplace = func_replace;

	Src = NULL;
	PageX = 0;
	PageY = 0;
	RefreshScrSize();
	CellWidth = 8;
	CellHeight = 18;
	{
		HDC dc = GetDC(NULL);
		CharHeight = MulDiv(10, GetDeviceCaps(dc, LOGPIXELSY), 72);
		ReleaseDC(NULL, dc);
	}
	CursorX = 0;
	CursorY = 0;
	AreaX = -1;
	AreaY = -1;
	Drag = False;
	LineNumberWidth = 0;

	FontSrc = CreateFont(-CharHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, L"Consolas");
}

EXPORT void EditorFin(void)
{
	DeleteObject((HGDIOBJ)FontSrc);
}

EXPORT void EditorSetSrc(const void* src)
{
	Src = src;
	PageX = 0;
	PageY = 0;
	RefreshCursor(False);
	InvalidateRect(WndHandle, NULL, TRUE);
}

EXPORT void EditorSetCursor(S64* newX, S64* newY, S64 x, S64 y, Bool refresh)
{
	CursorX = (int)x;
	CursorY = (int)y;
	if (refresh)
		RefreshCursor(False);
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
				int x = (int)LOWORD(l_param) - LineNumberWidth;
				int y = (int)HIWORD(l_param);
				RECT rect;
				GetClientRect(wnd, &rect);
				RecordArea((GetKeyState(VK_SHIFT) & 0x8000) != 0);
				CursorX = PageX + (x - (int)rect.left) / CellWidth;
				CursorY = PageY + (y - (int)rect.top) / CellHeight;
				Drag = True;
				RefreshCursor(False);
				if (!SelectArea())
				{
					AreaX = CursorX;
					AreaY = CursorY;
				}
				InvalidateRect(WndHandle, NULL, TRUE);
			}
			return 0;
		case WM_LBUTTONUP:
			if (Drag)
				Drag = False;
			return 0;
		case WM_MOUSEMOVE:
			if (Drag)
			{
				int x = (int)LOWORD(l_param) - LineNumberWidth;
				int y = (int)HIWORD(l_param);
				RECT rect;
				GetClientRect(wnd, &rect);
				CursorX = PageX + (x - (int)rect.left) / CellWidth;
				CursorY = PageY + (y - (int)rect.top) / CellHeight;
				RefreshCursor(False);
				InvalidateRect(WndHandle, NULL, TRUE);
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
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
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
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
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
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
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
								RefreshCursor(False);
								InvalidateRect(WndHandle, NULL, TRUE);
							}
							else
							{
								DelStrInArea();
								RefreshCursor(False);
								InvalidateRect(WndHandle, NULL, TRUE);
							}
						}
						break;
					case VK_TAB:
						if (!SelectArea())
						{
							InsertChar(L'\t');
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
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
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
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
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
						}
						break;
					case VK_HOME:
						if (!alt)
						{
							RecordArea(shift);
							CursorX = 0;
							if (ctrl)
								CursorY = 0;
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
						}
						break;
					case VK_LEFT:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorX--;
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
						}
						break;
					case VK_UP:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorY--;
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
						}
						break;
					case VK_RIGHT:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorX++;
							RefreshCursor(True);
							InvalidateRect(WndHandle, NULL, TRUE);
						}
						break;
					case VK_DOWN:
						if (!ctrl && !alt)
						{
							RecordArea(shift);
							CursorY++;
							RefreshCursor(False);
							InvalidateRect(WndHandle, NULL, TRUE);
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
								RefreshCursor(False);
								InvalidateRect(WndHandle, NULL, TRUE);
							}
							else
							{
								DelStrInArea();
								RefreshCursor(False);
								InvalidateRect(WndHandle, NULL, TRUE);
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
			if (Wide((Char)w_param) == 0)
				return 0;
			if (SelectArea())
				DelStrInArea();
			AreaX = -1;
			InsertChar((Char)w_param);
			RefreshCursor(False);
			InvalidateRect(WndHandle, NULL, TRUE);
			return 0;
		case WM_SIZE:
			// TODO:
			break;
		// TODO:
	}
	return CallWindowProc(DefaultWndProc, wnd, msg, w_param, l_param);
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
	RefreshCursor(False);
	InvalidateRect(WndHandle, NULL, TRUE);
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

static int Wide(Char c)
{
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

static void RefreshCursor(Bool right)
{
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
	{
		int src_len = SrcLen();
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
	}
	// TODO: Page.
	// TODO: Wide char.
	LineNumberWidth = CellWidth * Log10(SrcLen() + 1);
	SetCaretPos((CursorX - PageX) * CellWidth + LineNumberWidth, (CursorY - PageY) * CellHeight);
}

static void Draw(HDC dc, const RECT* rect)
{
	HGDIOBJ font_old = SelectObject(dc, (HGDIOBJ)FontSrc);
	SetBkMode(dc, OPAQUE);
	SetBkColor(dc, RGB(0xff, 0xe2, 0xdf));
	ExtTextOut(dc, rect->left, rect->top, ETO_CLIPPED | ETO_OPAQUE, rect, L"", 0, NULL);

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
				int x = -PageX * CellWidth + LineNumberWidth;
				for (j = 0; j < ScrWidth / CellWidth; j++)
				{
					ASSERT(*str != L'\r' && *str != L'\n');
					if (*str == L'\0' || x + CellWidth >= (int)rect->right)
						break;
					if (x >= (int)rect->left + LineNumberWidth)
					{
						Bool in_area = False;
						if (SelectArea())
						{
							int x3 = PageX + j, y3 = PageY + i;
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
							SetBkMode(dc, OPAQUE);
							SetBkColor(dc, RGB(127, 127, 127));
							SetTextColor(dc, RGB(255, 255, 255));
							ExtTextOut(dc, x, i * CellHeight, ETO_CLIPPED, rect, str, 1, NULL);
						}
						else
						{
							SetBkMode(dc, TRANSPARENT);
							SetTextColor(dc, RGB(0, 0, 0));
							ExtTextOut(dc, x, i * CellHeight, ETO_CLIPPED, rect, str, 1, NULL);
						}
					}
					{
						Char c = *str;
						int wide = Wide(c);
						if (wide == 0)
						{
							c = L'?';
							wide = 1;
						}
						x += CellWidth * wide;
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
