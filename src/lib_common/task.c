#include "task.h"

typedef struct SProcess
{
	SClass Class;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
} SProcess;

EXPORT SClass* _makeProcess(SClass* me_, const U8* path)
{
	SProcess* me2 = (SProcess*)me_;
	{
		PROCESS_INFORMATION process_info;
		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(STARTUPINFO);
		if (!CreateProcess((const Char*)(path + 0x10), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS, NULL, NULL, &startup_info, &process_info))
			return NULL;
		me2->ProcessHandle = process_info.hProcess;
		me2->ThreadHandle = process_info.hThread;
	}
	return me_;
}

EXPORT void _processDtor(SClass* me_)
{
	SProcess* me2 = (SProcess*)me_;
	if (me2->ThreadHandle != NULL)
		CloseHandle(me2->ThreadHandle);
	if (me2->ProcessHandle != NULL)
		CloseHandle(me2->ProcessHandle);
}

EXPORT S64 _processRun(SClass* me_)
{
	SProcess* me2 = (SProcess*)me_;
	DWORD exit_code;
	if (ResumeThread(me2->ThreadHandle) == (DWORD)-1)
		THROW(0x1000, L"");
	if (WaitForSingleObject(me2->ProcessHandle, INFINITE) == WAIT_FAILED)
		THROW(0x1000, L"");
	if (!GetExitCodeProcess(me2->ProcessHandle, &exit_code))
		THROW(0x1000, L"");
	CloseHandle(me2->ThreadHandle);
	CloseHandle(me2->ProcessHandle);
	me2->ThreadHandle = NULL;
	me2->ProcessHandle = NULL;
	return (S64)(S32)exit_code;
}
