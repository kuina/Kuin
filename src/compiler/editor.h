#pragma once

#include "..\common.h"

EXPORT void EditorInit(SClass* me_, void* func_ins, void* func_cmd, void* func_replace, SClass* scroll_x, SClass* scroll_y);
EXPORT void EditorFin(void);
EXPORT void EditorSetSrc(const void* src);
EXPORT void EditorSetCursor(S64* newX, S64* newY, S64 x, S64 y, Bool refresh);
