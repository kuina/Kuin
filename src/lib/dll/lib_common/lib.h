#pragma once

#include "..\common.h"

EXPORT double _acos(double x);
EXPORT double _acosh(double x);
EXPORT U64 _addr(SClass* me_);
EXPORT double _asin(double x);
EXPORT double _asinh(double x);
EXPORT double _atan(double x);
EXPORT double _atanh(double x);
EXPORT double _ceil(double x);
EXPORT void* _cmdLine(void);
EXPORT double _cos(double x);
EXPORT double _cosh(double x);
EXPORT double _dist(double x, double y, double centerX, double centerY);
EXPORT double _exp(double x);
EXPORT double _floor(double x);
EXPORT double _invRot(double x, double y, double centerX, double centerY);
EXPORT double _ln(double x);
EXPORT S64 _now(void);
EXPORT double _sin(double x);
EXPORT double _sinh(double x);
EXPORT void __sleep(S64 ms);
EXPORT double _sqrt(double x);
EXPORT S64 _sysTime(void);
EXPORT double _tan(double x);
EXPORT double _tanh(double x);
EXPORT U64 _toBit64Forcibly(double x);
EXPORT double _toFloatForcibly(U64 x);
