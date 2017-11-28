#pragma once

#include "..\common.h"

// 'math'
EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const Char* app_name);
EXPORT S64 _gcd(S64 a, S64 b);
EXPORT S64 _lcm(S64 a, S64 b);
EXPORT S64 _modPow(S64 value, S64 exponent, S64 modulus);
EXPORT S64 _modMul(S64 a, S64 b, S64 modulus);
EXPORT Bool _prime(S64 n);
EXPORT void* _primeFactors(S64 n);
EXPORT double _gamma(double n);
EXPORT double _fact(double n);
EXPORT S64 _factInt(S64 n);
EXPORT SClass* _makeMat(SClass* me_, S64 row, S64 col);
EXPORT void _matDtor(SClass* me_);
