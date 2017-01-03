#pragma once

#include "..\common.h"

#include "pos.h"

Bool SetLogFunc(void(*log_func)(const Char* code, const Char* msg, const Char* src, int row, int col), int lang, const Char* sys_dir);
void Err(const Char* code, const SPos* pos, ...);
void ResetErrOccurred(void);
Bool ErrOccurred(void);
