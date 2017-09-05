#pragma once

#include "..\common.h"

#pragma comment(lib, "zlibstat.lib")

#include <zlib.h>

EXPORT Bool _zip(const U8* out_path, const U8* path, S64 compression_level);
