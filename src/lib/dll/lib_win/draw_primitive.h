#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _circle(double x, double y, double radiusX, double radiusY, S64 color);
EXPORT_CPP void _circleLine(double x, double y, double radiusX, double radiusY, S64 color);
EXPORT_CPP void _line(double x1, double y1, double x2, double y2, S64 color);
EXPORT_CPP void _rect(double x, double y, double w, double h, S64 color);
EXPORT_CPP void _rectLine(double x, double y, double w, double h, S64 color);
EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color);
