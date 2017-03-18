#pragma once

#include "..\common.h"

EXPORT void EditorInit(SClass* me_, void* func_ins, void* func_cmd, void* func_replace);
EXPORT void EditorFin(void);
EXPORT void EditorSetSrc(const void* src);
