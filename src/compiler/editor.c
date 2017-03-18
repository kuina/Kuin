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
static HFONT FontSrc;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static LRESULT CALLBACK WndProcEditor(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static void RecordArea(Bool shift);
static int GetAreaLen(int* begin_x, int* begin_y);
static void DelStrInArea(void);
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
	InvalidateRect(WndHandle, NULL, TRUE);
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
				int x = (int)LOWORD(l_param);
				int y = (int)HIWORD(l_param);
				RECT rect;
				GetClientRect(wnd, &rect);
				RecordArea((GetKeyState(VK_SHIFT) & 0x8000) != 0);
				CursorX = PageX + (x - (int)rect.left) / CellWidth;
				CursorY = PageY + (y - (int)rect.top) / CellHeight;
				RefreshCursor(False);
				InvalidateRect(WndHandle, NULL, TRUE);
			}
			return 0;
		case WM_RBUTTONUP:
			// TODO:
			return 0;
		case WM_MOUSEMOVE:
			// TODO:
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
						if (!shift && ctrl && !alt && AreaX != -1)
						{
							// TODO:
						}
						break;
					case 'V':
						if (!shift && ctrl && !alt)
						{
							// TODO:
						}
						break;
					case 'X':
						if (!shift && ctrl && !alt && AreaX != -1)
						{
							// TODO:
						}
						break;
					case VK_BACK:
						if (!shift && !ctrl && !alt)
						{
							if (AreaX == -1)
							{
								U8 pos[0x10 + 0x10];
								*(S64*)(pos + 0x00) = 2;
								*(S64*)(pos + 0x08) = 2;
								*(S64*)(pos + 0x10) = (S64)CursorX;
								*(S64*)(pos + 0x18) = (S64)CursorY;
								CursorX--;
								RefreshCursor(False);
								Call3Asm(pos, (void*)(S64)1, (void*)(U64)0, FuncCmd);
								ASSERT(*(S64*)pos == 1);
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
						// TODO:
						break;
					case VK_RETURN:
						if (!shift && !ctrl && !alt)
						{
							U8 pos[0x10 + 0x10];
							*(S64*)(pos + 0x00) = 2;
							*(S64*)(pos + 0x08) = 2;
							*(S64*)(pos + 0x10) = (S64)CursorX;
							*(S64*)(pos + 0x18) = (S64)CursorY;
							Call3Asm(pos, (void*)(S64)1, (void*)(U64)2, FuncCmd);
							ASSERT(*(S64*)pos == 1);
							CursorX = 0;
							CursorY++;
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
							if (AreaX == -1)
							{
								U8 pos[0x10 + 0x10];
								*(S64*)(pos + 0x00) = 2;
								*(S64*)(pos + 0x08) = 2;
								*(S64*)(pos + 0x10) = (S64)CursorX;
								*(S64*)(pos + 0x18) = (S64)CursorY;
								Call3Asm(pos, (void*)(S64)1, (void*)(U64)1, FuncCmd);
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
					case VK_APPS:
						// TODO:
						break;
				}
			}
			return 0;
		case WM_CHAR:
			if (Wide((Char)w_param) == 0)
				return 0;
			{
				U8 pos[0x10 + 0x10];
				U8 str[0x10 + sizeof(Char) * 2];
				*(S64*)(pos + 0x00) = 2;
				*(S64*)(pos + 0x08) = 2;
				*(S64*)(pos + 0x10) = (S64)CursorX;
				*(S64*)(pos + 0x18) = (S64)CursorY;
				*(S64*)(str + 0x00) = 2;
				*(S64*)(str + 0x08) = 1;
				*(Char*)(str + 0x10) = (Char)w_param;
				*(Char*)(str + 0x12) = L'\0';
				Call2Asm(pos, str, FuncIns);
				ASSERT(*(S64*)pos == 1 && *(S64*)str == 1);
				CursorX++;
				RefreshCursor(False);
				InvalidateRect(WndHandle, NULL, TRUE);
			}
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
		if (AreaX == -1)
		{
			AreaX = CursorX;
			AreaY = CursorY;
		}
	}
	else
		AreaX = -1;
}

static int GetAreaLen(int* begin_x, int* begin_y)
{
	int x1 = AreaX, y1 = AreaY;
	int x2 = CursorX, y2 = CursorY;
	int result;
	const void* ptr = YPtrGet(y1);
	ASSERT(AreaX != -1);
	int i;
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
	if (y1 != y2)
	{
		result = YPtrItemLen(ptr) - x1 + 1;
		for (i = y1 + 1; i <= y2 - 1; i++)
		{
			ptr = YPtrNext(ptr);
			result += YPtrItemLen(ptr) + 1;
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
	int len = GetAreaLen(&CursorX, &CursorY);
	U8 pos[0x10 + 0x10];
	*(S64*)(pos + 0x00) = 2;
	*(S64*)(pos + 0x08) = 2;
	*(S64*)(pos + 0x10) = (S64)CursorX;
	*(S64*)(pos + 0x18) = (S64)CursorY;
	AreaX = -1;
	Call3Asm(pos, (void*)(S64)len, (void*)(U64)1, FuncCmd);
	ASSERT(*(S64*)pos == 1);
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
	// TODO: Wide char.
	SetCaretPos((CursorX - PageX) * CellWidth, (CursorY - PageY) * CellHeight);
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
		if (AreaX != -1 && (y1 > y2 || y1 == y2 && x1 > x2))
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
				int x = -PageX * CellWidth;
				for (j = 0; j < ScrWidth / CellWidth; j++)
				{
					ASSERT(*str != L'\r' && *str != L'\n');
					if (*str == L'\0' || x + CellWidth >= (int)rect->right)
						break;
					if (x >= (int)rect->left)
					{
						Bool in_area = False;
						if (AreaX != -1)
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
