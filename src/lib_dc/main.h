#pragma once

#include "..\common.h"

EXPORT_CPP SClass* _makeDCFromWindowHandle(SClass* me_, S64 hwnd);
EXPORT_CPP void _dcDtor(SClass* me_);
EXPORT_CPP void _dcBegin(SClass* me_);
EXPORT_CPP void _dcEnd(SClass* me_);
EXPORT_CPP void _circleLine(
	SClass* me_, double x, double y, double rx, double ry, double strokeWidth, S64 color);