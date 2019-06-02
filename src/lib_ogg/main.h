#pragma once

#include "..\common.h"

#pragma comment(lib, "libogg_static.lib")
#pragma comment(lib, "libvorbis_static.lib")
#pragma comment(lib, "libvorbisfile_static.lib")

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

EXPORT void* LoadOgg(const Char* path, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64), void(**func_set_pos)(void*, S64), S64(**func_get_pos)(void*));
EXPORT void init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
