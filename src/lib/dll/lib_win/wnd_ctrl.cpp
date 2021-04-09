#include "wnd_ctrl.h"

#include "png_decoder.h"

static void ListViewAdjustWidth(HWND wnd);
static void TreeExpandAllRecursion(HWND wnd_handle, HTREEITEM node, int flag);
static void CopyTreeNodeRecursion(HWND tree_wnd, HTREEITEM dst, HTREEITEM src, Char* buf);
static HIMAGELIST CreateImageList(const void* imgs);
static LRESULT CALLBACK WndProcBtn(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcChk(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcCombo(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcEdit(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcEditMulti(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcGroup(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcLabel(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcList(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcListView(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcRadio(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcScrollX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcScrollY(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcTab(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static LRESULT CALLBACK WndProcTree(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

EXPORT_CPP Bool _btnGetChk(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	return SendMessage(me2->WndHandle, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
}

EXPORT_CPP void _btnSetChk(SClass* me_, Bool chk)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	SendMessage(me2->WndHandle, BM_SETCHECK, static_cast<WPARAM>(chk ? BST_CHECKED : BST_UNCHECKED), 0);
}

EXPORT_CPP void _comboAdd(SClass* me_, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _comboClear(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_RESETCONTENT, 0, 0);
}

EXPORT_CPP void _comboDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_DELETESTRING, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP S64 _comboGetSel(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETCURSEL, 0, 0));
}

EXPORT_CPP void* _comboGetText(SClass* me_, S64 idx)
{
#if defined(DBG)
	{
		S64 len = _comboLen(me_);
		THROWDBG(idx < 0 || len <= idx, 0xe9170006);
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

EXPORT_CPP void _comboIns(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP S64 _comboLen(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETCOUNT, 0, 0));
}

EXPORT_CPP void _comboSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < -1 || len <= idx, 0xe9170006);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP void _comboSetText(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _comboLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	{
		int sel = static_cast<int>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_GETCURSEL, 0, 0));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_DELETESTRING, static_cast<WPARAM>(idx + 1), 0);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, CB_SETCURSEL, static_cast<WPARAM>(sel), 0);
	}
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
	if (enabled)
		SetWindowLong(wnd, GWL_STYLE, dw_style | ES_RIGHT);
	else
		SetWindowLong(wnd, GWL_STYLE, dw_style & ~ES_RIGHT);
	InvalidateRect(wnd, nullptr, TRUE);
}

EXPORT_CPP void _editSetSel(SClass* me_, S64 start, S64 len)
{
	THROWDBG(!(len == -1 && (start == -1 || start == 0) || 0 <= start && 0 <= len), 0xe9170006);
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

EXPORT_CPP void _editMultiAddText(SClass* me_, const U8* text)
{
	if (text == nullptr)
		return;
	const U8* text2 = NToRN(reinterpret_cast<const Char*>(text + 0x10));
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	HWND wnd = me2->WndHandle;
	Bool redraw_enabled = me2->RedrawEnabled;
	if (redraw_enabled)
		SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
	SendMessage(wnd, EM_SETSEL, 0, static_cast<LPARAM>(-1)); // Select all.
	SendMessage(wnd, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1)); // Unselect.
	SendMessage(wnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(wnd, nullptr, TRUE);
	SendMessage(wnd, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(const_cast<U8*>(text2 + 0x10)));
	if (!redraw_enabled)
		SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
	FreeMem(const_cast<U8*>(text2));
}

EXPORT_CPP void _listAdd(SClass* me_, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP void _listClear(SClass* me_)
{
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_RESETCONTENT, 0, 0);
}

EXPORT_CPP void _listDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_DELETESTRING, static_cast<WPARAM>(idx), 0);
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
		THROWDBG(idx < 0 || len <= idx, 0xe9170006);
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

EXPORT_CPP void _listIns(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
}

EXPORT_CPP S64 _listLen(SClass* me_)
{
	return static_cast<S64>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCOUNT, 0, 0));
}

EXPORT_CPP void _listSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < -1 || len <= idx, 0xe9170006);
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP void _listSetText(SClass* me_, S64 idx, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _listLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	{
		int sel = static_cast<int>(SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_GETCURSEL, 0, 0));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_INSERTSTRING, static_cast<WPARAM>(idx), reinterpret_cast<LPARAM>(text + 0x10));
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_DELETESTRING, static_cast<WPARAM>(idx + 1), 0);
		SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, LB_SETCURSEL, static_cast<WPARAM>(sel), 0);
	}
}

