#pragma once

#include "..\common.h"

// 'lib'
EXPORT void* _cmdLine(void);
EXPORT S64 _rnd(S64 min, S64 max);
EXPORT double _rndFloat(double min, double max);
EXPORT U64 _rndBit64(void);
EXPORT double _cos(double x);
EXPORT double _sin(double x);
EXPORT double _tan(double x);
EXPORT double _acos(double x);
EXPORT double _asin(double x);
EXPORT double _atan(double x);
EXPORT double _cosh(double x);
EXPORT double _sinh(double x);
EXPORT double _tanh(double x);
EXPORT double _acosh(double x);
EXPORT double _asinh(double x);
EXPORT double _atanh(double x);
EXPORT double _sqrt(double x);
EXPORT double _exp(double x);
EXPORT double _ln(double x);
EXPORT double _log(double x, double base);
EXPORT double _floor(double x);
EXPORT double _ceil(double x);
EXPORT double _round(double x, S64 precision);
EXPORT void _rot(double* x, double* y, double centerX, double centerY, double angle);
EXPORT double _invRot(double x, double y, double centerX, double centerY);
EXPORT double _dist(double x, double y, double centerX, double centerY);
EXPORT Bool _chase(double* x, double target, double vel);
void LibInit(void);
