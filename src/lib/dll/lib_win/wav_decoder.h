#pragma once

#include "..\common.h"

void* LoadWav(size_t size, const U8* data, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64));