EXPORT_CPP void _listViewAdd(SClass* me_, const U8* text, S64 img)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(img < -1, 0xe9170006);
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = ListView_GetItemCount(wnd);
	item.iSubItem = 0;
	item.iImage = static_cast<int>(img);
	item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertItem(wnd, &item);
}

EXPORT_CPP void _listViewAddColumn(SClass* me_, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	LVCOLUMN lvcolumn;
	lvcolumn.mask = LVCF_TEXT;
	lvcolumn.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertColumn(wnd, Header_GetItemCount(ListView_GetHeader(wnd)), &lvcolumn);
	ListViewAdjustWidth(wnd);
}

EXPORT_CPP void _listViewAdjustWidth(SClass* me_)
{
	ListViewAdjustWidth(reinterpret_cast<SWndBase*>(me_)->WndHandle);
}

EXPORT_CPP void _listViewClear(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	ListView_DeleteAllItems(me2->WndHandle);
}

EXPORT_CPP void _listViewClearAll(SClass* me_)
{
	S64 len = _listViewLenColumn(me_);
	for (S64 i = 0; i < len; i++)
		_listViewDelColumn(me_, 0);
	_listViewClear(me_);
}

EXPORT_CPP void _listViewDel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	ListView_DeleteItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx));
}

EXPORT_CPP void _listViewDelColumn(SClass* me_, S64 column)
{
#if defined(DBG)
	S64 len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, 0xe9170006);
#endif
	ListView_DeleteColumn(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(column));
}

EXPORT_CPP void _listViewDraggable(SClass* me_, bool enabled)
{
	reinterpret_cast<SListView*>(me_)->Draggable = enabled;
}

EXPORT_CPP Bool _listViewGetChk(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	return ListView_GetCheckState(reinterpret_cast<SWndBase*>(me_)->WndHandle, idx) != 0;
}

EXPORT_CPP S64 _listViewGetSel(SClass* me_)
{
	return static_cast<S64>(ListView_GetNextItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, -1, LVNI_ALL | LVNI_SELECTED));
}

EXPORT_CPP Bool _listViewGetSelMulti(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	return ListView_GetItemState(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx), LVIS_SELECTED) != 0;
}

EXPORT_CPP void* _listViewGetText(SClass* me_, S64* img, S64 idx, S64 column)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
	len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, 0xe9170006);
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

EXPORT_CPP void _listViewIns(SClass* me_, S64 idx, const U8* text, S64 img)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(img < -1, 0xe9170006);
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = static_cast<int>(idx);
	item.iSubItem = 0;
	item.iImage = static_cast<int>(img);
	item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, &item);
}

EXPORT_CPP void _listViewInsColumn(SClass* me_, S64 column, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
#if defined(DBG)
	S64 len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, 0xe9170006);
#endif
	HWND wnd = reinterpret_cast<SWndBase*>(me_)->WndHandle;
	LVCOLUMN lvcolumn;
	lvcolumn.mask = LVCF_TEXT;
	lvcolumn.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_InsertColumn(wnd, static_cast<int>(column), &lvcolumn);
	ListViewAdjustWidth(wnd);
}

EXPORT_CPP S64 _listViewLen(SClass* me_)
{
	return static_cast<S64>(ListView_GetItemCount(reinterpret_cast<SWndBase*>(me_)->WndHandle));
}

