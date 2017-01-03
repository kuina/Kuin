#pragma once

#include "..\common.h"

int BinSearch(const Char** hay_stack, int num, const Char* needle);
const Char* NewStr(int* len, const Char* format, ...);
const Char* SubStr(const Char* str, int start, int len);
S64* NewAddr(void);
U8* StrToBin(const Char* str, int* size);
Bool CmpData(int size1, const U8* data1, int size2, const U8* data2);
void ReadFileLine(Char* buf, int size, FILE* file_ptr);
Char* GetDir(const Char* path, Bool dir, const Char* add_name);
Bool DelDir(const Char* path);
