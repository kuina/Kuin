#include "task.h"

typedef struct SProcess
{
	SClass Class;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
} SProcess;

EXPORT SClass* _makeProcess(SClass* me_, const U8* path, const U8* cmd_line)
{
	SProcess* me2 = (SProcess*)me_;
	const Char* path2 = (const Char*)(path + 0x10);
	Char cur_dir[MAX_PATH + 1];
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
			return NULL;
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
		THROW(0x1000, L"");
	if (waitUntilExit)
	{
		if (WaitForSingleObject(me2->ProcessHandle, INFINITE) == WAIT_FAILED)
			THROW(0x1000, L"");
		if (!GetExitCodeProcess(me2->ProcessHandle, &exit_code))
			THROW(0x1000, L"");
	}
	CloseHandle(me2->ThreadHandle);
	CloseHandle(me2->ProcessHandle);
	me2->ThreadHandle = NULL;
	me2->ProcessHandle = NULL;
	return (S64)(S32)exit_code;
}

EXPORT void _processRunAsync(SClass* me_, void* func)
{
	// TODO:
}

EXPORT void _taskOpen(const U8* path)
{
	const Char* path2 = (const Char*)(path + 0x10);
	Char cur_dir[MAX_PATH + 1];
	{
		Char* ptr;
		wcscpy(cur_dir, path2);
		ptr = wcsrchr(cur_dir, L'\\');
		if (ptr != NULL)
			*(ptr + 1) = L'\0';
	}
	ShellExecute(NULL, L"open", path2, NULL, cur_dir, SW_SHOWNORMAL);
}
