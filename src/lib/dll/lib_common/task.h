#pragma once

#include "..\common.h"

EXPORT S64 _processRun(SClass* me_, Bool wait_until_exit);
EXPORT void _processFin(SClass* me_);
EXPORT Bool _processRunning(SClass* me_, S64* exit_code);
EXPORT void* _processReadPipe(SClass* me_);
EXPORT SClass* _makeProcess(SClass* me_, const U8* path, const U8* cmd_line);
EXPORT void _taskOpen(const U8* path, S64 mode, Bool wait_until_exit);
