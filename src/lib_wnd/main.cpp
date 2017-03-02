// LibWin.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "draw.h"
#include "snd.h"
#include "input.h"

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

struct SWnd
{
	SClass Class;
	EWndKind Kind;
	void* Mfc;
	void* DrawBuf;
};

static HMODULE MfcHandle;
static void(*MfcInit)();
static void(*MfcFin)();
static Bool(*MfcAct)();
static void*(*MfcMakeWnd)(S64 kind, void* parent, S64 x, S64 y, S64 width, S64 height, S64 anchor_num, const S64* anchor, const Char* text);
static void(*MfcShowWnd)(void* wnd);
static void(*MfcDestroyWnd)(void* wnd);
static HWND(*MfcGetHwnd)(void* ptr);

static Bool ExitAct;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : reinterpret_cast<const Char*>(app_name + 0x10);
	Instance = static_cast<HINSTANCE>(GetModuleHandle(NULL));

	MfcHandle = LoadLibrary(L"d0003.knd");
	if (MfcHandle == NULL)
		THROW(0x1000, L"");
	MfcInit = reinterpret_cast<void(*)()>(GetProcAddress(MfcHandle, "_init"));
	if (MfcInit == NULL)
		THROW(0x1000, L"");
	MfcFin = reinterpret_cast<void(*)()>(GetProcAddress(MfcHandle, "_fin"));
	if (MfcFin == NULL)
		THROW(0x1000, L"");
	MfcAct = reinterpret_cast<Bool(*)()>(GetProcAddress(MfcHandle, "_act"));
	if (MfcAct == NULL)
		THROW(0x1000, L"");
	MfcMakeWnd = reinterpret_cast<void*(*)(S64, void*, S64, S64, S64, S64, S64, const S64*, const Char*)>(GetProcAddress(MfcHandle, "_makeWnd"));
	if (MfcMakeWnd == NULL)
		THROW(0x1000, L"");
	MfcShowWnd = reinterpret_cast<void(*)(void*)>(GetProcAddress(MfcHandle, "_showWnd"));
	if (MfcShowWnd == NULL)
		THROW(0x1000, L"");
	MfcDestroyWnd = reinterpret_cast<void(*)(void*)>(GetProcAddress(MfcHandle, "_destroyWnd"));
	if (MfcDestroyWnd == NULL)
		THROW(0x1000, L"");
	MfcGetHwnd = reinterpret_cast<HWND(*)(void*)>(GetProcAddress(MfcHandle, "_getHwnd"));
	if (MfcGetHwnd == NULL)
		THROW(0x1000, L"");

	ExitAct = False;

	MfcInit();
	Draw::Init();
	Snd::Init();
	Input::Init();

	// TODO:
}

EXPORT_CPP void _fin()
{
	Input::Fin();
	Snd::Fin();
	Draw::Fin();
	MfcFin();

	if (MfcHandle != NULL)
		FreeLibrary(MfcHandle);

	// TODO:
}

EXPORT_CPP Bool _act()
{
	if (ExitAct)
		return False;

	if (!MfcAct())
	{
		ExitAct = True;
		return False;
	}

	Input::Update();

	Sleep(1);

	return True;
}

EXPORT_CPP SClass* _makeWnd(SClass* me_, SClass* parent, S64 style, S64 width, S64 height, const U8* text)
{
	ASSERT(0 <= style && style <= 6);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	me2->Kind = static_cast<EWndKind>(static_cast<S64>(WndKind_WndNormal) + style);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_WndNormal) + style, parent2 == NULL ? NULL : parent2->Mfc, 0, 0, width, height, 0, NULL, text == NULL ? AppName : reinterpret_cast<const Char*>(text + 0x10));
	me2->DrawBuf = NULL;
	return me_;
}

EXPORT_CPP void _wndDtor(SClass* me_)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	if (me2->DrawBuf != NULL)
		Draw::FinDrawBuf(me2->DrawBuf);
	if (me2->Mfc != NULL)
		MfcDestroyWnd(me2->Mfc);
}

EXPORT_CPP SClass* _wndShow(SClass* me_)
{
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	ASSERT(static_cast<S64>(me2->Kind) < 0x80);
	MfcShowWnd(me2->Mfc);
	return me_;
}

EXPORT_CPP SClass* _makeDraw(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor)
{
	ASSERT(parent != NULL);
	ASSERT(width >= 0 && height >= 0);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	me2->Kind = WndKind_Draw;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Draw), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, NULL);
	me2->DrawBuf = Draw::MakeDrawBuf(static_cast<int>(width), static_cast<int>(height), MfcGetHwnd(me2->Mfc));
	return me_;
}

EXPORT_CPP SClass* _makeBtn(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor, const U8* text)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Btn;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Btn), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, reinterpret_cast<const Char*>(text + 0x10));
	return me_;
}

EXPORT_CPP SClass* _makeChk(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor, const U8* text)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Chk;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Chk), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, reinterpret_cast<const Char*>(text + 0x10));
	return me_;
}

EXPORT_CPP SClass* _makeRadio(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor, const U8* text)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Radio;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Radio), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, reinterpret_cast<const Char*>(text + 0x10));
	return me_;
}

EXPORT_CPP SClass* _makeEdit(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Edit;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Edit), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, NULL);
	return me_;
}

EXPORT_CPP SClass* _makeEditMulti(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_EditMulti;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_EditMulti), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, NULL);
	return me_;
}

EXPORT_CPP SClass* _makeList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_List;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_List), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, NULL);
	return me_;
}

EXPORT_CPP SClass* _makeCombo(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Combo;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Combo), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, NULL);
	return me_;
}

EXPORT_CPP SClass* _makeComboList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_ComboList;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_ComboList), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, NULL);
	return me_;
}

EXPORT_CPP SClass* _makeLabel(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor, const U8* text)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Label;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Label), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, reinterpret_cast<const Char*>(text + 0x10));
	return me_;
}

EXPORT_CPP SClass* _makeGroup(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, const U8* anchor, const U8* text)
{
	ASSERT(width >= 0 && height >= 0);
	SWnd* parent2 = reinterpret_cast<SWnd*>(parent);
	SWnd* me2 = reinterpret_cast<SWnd*>(me_);
	me2->Kind = WndKind_Group;
	S64 anchor_num = anchor == NULL ? 0 : *reinterpret_cast<const S64*>(anchor + 0x08);
	const S64* anchor_ptr = anchor_num == 0 ? NULL : reinterpret_cast<const S64*>(anchor + 0x10);
	me2->Mfc = MfcMakeWnd(static_cast<S64>(WndKind_Group), parent2->Mfc, x, y, width, height, anchor_num, anchor_ptr, reinterpret_cast<const Char*>(text + 0x10));
	return me_;
}
