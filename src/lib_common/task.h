#pragma once

#include "..\common.h"

// 'task'
EXPORT SClass* _makeProcess(SClass* me_, const U8* path, const U8* cmd_line);
EXPORT void _processDtor(SClass* me_);
EXPORT S64 _processRun(SClass* me_, Bool waitUntilExit);
EXPORT void _taskOpen(const U8* path);
EXPORT SClass* _makeThread(SClass* me_, const void* thread_func);
EXPORT void _threadDtor(SClass* me_);
EXPORT void _threadRun(SClass* me_);
EXPORT SClass* _makeMutex(SClass* me_);
EXPORT void _mutexDtor(SClass* me_);
EXPORT void _mutexLock(SClass* me_);
EXPORT Bool _mutexTryLock(SClass* me_);
EXPORT void _mutexUnlock(SClass* me_);
