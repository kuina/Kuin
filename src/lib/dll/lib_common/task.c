#include "task.h"

typedef struct SProcess
{
	SClass Class;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	HANDLE ReadPipeHandle;
	HANDLE WritePipeHandle;
} SProcess;

EXPORT S64 _processRun(SClass* me_, Bool wait_until_exit)
{
	SProcess* me2 = (SProcess*)me_;
	if (me2->ProcessHandle == NULL)
		return 0;
	DWORD exit_code = 0;
	if (ResumeThread(me2->ThreadHandle) == (DWORD)-1)
		THROW(0xe9170009);
	if (wait_until_exit)
	{
		if (WaitForSingleObject(me2->ProcessHandle, INFINITE) != WAIT_OBJECT_0)
			THROW(0xe9170009);
		if (!GetExitCodeProcess(me2->ProcessHandle, &exit_code))
			THROW(0xe9170009);
	}
	return (S64)(S32)exit_code;
}

EXPORT void _processFin(SClass* me_)
{
	SProcess* me2 = (SProcess*)me_;
	if (me2->ProcessHandle != NULL)
		TerminateProcess(me2->ProcessHandle, 0);
	if (me2->WritePipeHandle != NULL)
	{
		CloseHandle(me2->WritePipeHandle);
		me2->WritePipeHandle = NULL;
	}
	if (me2->ReadPipeHandle != NULL)
	{
		CloseHandle(me2->ReadPipeHandle);
		me2->ReadPipeHandle = NULL;
	}
	if (me2->ThreadHandle != NULL)
	{
		CloseHandle(me2->ThreadHandle);
		me2->ThreadHandle = NULL;
	}
	if (me2->ProcessHandle != NULL)
	{
		CloseHandle(me2->ProcessHandle);
		me2->ProcessHandle = NULL;
	}
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
		THROW(0xe9170009);
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

EXPORT void _taskOpen(const U8* path, S64 mode, Bool wait_until_exit)
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
	SHELLEXECUTEINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	switch (mode)
	{
		case 0:
			info.lpVerb = L"open";
			break;
		case 1:
			info.lpVerb = L"explore";
			break;
		case 2:
			info.lpVerb = L"properties";
			break;
		default:
			return;
	}
	info.lpFile = path2;
	info.lpDirectory = cur_dir;
	info.nShow = SW_SHOWNORMAL;
	if (wait_until_exit)
		info.fMask |= SEE_MASK_NOCLOSEPROCESS;
	if (!ShellExecuteEx(&info))
		THROW(0xe9170009);
	if (wait_until_exit)
	{
		if (WaitForSingleObject(info.hProcess, INFINITE) != WAIT_OBJECT_0)
			THROW(0xe9170009);
	}
}
