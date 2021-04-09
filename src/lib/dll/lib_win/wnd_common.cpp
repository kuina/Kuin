#include "wnd_common.h"

#pragma comment(lib, "Comctl32.lib")

HINSTANCE Instance;
HFONT FontCtrl;

static void ParseAnchor(SWndBase* wnd, S64 anchor_x, S64 anchor_y, S64 x, S64 y, S64 width, S64 height);

SClass* IncWndRef(SClass* wnd)
{
	if (wnd != nullptr)
		wnd->RefCnt++;
	return wnd;
}

void CommandAndNotify(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	if (msg == WM_COMMAND)
	{
		if (l_param == 0)
		{
			// A menu item is clicked.
			SWnd* wnd2 = reinterpret_cast<SWnd*>(ToWnd(wnd));
			if (wnd2->OnPushMenu != nullptr)
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
							if (btn->OnPush != nullptr)
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
							if (chk->OnPush != nullptr)
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
							if (radio->OnPush != nullptr)
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
							if (reinterpret_cast<SEditBase*>(edit)->OnChange != nullptr)
								Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), reinterpret_cast<SEditBase*>(edit)->OnChange);
							break;
						case EN_HSCROLL:
							// TODO:
							break;
						case EN_KILLFOCUS:
							if (reinterpret_cast<SEditBase*>(edit)->OnFocus != nullptr)
								Call2Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), reinterpret_cast<void*>(static_cast<U64>(False)), reinterpret_cast<SEditBase*>(edit)->OnFocus);
							break;
						case EN_SETFOCUS:
							if (reinterpret_cast<SEditBase*>(edit)->OnFocus != nullptr)
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
							if (list->OnSel != nullptr)
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
		if (wnd_ctrl2 != nullptr)
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
								if (tab->OnSel != nullptr)
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
									ImageList_DragEnter(nullptr, param->ptDrag.x, param->ptDrag.y);
									tree->DraggingItem = param->itemNew.hItem;
								}
								break;
							case TVN_SELCHANGED:
								if (tree->OnSel != nullptr)
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
								if (list_view->OnSel != nullptr)
								{
									NMLISTVIEW* param = reinterpret_cast<NMLISTVIEW*>(l_param);
									if ((param->uOldState & LVIS_SELECTED) != (param->uNewState & LVIS_SELECTED))
										Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd_ctrl2)), list_view->OnSel);
								}
								break;
							case NM_CLICK:
								if (list_view->OnMouseClick != nullptr)
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

SWndBase* ToWnd(HWND wnd)
{
	return reinterpret_cast<SWndBase*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
}

BOOL CALLBACK ResizeCallback(HWND wnd, LPARAM l_param)
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
	SetWindowPos(wnd, nullptr, new_x, new_y, new_width, new_height, SWP_NOZORDER);
	return TRUE;
}

const U8* NToRN(const Char* str)
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

const U8* RNToN(const Char* str)
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

void SetCtrlParam(SWndBase* wnd, SWndBase* parent, EWndKind kind, const Char* ctrl, DWORD style_ex, DWORD style, S64 x, S64 y, S64 width, S64 height, const Char* text, WNDPROC wnd_proc, S64 anchor_x, S64 anchor_y)
{
	THROWDBG(parent == nullptr, 0xe9170006);
	THROWDBG(x < 0 || y < 0 || width < 0 || height < 0, 0xe9170006);
	wnd->Kind = kind;
	wnd->WndHandle = CreateWindowEx(style_ex, ctrl, text, style, static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), parent->WndHandle, nullptr, Instance, nullptr);
	if (wnd->WndHandle == nullptr)
		THROW(0xe9170009);
	SetWindowLongPtr(wnd->WndHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
	wnd->Name = nullptr;
	wnd->DefaultWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(wnd->WndHandle, GWLP_WNDPROC));
	wnd->RedrawEnabled = True;
	wnd->Children = AllocMem(0x28);
	*(S64*)wnd->Children = 1;
	memset((U8*)wnd->Children + 0x08, 0x00, 0x20);
	SetWindowLongPtr(wnd->WndHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc));
	ParseAnchor(wnd, anchor_x, anchor_y, x, y, width, height);
	SendMessage(wnd->WndHandle, WM_SETFONT, reinterpret_cast<WPARAM>(FontCtrl), static_cast<LPARAM>(FALSE));
}

static void ParseAnchor(SWndBase* wnd, S64 anchor_x, S64 anchor_y, S64 x, S64 y, S64 width, S64 height)
{
	THROWDBG(x != static_cast<S64>(static_cast<U16>(x)) || y != static_cast<S64>(static_cast<U16>(y)) || width != static_cast<S64>(static_cast<U16>(width)) || height != static_cast<S64>(static_cast<U16>(height)), 0xe9170006);
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
			THROWDBG(True, 0xe9170006);
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
			THROWDBG(True, 0xe9170006);
			break;
	}
	wnd->DefaultX = static_cast<U16>(x);
	wnd->DefaultY = static_cast<U16>(y);
	wnd->DefaultWidth = static_cast<U16>(width);
	wnd->DefaultHeight = static_cast<U16>(height);
	ResizeCallback(wnd->WndHandle, 0);
}

// TODO:
/*

static LRESULT CALLBACK WndProcCalendar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcProgress(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcRebar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcStatus(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcToolbar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcTrackbar(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcLabelLink(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcPager(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcSplitX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcSplitY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

EXPORT_CPP void* _openFileDialogMulti(SClass* parent, const U8* filter, S64 defaultFilter)
{
	// TODO:
	return nullptr;
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

EXPORT_CPP SClass* _makePager(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Pager, WC_PAGESCROLLER, 0, WS_VISIBLE | WS_CHILD, x, y, width, height, L"", WndProcPager, anchorX, anchorY);
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

*/
