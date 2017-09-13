#include "task.h"

#include "main.h"

typedef struct SProcess
{
	SClass Class;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
} SProcess;

typedef struct SThread
{
	SClass Class;
	HANDLE ThreadHandle;
	Bool Begin;
} SThread;

typedef struct SMutex
{
	SClass Class;
	CRITICAL_SECTION* Handle;
} SMutex;

static DWORD WINAPI ThreadFunc(LPVOID arg);

EXPORT SClass* _makeProcess(SClass* me_, const U8* path, const U8* cmd_line)
{
	THROWDBG(path == NULL, 0xc0000005);
	SProcess* me2 = (SProcess*)me_;
	const Char* path2 = (const Char*)(path + 0x10);
	Char cur_dir[KUIN_MAX_PATH + 1];
	Char* cmd_line_buf = NULL;
	{
		Char* ptr;
		wcscpy(cur_dir, path2);
		ptr = cur_dir + wcslen(cur_dir);
		while (ptr != cur_dir && *ptr != L'/' && *ptr != L'\\')
			ptr--;
		if (ptr != NULL)
			*(ptr + 1) = L'\0';
	}
	if (cmd_line != NULL)
	{
		size_t len = wcslen((const Char*)(cmd_line + 0x10));
		cmd_line_buf = (Char*)AllocMem(sizeof(Char) * (len + 1));
		wcscpy(cmd_line_buf, (const Char*)(cmd_line + 0x10));
	}
	{
		PROCESS_INFORMATION process_info;
		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(STARTUPINFO);
		if (!CreateProcess(path2, cmd_line_buf, NULL, NULL, FALSE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS, NULL, cur_dir, &startup_info, &process_info))
		{
			if (cmd_line_buf != NULL)
				FreeMem(cmd_line_buf);
			THROW(0xe9170009);
			return NULL;
		}
		me2->ProcessHandle = process_info.hProcess;
		me2->ThreadHandle = process_info.hThread;
	}
	if (cmd_line_buf != NULL)
		FreeMem(cmd_line_buf);
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

EXPORT S64 _processRun(SClass* me_, Bool waitUntilExit)
{
	SProcess* me2 = (SProcess*)me_;
	DWORD exit_code = 0;
	if (ResumeThread(me2->ThreadHandle) == (DWORD)-1)
		THROW(0xe9170009);
	if (waitUntilExit)
	{
		if (WaitForSingleObject(me2->ProcessHandle, INFINITE) == WAIT_FAILED)
			THROW(0xe9170009);
		if (!GetExitCodeProcess(me2->ProcessHandle, &exit_code))
			THROW(0xe9170009);
	}
	CloseHandle(me2->ThreadHandle);
	CloseHandle(me2->ProcessHandle);
	me2->ThreadHandle = NULL;
	me2->ProcessHandle = NULL;
	return (S64)(S32)exit_code;
}

EXPORT void _taskOpen(const U8* path)
{
	const Char* path2 = (const Char*)(path + 0x10);
	Char cur_dir[KUIN_MAX_PATH + 1];
	{
		Char* ptr;
		wcscpy(cur_dir, path2);
		ptr = wcsrchr(cur_dir, L'\\');
		if (ptr != NULL)
			*(ptr + 1) = L'\0';
	}
	if ((U64)ShellExecute(NULL, L"open", path2, NULL, cur_dir, SW_SHOWNORMAL) <= 32)
		THROW(0xe9170009);
}

EXPORT SClass* _makeThread(SClass* me_, const void* thread_func)
{
	THROWDBG(thread_func == NULL, 0xc0000005);
	SThread* me2 = (SThread*)me_;
	me2->Begin = False;
	me2->ThreadHandle = CreateThread(NULL, 0, ThreadFunc, (LPVOID)thread_func, CREATE_SUSPENDED, NULL);
	if (me2->ThreadHandle == NULL)
	{
		THROW(0xe9170009);
		return NULL;
	}
	return me_;
}

EXPORT void _threadDtor(SClass* me_)
{
	SThread* me2 = (SThread*)me_;
	if (me2->ThreadHandle != NULL)
		CloseHandle(me2->ThreadHandle);
}

EXPORT void _threadRun(SClass* me_)
{
	SThread* me2 = (SThread*)me_;
	me2->Begin = True;
	if (ResumeThread(me2->ThreadHandle) == (DWORD)-1)
		THROW(0xe9170009);
}

EXPORT Bool _threadRunning(SClass* me_)
{
	SThread* me2 = (SThread*)me_;
	if (!me2->Begin)
		return False;
	DWORD exit_code;
	GetExitCodeThread(me2->ThreadHandle, &exit_code);
	return exit_code == STILL_ACTIVE;
}

EXPORT SClass* _makeMutex(SClass* me_)
{
	SMutex* me2 = (SMutex*)me_;
	me2->Handle = (CRITICAL_SECTION*)AllocMem(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(me2->Handle);
	return me_;
}

EXPORT void _mutexDtor(SClass* me_)
{
	SMutex* me2 = (SMutex*)me_;
	DeleteCriticalSection(me2->Handle);
	FreeMem(me2->Handle);
}

EXPORT void _mutexLock(SClass* me_)
{
	SMutex* me2 = (SMutex*)me_;
	EnterCriticalSection(me2->Handle);
}

EXPORT Bool _mutexTryLock(SClass* me_)
{
	SMutex* me2 = (SMutex*)me_;
	return TryEnterCriticalSection(me2->Handle) ? True : False;
}

EXPORT void _mutexUnlock(SClass* me_)
{
	SMutex* me2 = (SMutex*)me_;
	LeaveCriticalSection(me2->Handle);
}

static DWORD WINAPI ThreadFunc(LPVOID arg)
{
	Call0Asm(arg);
	return 0;
}