EXPORT_CPP S64 _listViewLenColumn(SClass* me_)
{
	S64 len = static_cast<S64>(Header_GetItemCount(ListView_GetHeader(reinterpret_cast<SWndBase*>(me_)->WndHandle)));
	if (len == 0)
		len = 1;
	return len;
}

EXPORT_CPP void _listViewSetChk(SClass* me_, S64 idx, bool value)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	ListView_SetCheckState(reinterpret_cast<SWndBase*>(me_)->WndHandle, idx, value ? TRUE : FALSE);
}

EXPORT_CPP void _listViewSetSel(SClass* me_, S64 idx)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	ListView_SetItemState(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx), LVIS_SELECTED, LVIS_SELECTED);
}

EXPORT_CPP void _listViewSetSelMulti(SClass* me_, S64 idx, Bool value)
{
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
#endif
	ListView_SetItemState(reinterpret_cast<SWndBase*>(me_)->WndHandle, static_cast<int>(idx), LVIS_SELECTED, value ? LVIS_SELECTED : 0);
}

EXPORT_CPP void _listViewSetText(SClass* me_, S64 idx, S64 column, const U8* text, S64 img)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(img < -1, 0xe9170006);
#if defined(DBG)
	S64 len = _listViewLen(me_);
	THROWDBG(idx < 0 || len <= idx, 0xe9170006);
	len = _listViewLenColumn(me_);
	THROWDBG(column < 0 || len <= column, 0xe9170006);
#endif
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = static_cast<int>(idx);
	item.iSubItem = static_cast<int>(column);
	item.iImage = static_cast<int>(img);
	item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(text + 0x10));
	ListView_SetItem(reinterpret_cast<SWndBase*>(me_)->WndHandle, &item);
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
	InvalidateRect(me2->WndHandle, nullptr, TRUE);
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

EXPORT_CPP void _tabAdd(SClass* me_, const U8* text)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	S64 cnt = _tabLen(me_);
	TC_ITEM item = { 0 };
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<Char*>(text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10));
	SendMessage(me2->WndHandle, TCM_INSERTITEM, static_cast<WPARAM>(cnt), reinterpret_cast<LPARAM>(&item));
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

EXPORT_CPP S64 _tabGetSel(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	return static_cast<S64>(SendMessage(me2->WndHandle, TCM_GETCURSEL, 0, 0));
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
		THROWDBG(idx < -1 || len <= idx, 0xe9170006);
	}
#endif
	SendMessage(reinterpret_cast<SWndBase*>(me_)->WndHandle, TCM_SETCURSEL, static_cast<WPARAM>(idx), 0);
}

EXPORT_CPP void _treeAllowDraggingToRoot(SClass* me_, Bool enabled)
{
	reinterpret_cast<STree*>(me_)->AllowDraggingToRoot = enabled;
}

EXPORT_CPP void _treeClear(SClass* me_)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	TreeView_DeleteAllItems(me2->WndHandle);
}

EXPORT_CPP void _treeDraggable(SClass* me_, Bool enabled)
{
	reinterpret_cast<STree*>(me_)->Draggable = enabled;
}

EXPORT_CPP void _treeExpand(SClass* me_, Bool expand)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	HTREEITEM root = TreeView_GetRoot(me2->WndHandle);
	if (root != nullptr)
		TreeExpandAllRecursion(me2->WndHandle, root, expand ? TVE_EXPAND : TVE_COLLAPSE);
}

EXPORT_CPP SClass* _treeGetSel(SClass* me_, SClass* me2)
{
	SWndBase* me3 = reinterpret_cast<SWndBase*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	me4->WndHandle = me3->WndHandle;
	me4->Item = TreeView_GetSelection(me3->WndHandle);
	if (me4->Item == nullptr)
		return nullptr;
	return me2;
}

EXPORT_CPP SClass* _treeRoot(SClass* me_, SClass* me2)
{
	SWndBase* me3 = reinterpret_cast<SWndBase*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	me4->WndHandle = me3->WndHandle;
	me4->Item = nullptr;
	return me2;
}

