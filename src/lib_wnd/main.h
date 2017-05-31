#pragma once

#include "..\common.h"

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT_CPP void _fin();
EXPORT_CPP Bool _act();
EXPORT_CPP void _onKeyPress(void* onKeyPressFunc);
EXPORT_CPP S64 _msgBox(SClass* parent, const U8* text, const U8* title, S64 icon, S64 btn);
EXPORT_CPP void* _openFileDialog(SClass* parent, const U8* filter, S64 defaultFilter);
EXPORT_CPP void* _openFileDialogMulti(SClass* parent, const U8* filter, S64 defaultFilter);
EXPORT_CPP void* _saveFileDialog(SClass* parent, const U8* filter, S64 defaultFilter, const U8* defaultExt);
EXPORT_CPP void _setClipboardStr(const U8* str);
EXPORT_CPP void* _getClipboardStr();
EXPORT_CPP SClass* _makeWnd(SClass* me_, SClass* parent, S64 style, S64 width, S64 height, const U8* text);
EXPORT_CPP void _wndBaseDtor(SClass* me_);
EXPORT_CPP void _wndBaseGetSize(SClass* me_, S64* width, S64* height);
EXPORT_CPP void _wndBasePaint(SClass* me_);
EXPORT_CPP void _wndMinMax(SClass* me_, S64 minWidth, S64 minHeight, S64 maxWidth, S64 maxHeight);
EXPORT_CPP void _wndClose(SClass* me_);
EXPORT_CPP void _wndExit(SClass* me_);
EXPORT_CPP void _wndSetText(SClass* me_, const U8* text);
EXPORT_CPP const U8* _wndGetText(SClass* me_);
EXPORT_CPP void _wndReadonly(SClass* me_, Bool flag);
EXPORT_CPP void _wndSetMenu(SClass* me_, SClass* menu);
EXPORT_CPP Bool _wndActive(SClass* me_);
EXPORT_CPP SClass* _makeDraw(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, Bool equalMagnification);
EXPORT_CPP void _drawDtor(SClass* me_);
EXPORT_CPP void _drawShowCaret(SClass* me_, S64 height, SClass* font);
EXPORT_CPP void _drawHideCaret(SClass* me_);
EXPORT_CPP void _drawMoveCaret(SClass* me_, S64 x, S64 y);
EXPORT_CPP SClass* _makeBtn(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text);
EXPORT_CPP void _btnSetChk(SClass* me_, Bool chk);
EXPORT_CPP Bool _btnGetChk(SClass* me_);
EXPORT_CPP SClass* _makeChk(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text);
EXPORT_CPP SClass* _makeRadio(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text);
EXPORT_CPP SClass* _makeEdit(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeEditMulti(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP void _listClear(SClass* me_);
EXPORT_CPP void _listAdd(SClass* me_, const U8* text);
EXPORT_CPP SClass* _makeCombo(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeComboList(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeLabel(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text);
EXPORT_CPP SClass* _makeGroup(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, const U8* text);
EXPORT_CPP SClass* _makeCalendar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeProgress(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeRebar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeStatus(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeToolbar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeTrackbar(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeLabelLink(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeListView(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makePager(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeTab(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeTree(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeSplitX(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeSplitY(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeScrollX(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP SClass* _makeScrollY(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY);
EXPORT_CPP void _scrollSetState(SClass* me_, S64 min, S64 max, S64 page, S64 value);

EXPORT_CPP SClass* _makeMenu(SClass* me_);
EXPORT_CPP void _menuDtor(SClass* me_);
EXPORT_CPP void _menuAdd(SClass* me_, S64 id, const U8* text);
EXPORT_CPP void _menuAddLine(SClass* me_);
EXPORT_CPP void _menuAddPopup(SClass* me_, const U8* text, const U8* popup);
EXPORT_CPP SClass* _makePopup(SClass* me_);

// Assembly functions.
extern "C" void* Call0Asm(void* func);
extern "C" void* Call1Asm(void* arg1, void* func);
extern "C" void* Call2Asm(void* arg1, void* arg2, void* func);
extern "C" void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);
SClass* IncWndRef(SClass* wnd);
