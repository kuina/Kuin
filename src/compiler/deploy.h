#pragma once

#include "..\common.h"

#include "dict.h"
#include "option.h"

void Deploy(U64 app_code, const SOption* option, SDict* dlls, const void* related_files);
