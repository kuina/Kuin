#pragma once

#include "..\common.h"

#include <commctrl.h>

enum ECtrlFlag
{
	CtrlFlag_AnchorLeft = 0x01,
	CtrlFlag_AnchorTop = 0x02,
	CtrlFlag_AnchorRight = 0x04,
	CtrlFlag_AnchorBottom = 0x08,
};

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

extern HINSTANCE Instance;
extern HFONT FontCtrl;

SClass* IncWndRef(SClass* wnd);
void CommandAndNotify(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
SWndBase* ToWnd(HWND wnd);
BOOL CALLBACK ResizeCallback(HWND wnd, LPARAM l_param);
const U8* NToRN(const Char* str);
const U8* RNToN(const Char* str);
void SetCtrlParam(SWndBase* wnd, SWndBase* parent, EWndKind kind, const Char* ctrl, DWORD style_ex, DWORD style, S64 x, S64 y, S64 width, S64 height, const Char* text, WNDPROC wnd_proc, S64 anchor_x, S64 anchor_y);

// TODO:
/*
EXPORT_CPP void* _openFileDialogMulti(SClass* parent, const U8* filter, S64 defaultFilter);
EXPORT_CPP SClass* _makeCalendar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeProgress(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeRebar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeStatus(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeToolbar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeTrackbar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeLabelLink(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makePager(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeSplitX(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeSplitY(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
*/
