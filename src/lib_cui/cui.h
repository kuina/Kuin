#pragma once

#include "..\common.h"

// 'cui'
EXPORT void _delimiter(const U8* delimiters);
EXPORT void _print(const U8* str);
EXPORT void _flush(void);
EXPORT Char _inputLetter(void);
EXPORT S64 _inputInt(void);
EXPORT double _inputFloat(void);
EXPORT Char _inputChar(void);
EXPORT void* _inputStr(void);
EXPORT void* _input(void);

void InitCui(void);
void FinCui(void);
