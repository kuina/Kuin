#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double srcX, double srcY, double srcW, double srcH, S64 color);
EXPORT_CPP void _texDrawRot(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, double centerX, double centerY, double angle, S64 color);
EXPORT_CPP void _texDrawScale(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, S64 color);
EXPORT_CPP void _texFin(SClass* me_);
EXPORT_CPP S64 _texHeight(SClass* me_);
EXPORT_CPP S64 _texImgHeight(SClass* me_);
EXPORT_CPP S64 _texImgWidth(SClass* me_);
EXPORT_CPP S64 _texWidth(SClass* me_);
EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* data, const U8* path);
EXPORT_CPP SClass* _makeTexArgb(SClass* me_, const U8* data, const U8* path);
EXPORT_CPP SClass* _makeTexEvenArgb(SClass* me_, double a, double r, double g, double b);
EXPORT_CPP SClass* _makeTexEvenColor(SClass* me_, S64 color);