EXPORT_CPP void _treeSetSel(SClass* me_, SClass* node)
{
	SWndBase* me2 = reinterpret_cast<SWndBase*>(me_);
	STreeNode* node2 = reinterpret_cast<STreeNode*>(node);
	THROWDBG(me2->WndHandle != node2->WndHandle, 0xe9170006);
	TreeView_Select(me2->WndHandle, node2 == nullptr ? nullptr : node2->Item, TVGN_CARET);
}

EXPORT_CPP SClass* _treeNodeAddChild(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	TVINSERTSTRUCT tvis;
	tvis.hParent = me3->Item == nullptr ? TVI_ROOT : me3->Item;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(name + 0x10));
	me4->Item = TreeView_InsertItem(me3->WndHandle, &tvis);
	if (me4->Item == nullptr)
		return nullptr;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP void _treeNodeDelChild(SClass* me_, SClass* node)
{
	THROWDBG(node == nullptr, EXCPT_ACCESS_VIOLATION);
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* node2 = reinterpret_cast<STreeNode*>(node);
	if (node2->Item == nullptr)
		return;
	if (me3->Item == nullptr)
		TreeView_DeleteItem(me3->WndHandle, node2->Item);
}

EXPORT_CPP SClass* _treeNodeFirstChild(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == nullptr)
		me4->Item = TreeView_GetRoot(me3->WndHandle);
	else
		me4->Item = TreeView_GetChild(me3->WndHandle, me3->Item);
	if (me4->Item == nullptr)
		return nullptr;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP void* _treeNodeGetName(SClass* me_)
{
	STreeNode* me2 = reinterpret_cast<STreeNode*>(me_);
	if (me2->Item == nullptr)
		return nullptr;
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

EXPORT_CPP SClass* _treeNodeInsChild(SClass* me_, SClass* me2, SClass* node, const U8* name)
{
	THROWDBG(node == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	STreeNode* node2 = reinterpret_cast<STreeNode*>(node);
	TVINSERTSTRUCT tvis;
	tvis.hParent = me3->Item == nullptr ? TVI_ROOT : me3->Item;
	HTREEITEM prev = TreeView_GetPrevSibling(me3->WndHandle, node2->Item);
	tvis.hInsertAfter = prev == nullptr ? TVI_FIRST : prev;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = const_cast<Char*>(reinterpret_cast<const Char*>(name + 0x10));
	me4->Item = TreeView_InsertItem(me3->WndHandle, &tvis);
	if (me4->Item == nullptr)
		return nullptr;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _treeNodeNext(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == nullptr)
		return nullptr;
	me4->Item = TreeView_GetNextSibling(me3->WndHandle, me3->Item);
	if (me4->Item == nullptr)
		return nullptr;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _treeNodeParent(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == nullptr)
		return nullptr;
	me4->Item = TreeView_GetParent(me3->WndHandle, me3->Item);
	if (me4->Item == nullptr)
		return nullptr;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _treeNodePrev(SClass* me_, SClass* me2)
{
	STreeNode* me3 = reinterpret_cast<STreeNode*>(me_);
	STreeNode* me4 = reinterpret_cast<STreeNode*>(me2);
	if (me3->Item == nullptr)
		return nullptr;
	me4->Item = TreeView_GetPrevSibling(me3->WndHandle, me3->Item);
	if (me4->Item == nullptr)
		return nullptr;
	me4->WndHandle = me3->WndHandle;
	return me2;
}

EXPORT_CPP SClass* _makeBtn(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SBtn* me2 = reinterpret_cast<SBtn*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Btn, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP, x, y, width, height, text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcBtn, anchorX, anchorY);
	me2->OnPush = nullptr;
	return me_;
}

EXPORT_CPP SClass* _makeChk(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SChk* me2 = reinterpret_cast<SChk*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Chk, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, width, height, text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcChk, anchorX, anchorY);
	me2->OnPush = nullptr;
	return me_;
}

EXPORT_CPP SClass* _makeCombo(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Combo, WC_COMBOBOX, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST, x, y, width, height, L"", WndProcCombo, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeEdit(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SEdit* me2 = reinterpret_cast<SEdit*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Edit, WC_EDIT, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NOHIDESEL, x, y, width, height, L"", WndProcEdit, anchorX, anchorY);
	reinterpret_cast<SEditBase*>(me2)->OnChange = nullptr;
	reinterpret_cast<SEditBase*>(me2)->OnFocus = nullptr;
	return me_;
}

EXPORT_CPP SClass* _makeEditMulti(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SEditMulti* me2 = reinterpret_cast<SEditMulti*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_EditMulti, WC_EDIT, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL, x, y, width, height, L"", WndProcEditMulti, anchorX, anchorY);
	reinterpret_cast<SEditBase*>(me2)->OnChange = nullptr;
	reinterpret_cast<SEditBase*>(me2)->OnFocus = nullptr;
	return me_;
}

