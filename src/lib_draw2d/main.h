#pragma once

#include "..\common.h"

// 'draw2d'
EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT_CPP void _fin();
EXPORT_CPP void _line(double x1, double y1, double x2, double y2, double stroke_width, S64 color);
EXPORT_CPP void _rect(double x, double y, double width, double height, S64 color);
EXPORT_CPP void _rectLine(double x, double y, double width, double height, double stroke_width, S64 color);
EXPORT_CPP void _circle(double x, double y, double radius_x, double radius_y, S64 color);
EXPORT_CPP void _circleLine(double x, double y, double radius_x, double radius_y, double stroke_width, S64 color);
EXPORT_CPP void _roundRect(double x, double y, double width, double height, double radius_x, double radius_y, S64 color);
EXPORT_CPP void _roundRectLine(double x, double y, double width, double height, double radius_x, double radius_y, double stroke_width, S64 color);
EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color);
EXPORT_CPP SClass* _makeBrushLinearGradient(SClass* me_, double x1, double y1, double x2, double y2, void* color_position, void* color);
EXPORT_CPP void _brushLinearGradientDtor(SClass* me_);
EXPORT_CPP void _brushLine(SClass* me_, double x1, double y1, double x2, double y2, double stroke_width, SClass* stroke_style);
EXPORT_CPP SClass* _makeGeometryPath(SClass* me_);
EXPORT_CPP void _geometryPathDtor(SClass* me_);
EXPORT_CPP void _geometryPathOpen(SClass* me_);
EXPORT_CPP void _geometryPathClose(SClass* me_);
EXPORT_CPP void _geometryPathOpenFigure(SClass* me_, double x, double y, Bool filled);
EXPORT_CPP void _geometryPathCloseFigure(SClass* me_, Bool closed_path);
EXPORT_CPP void _geometryPathAddArc(SClass* me_, double x, double y, double radius_x, double radius_y, double angle, Bool ccw, Bool large_arc);
EXPORT_CPP void _geometryPathAddBezier(SClass* me_, double x1, double y1, double x2, double y2, double x3, double y3);
EXPORT_CPP void _geometryPathAddLine(SClass* me_, double x, double y);
EXPORT_CPP void _geometryPathAddQuadraticBezier(SClass* me_, double x1, double y1, double x2, double y2);
EXPORT_CPP void _geometryDraw(SClass* me_, S64 color);
EXPORT_CPP void _geometryDrawLine(SClass* me_, double stroke_width, S64 color);
