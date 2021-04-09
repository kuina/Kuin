#pragma once

#include "..\common.h"

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT_CPP void _fin();
EXPORT_CPP void _circle(double x, double y, double radius_x, double radius_y, S64 color);
EXPORT_CPP void _circleLine(double x, double y, double radius_x, double radius_y, double stroke_width, S64 color);
EXPORT_CPP void _line(double x1, double y1, double x2, double y2, double stroke_width, S64 color);
EXPORT_CPP void _rect(double x, double y, double width, double height, S64 color);
EXPORT_CPP void _rectLine(double x, double y, double width, double height, double stroke_width, S64 color);
EXPORT_CPP void _roundRect(double x, double y, double width, double height, double radius_x, double radius_y, S64 color);
EXPORT_CPP void _roundRectLine(double x, double y, double width, double height, double radius_x, double radius_y, double stroke_width, S64 color);
EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color);
