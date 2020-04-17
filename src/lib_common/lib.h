#pragma once

#include "..\common.h"

// 'lib'
EXPORT void* _cmdLine(void);
EXPORT S64 _rnd(S64 min, S64 max);
EXPORT double _rndFloat(double min, double max);
EXPORT U64 _rndBit64(void);
EXPORT void* _rndUuid(void);
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
EXPORT double _log(double base, double x);
EXPORT double _floor(double x);
EXPORT double _ceil(double x);
EXPORT double _round(double x, S64 precision);
EXPORT void _rot(double* x, double* y, double centerX, double centerY, double angle);
EXPORT double _invRot(double x, double y, double centerX, double centerY);
EXPORT double _dist(double x, double y, double centerX, double centerY);
EXPORT Bool _chase(double* x, double target, double vel);
EXPORT Bool _same(double n1, double n2);
EXPORT double _toRad(double degree);
EXPORT double _toDegree(double rad);
EXPORT S64 _cmp(const U8* s1, const U8* s2);
EXPORT S64 _cmpEx(const U8* s1, const U8* s2, S64 len, Bool ignoreCase);
EXPORT S64 _minInt(S64 n1, S64 n2);
EXPORT double _minFloat(double n1, double n2);
EXPORT S64 _maxInt(S64 n1, S64 n2);
EXPORT double _maxFloat(double n1, double n2);
EXPORT S64 _levenshtein(const U8* s1, const U8* s2);
EXPORT double _lerp(double first, double last, double rate);
EXPORT double _qerp(double first, double last, Bool easeIn, double rate);
EXPORT double _cerp(double first, double last, double rate);
EXPORT double _hermite(const void* pos, double rate);
EXPORT SClass* _makeBmSearch(SClass* me_, const U8* pattern);
EXPORT void _bmSearchDtor(SClass* me_);
EXPORT S64 _bmSearchFind(SClass* me_, const U8* text, S64 start);
EXPORT void* _countUp(S64 min, S64 max);
EXPORT S64 _addChkOverflow(Bool* overflowed, S64 n1, S64 n2);
EXPORT S64 _subChkOverflow(Bool* overflowed, S64 n1, S64 n2);
EXPORT S64 _mulChkOverflow(Bool* overflowed, S64 n1, S64 n2);
EXPORT U64 _addr(SClass* me_);
EXPORT U64 _toBit64Forcibly(double x);
EXPORT double _toFloatForcibly(U64 x);

void LibInit(void);

// Assembly functions.
Bool AddAsm(S64* a, S64 b);
Bool SubAsm(S64* a, S64 b);
Bool MulAsm(S64* a, S64 b);