EXPORT_CPP SClass* _makeGroup(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Group, WC_BUTTON, WS_EX_TRANSPARENT, WS_VISIBLE | WS_CHILD | BS_GROUPBOX, x, y, width, height, text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcGroup, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeLabel(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Label, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_SIMPLE, x, y, width, height, text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcLabel, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SList* me2 = reinterpret_cast<SList*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_List, WC_LISTBOX, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_DISABLENOSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, x, y, width, height, L"", WndProcList, anchorX, anchorY);
	me2->OnSel = nullptr;
	me2->OnMouseDoubleClick = nullptr;
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
	me2->OnSel = nullptr;
	me2->OnMouseDoubleClick = nullptr;
	me2->OnMouseClick = nullptr;
	me2->OnMoveNode = nullptr;
	me2->Draggable = False;
	me2->Dragging = False;
	me2->DraggingImage = nullptr;

	if (small_imgs != nullptr)
		ListView_SetImageList(wnd, CreateImageList(small_imgs), LVSIL_SMALL);
	if (large_imgs != nullptr)
		ListView_SetImageList(wnd, CreateImageList(large_imgs), LVSIL_NORMAL);

	return me_;
}

EXPORT_CPP SClass* _makeRadio(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text)
{
	SRadio* me2 = reinterpret_cast<SRadio*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Radio, WC_BUTTON, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON, x, y, width, height, text == nullptr ? L"" : reinterpret_cast<const Char*>(text + 0x10), WndProcRadio, anchorX, anchorY);
	me2->OnPush = nullptr;
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

EXPORT_CPP SClass* _makeTab(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Tab, WC_TABCONTROL, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN | TCS_BUTTONS | TCS_FLATBUTTONS, x, y, width, height, L"", WndProcTab, anchorX, anchorY);
	return me_;
}

EXPORT_CPP SClass* _makeTree(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY)
{
	STree* me2 = reinterpret_cast<STree*>(me_);
	SetCtrlParam(reinterpret_cast<SWndBase*>(me_), reinterpret_cast<SWndBase*>(parent), WndKind_Tree, WC_TREEVIEW, WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_LINESATROOT, x, y, width, height, L"", WndProcTree, anchorX, anchorY);
	me2->Draggable = False;
	me2->AllowDraggingToRoot = False;
	me2->DraggingItem = nullptr;
	me2->OnSel = nullptr;
	me2->OnMoveNode = nullptr;
	return me_;
}

static void ListViewAdjustWidth(HWND wnd)
{
	int cnt = Header_GetItemCount(ListView_GetHeader(wnd));
	for (int i = 0; i < cnt; i++)
		ListView_SetColumnWidth(wnd, i, LVSCW_AUTOSIZE_USEHEADER);
}

