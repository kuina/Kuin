#pragma once

#include "..\common.h"

#include "dict.h"
#include "option.h"

SDict* Parse(FILE*(__cdecl*func_wfopen)(const Char*, const Char*), int(__cdecl*func_fclose)(FILE*), U16(__cdecl*func_fgetwc)(FILE*), size_t(_cdecl*func_size)(FILE*), const SOption* option);
