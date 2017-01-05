#pragma once

#include "..\common.h"

// 'lib'
EXPORT S64 _rnd(S64 min, S64 max);
EXPORT double _rndFloat(double min, double max);
EXPORT U64 _rndBit64(void);
EXPORT double _cos(double x);
EXPORT double _sin(double x);
EXPORT double _tan(double x);
EXPORT double _sqrt(double x);
EXPORT double _exp(double x);
EXPORT double _ln(double x);
EXPORT double _log(double x, double base);
EXPORT void _rot(double* x, double* y, double centerX, double centerY, double angle);
EXPORT double _invRot(double x, double y);
EXPORT double _dist(double x, double y);
EXPORT Bool _chase(double* x, double target, double vel);
EXPORT double _floor(double x);
EXPORT double _ceil(double x);
void LibInit(void);
