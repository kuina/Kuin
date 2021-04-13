#pragma once

#include "..\common.h"
#include "wnd_common.h"
#include "draw_common.h"

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

EXPORT_CPP void _drawDtor(SClass* me_);
EXPORT_CPP void _drawPaint(SClass* me_);
EXPORT_CPP void _drawHideCaret(SClass* me_);
EXPORT_CPP void _drawMouseCapture(SClass* me_, Bool enabled);
EXPORT_CPP void _drawMoveCaret(SClass* me_, S64 x, S64 y);
EXPORT_CPP void _drawShowCaret(SClass* me_, S64 height, S64 font_handle);
EXPORT_CPP SClass* _makeDraw(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, Bool equalMagnification);
EXPORT_CPP SClass* _makeDrawEditable(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height);
EXPORT_CPP SClass* _makeDrawReduced(SClass* me_, SClass* parent, S64 x, S64 y, S64 width, S64 height, S64 anchorX, S64 anchorY, Bool equalMagnification, S64 split);
EXPORT_CPP void _target(SClass* draw_ctrl);
