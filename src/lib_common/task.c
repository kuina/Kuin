#include "task.h"

#include "main.h"

#include <fcntl.h>
#include <io.h>

typedef struct SProcess
{
	SClass Class;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	HANDLE ReadPipeHandle;
	HANDLE WritePipeHandle;
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
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
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
		size_t path_len = wcslen(path2);
		size_t len = wcslen((const Char*)(cmd_line + 0x10));
		cmd_line_buf = (Char*)AllocMem(sizeof(Char) * (path_len + 3 + len + 1));
		wcscpy(cmd_line_buf, L"\"");
		wcscat(cmd_line_buf, path2);
		wcscat(cmd_line_buf, L"\" ");
		wcscat(cmd_line_buf, (const Char*)(cmd_line + 0x10));
	}
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&me2->ReadPipeHandle, &me2->WritePipeHandle, &sa, 0))
	{
		THROW(EXCPT_DEVICE_INIT_FAILED);
		return NULL;
	}
	{
		PROCESS_INFORMATION process_info;
		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(STARTUPINFO);
		startup_info.dwFlags = STARTF_USESTDHANDLES;
		startup_info.hStdOutput = me2->WritePipeHandle;
		startup_info.hStdError = me2->WritePipeHandle;
		if (!CreateProcess(path2, cmd_line_buf, NULL, NULL, TRUE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, cur_dir, &startup_info, &process_info))
		{
			if (cmd_line_buf != NULL)
				FreeMem(cmd_line_buf);
			THROW(EXCPT_DEVICE_INIT_FAILED);
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
	if (me2->WritePipeHandle != NULL)
		CloseHandle(me2->WritePipeHandle);
	if (me2->ReadPipeHandle != NULL)
		CloseHandle(me2->ReadPipeHandle);
	if (me2->ThreadHandle != NULL)
		CloseHandle(me2->ThreadHandle);
	if (me2->ProcessHandle != NULL)
		CloseHandle(me2->ProcessHandle);
}

EXPORT S64 _processRun(SClass* me_, Bool wait_until_exit)
{
	SProcess* me2 = (SProcess*)me_;
	if (me2->ProcessHandle == NULL)
		return 0;
	DWORD exit_code = 0;
	if (ResumeThread(me2->ThreadHandle) == (DWORD)-1)
		THROW(EXCPT_DEVICE_INIT_FAILED);
	if (wait_until_exit)
	{
		if (WaitForSingleObject(me2->ProcessHandle, INFINITE) != WAIT_OBJECT_0)
			THROW(EXCPT_DEVICE_INIT_FAILED);
		if (!GetExitCodeProcess(me2->ProcessHandle, &exit_code))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}
	return (S64)(S32)exit_code;
}

EXPORT void _processFin(SClass* me_)
{
	SProcess* me2 = (SProcess*)me_;
	if (me2->ProcessHandle == NULL)
		return;
	TerminateProcess(me2->ProcessHandle, 0);
	CloseHandle(me2->WritePipeHandle);
	CloseHandle(me2->ReadPipeHandle);
	CloseHandle(me2->ThreadHandle);
	CloseHandle(me2->ProcessHandle);
	me2->WritePipeHandle = NULL;
	me2->ReadPipeHandle = NULL;
	me2->ThreadHandle = NULL;
	me2->ProcessHandle = NULL;
}

EXPORT Bool _processRunning(SClass* me_, S64* exit_code)
{
	*exit_code = 0;
	SProcess* me2 = (SProcess*)me_;
	if (me2->ProcessHandle == NULL)
		return False;
	DWORD dword_exit_code;
	if (!GetExitCodeProcess(me2->ProcessHandle, &dword_exit_code))
		return False;
	if (dword_exit_code == STILL_ACTIVE)
		return True;
	*exit_code = (S64)dword_exit_code;
	return False;
}

EXPORT void* _processReadPipe(SClass* me_)
{
	SProcess* me2 = (SProcess*)me_;
	if (me2->ProcessHandle == NULL)
		return NULL;
	DWORD size;
	if (!PeekNamedPipe(me2->ReadPipeHandle, NULL, 0, NULL, &size, NULL) || size == 0)
		return NULL;
	U8* buf_tmp = (U8*)AllocMem((size_t)size);
	DWORD read_size;
	if (!ReadFile(me2->ReadPipeHandle, buf_tmp, size, &read_size, NULL))
	{
		FreeMem(buf_tmp);
		return NULL;
	}

	size_t len = (size_t)MultiByteToWideChar(CP_ACP, 0, (char*)buf_tmp, read_size, NULL, 0);
	Char* result = (Char*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)len;
	if (MultiByteToWideChar(CP_ACP, 0, (char*)buf_tmp, read_size, (Char*)((U8*)result + 0x10), (int)len) != (int)len)
	{
		FreeMem(buf_tmp);
		FreeMem(result);
		return NULL;
	}
	((Char*)((U8*)result + 0x10))[len] = L'\0';
	FreeMem(buf_tmp);
	return result;
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
		THROW(EXCPT_DEVICE_INIT_FAILED);
}

EXPORT SClass* _makeThread(SClass* me_, const void* thread_func)
{
	THROWDBG(thread_func == NULL, EXCPT_ACCESS_VIOLATION);
	SThread* me2 = (SThread*)me_;
	me2->Begin = False;
	me2->ThreadHandle = CreateThread(NULL, 0, ThreadFunc, (LPVOID)thread_func, CREATE_SUSPENDED, NULL);
	if (me2->ThreadHandle == NULL)
	{
		THROW(EXCPT_DEVICE_INIT_FAILED);
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
		THROW(EXCPT_DEVICE_INIT_FAILED);
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
