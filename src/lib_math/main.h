#pragma once

#include "..\common.h"

// 'math'
EXPORT void init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT S64 _gcd(S64 a, S64 b);
EXPORT S64 _lcm(S64 a, S64 b);
EXPORT S64 _modPow(S64 value, S64 exponent, S64 modulus);
EXPORT S64 _modMul(S64 a, S64 b, S64 modulus);
EXPORT Bool _prime(S64 n);
EXPORT void* _primeFactors(S64 n);
