#pragma once

#include "..\common.h"

#include "dict.h"
#include "option.h"

SDict* Parse(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const SOption* option);
void InterpretImpl1(const void* src, const void* color);
int GetReservedNum(void);
const Char** GetReserved(void);
