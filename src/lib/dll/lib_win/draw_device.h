#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _drawInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT_CPP void _drawFin();
EXPORT_CPP void _ambLight(double topR, double topG, double topB, double bottomR, double bottomG, double bottomB);
EXPORT_CPP S64 _argbToColor(double a, double r, double g, double b);
EXPORT_CPP void _autoClear(Bool enabled);
EXPORT_CPP void _blend(S64 kind);
EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ);
EXPORT_CPP Bool _capture(const U8* path);
EXPORT_CPP void _clear();
EXPORT_CPP void _clearColor(S64 color);
EXPORT_CPP S64 _cnt();
EXPORT_CPP void _colorToArgb(double* a, double* r, double* g, double* b, S64 color);
EXPORT_CPP void _depth(Bool test, Bool write);
EXPORT_CPP void _dirLight(double atX, double atY, double atZ, double r, double g, double b);
EXPORT_CPP void _editPixels(const void* callback);
EXPORT_CPP void _filterMonotone(S64 color, double rate);
EXPORT_CPP void _filterNone();
EXPORT_CPP void _proj(double fovy, double aspectX, double aspectY, double nearZ, double farZ);
EXPORT_CPP void _render(S64 fps);
EXPORT_CPP void _sampler(S64 kind);
EXPORT_CPP S64 _screenHeight();
EXPORT_CPP S64 _screenWidth();

// This method is provided for the 'draw2d' library.
EXPORT_CPP void _set2dCallback(void* (*callback)(int, void*, void*));
