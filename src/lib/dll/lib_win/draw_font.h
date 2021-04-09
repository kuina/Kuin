#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _fontAlign(SClass* me_, S64 horizontal, S64 vertical);
EXPORT_CPP void _fontCalcSize(SClass* me_, double* width, double* height, const U8* text);
EXPORT_CPP double _fontCalcWidth(SClass* me_, const U8* text);
EXPORT_CPP void _fontDraw(SClass* me_, double dstX, double dstY, const U8* text, S64 color);
EXPORT_CPP void _fontDrawScale(SClass* me_, double dstX, double dstY, double dstScaleX, double dstScaleY, const U8* text, S64 color);
EXPORT_CPP void _fontFin(SClass* me_);
EXPORT_CPP double _fontGetHeight(SClass* me_);
EXPORT_CPP S64 _fontHandle(SClass* me_);
EXPORT_CPP double _fontMaxHeight(SClass* me_);
EXPORT_CPP double _fontMaxWidth(SClass* me_);
EXPORT_CPP void _fontSetHeight(SClass* me_, double height);
EXPORT_CPP SClass* _makeFont(SClass* me_, const U8* fontName, S64 size, bool bold, bool italic, bool proportional, double advance);