static void TreeExpandAllRecursion(HWND wnd_handle, HTREEITEM node, int flag)
{
	TreeView_Expand(wnd_handle, node, flag);

	HTREEITEM child = TreeView_GetChild(wnd_handle, node);
	while (child != nullptr)
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
	if (new_item == nullptr)
		return;
	HTREEITEM child = TreeView_GetChild(tree_wnd, src);
	while (child != nullptr)
	{
		CopyTreeNodeRecursion(tree_wnd, new_item, child, buf);
		child = TreeView_GetNextSibling(tree_wnd, child);
	}
}

static HIMAGELIST CreateImageList(const void* imgs)
{
	S64 len = static_cast<const S64*>(imgs)[1];
	HIMAGELIST result = nullptr;
	const void* const* ptr = reinterpret_cast<void* const*>(static_cast<const U8*>(imgs) + 0x10);
	for (S64 i = 0; i < len; i++)
	{
		size_t size = static_cast<size_t>(*reinterpret_cast<const S64*>(static_cast<const U8*>(ptr[i]) + 0x08));
		const void* bin = static_cast<const U8*>(ptr[i]) + 0x10;
		int width;
		int height;
		void* img = DecodePng(size, bin, &width, &height);
		if (result == nullptr)
			result = ImageList_Create(width, height, ILC_COLOR32, static_cast<int>(len), 0);
		BITMAPINFO info = { 0 };
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biWidth = static_cast<LONG>(width);
		info.bmiHeader.biHeight = static_cast<LONG>(height);
		void* raw;
		HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &raw, nullptr, 0);
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
		ImageList_Add(result, bitmap, nullptr);
		DeleteObject(bitmap);
		FreeMem(img);
	}
	return result;
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

static LRESULT CALLBACK WndProcList(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	SList* wnd3 = reinterpret_cast<SList*>(wnd2);
	ASSERT(wnd2->Kind == WndKind_List);
	switch (msg)
	{
		case WM_LBUTTONDBLCLK:
			if (wnd3->OnMouseDoubleClick != nullptr)
				Call1Asm(IncWndRef(reinterpret_cast<SClass*>(wnd2)), wnd3->OnMouseDoubleClick);
			return 0;
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
			if (wnd3->OnMouseDoubleClick != nullptr)
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
				wnd3->DraggingImage = nullptr;
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

static LRESULT CALLBACK WndProcScrollX(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	SWndBase* wnd2 = ToWnd(wnd);
	ASSERT(wnd2->Kind == WndKind_ScrollX);
	switch (msg)
	{
		case WM_SETCURSOR:
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
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
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
			return 1;
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
			if (wnd3->DraggingItem != nullptr)
			{
				POINT point;
				TVHITTESTINFO hit_test = { 0 };
				ImageList_DragLeave(nullptr);
				GetCursorPos(&point);
				hit_test.flags = TVHT_ONITEM;
				hit_test.pt.x = point.x;
				hit_test.pt.y = point.y;
				ScreenToClient(wnd, &hit_test.pt);
				HTREEITEM item = reinterpret_cast<HTREEITEM>(SendMessage(wnd, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hit_test)));
				if (item != nullptr)
				{
					ImageList_DragShowNolock(FALSE);
					SendMessage(wnd, TVM_SELECTITEM, TVGN_DROPHILITE, reinterpret_cast<LPARAM>(item));
					ImageList_DragShowNolock(TRUE);
				}
				ImageList_DragMove(point.x, point.y);
				ImageList_DragEnter(nullptr, point.x, point.y);
			}
			return 0;
		case WM_LBUTTONUP:
			if (wnd3->DraggingItem != nullptr)
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
				if (wnd3->DraggingItem != item && (wnd3->AllowDraggingToRoot || item != nullptr))
				{
					Bool success = True;
					{
						// Prohibit dropping a node on one of its children.
						HTREEITEM item2 = item;
						while (item2 != nullptr)
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
				ImageList_DragLeave(nullptr);
				ImageList_EndDrag();
				wnd3->DraggingItem = nullptr;
			}
			return 0;
	}
	return CallWindowProc(wnd2->DefaultWndProc, wnd, msg, w_param, l_param);
}
