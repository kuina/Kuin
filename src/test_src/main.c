// test.exe
//
// (C)Kuina-chan
//

#include "main.h"

#include <fcntl.h>
#include <io.h>
#include <conio.h>

// 0 = 'Ja', 1 = 'En'.
#define LANG (0)

#define TEST_NUM (24)

#pragma comment(lib, "compiler.lib")

Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, const void* related_files, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang, S64 app_code, Bool not_deploy);

static void Log(const Char* code, const Char* msg, const Char* src, int row, int col);
static Bool Compare(const Char* path1, const Char* path2);

int wmain(void)
{
	int type;
	_setmode(_fileno(stdout), _O_U8TEXT); // Set the output format to UTF-8.
	wprintf(L"-1 = Just build 'test.kn'.\n");
	wprintf(L" 0 = Run all the tests.\n");
	wprintf(L"> ");
	wscanf(L"%d", &type);
	if (type >= 0)
	{
		int i;
		Bool correct = True;
		for (i = type; i < TEST_NUM; i++)
		{
			PROCESS_INFORMATION process_info;
			HANDLE process;
			Char test_path[KUIN_MAX_PATH + 1];
			Char output_path[KUIN_MAX_PATH + 1];
			Char log_path[KUIN_MAX_PATH + 1];
			swprintf(test_path, KUIN_MAX_PATH + 1, L"../../test/kn/test%04d.kn", i);
			swprintf(output_path, KUIN_MAX_PATH + 1, L"../../test/output/output%04d.exe", i);
			swprintf(log_path, KUIN_MAX_PATH + 1, L"../../test/output/log%04d.txt", i);
			wprintf(L"%s\n", output_path);
			if (!BuildFile(test_path, L"../../package/sys/", output_path, L"../../package/sys/default.ico", NULL, False, L"cui", Log, 1, 0, False))
				goto Failure;
			wprintf(L"Compile[S]");
			{
				STARTUPINFO startup_info = { 0 };
				startup_info.cb = sizeof(STARTUPINFO);
				if (!CreateProcess(output_path, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &startup_info, &process_info))
					goto Failure;
				process = OpenProcess(PROCESS_VM_READ, FALSE, process_info.dwProcessId);
			}
			{
				FILE* file_ptr;
				DEBUG_EVENT debug_event = { 0 };
				Bool end = False;
				file_ptr = _wfopen(log_path, L"w, ccs=UTF-8");
				if (file_ptr == NULL)
					goto Failure;
				rewind(file_ptr);
				fwprintf(file_ptr, L"Begin: %s\n", output_path);
				ResumeThread(process_info.hThread);
				while (!end)
				{
					WaitForDebugEvent(&debug_event, INFINITE);
					switch (debug_event.dwDebugEventCode)
					{
						case CREATE_PROCESS_DEBUG_EVENT:
							if (debug_event.u.CreateProcessInfo.hFile != 0)
								CloseHandle(debug_event.u.CreateProcessInfo.hFile);
							break;
						case LOAD_DLL_DEBUG_EVENT:
							if (debug_event.u.LoadDll.hFile != 0)
								CloseHandle(debug_event.u.LoadDll.hFile);
							break;
						case EXIT_PROCESS_DEBUG_EVENT:
							end = True;
							break;
						case EXCEPTION_DEBUG_EVENT:
							fwprintf(file_ptr, L"Excpt: %08x\n", debug_event.u.Exception.ExceptionRecord.ExceptionCode);
							break;
						case OUTPUT_DEBUG_STRING_EVENT:
							{
								U8 buf[1024] = { 0 };
								SIZE_T size;
								if (!ReadProcessMemory(process, debug_event.u.DebugString.lpDebugStringData, buf, debug_event.u.DebugString.nDebugStringLength, &size))
									goto Failure;
								fwprintf(file_ptr, L"> ");
								{
									const U8* ptr = buf;
									while (*ptr != 0)
									{
										fwprintf(file_ptr, L"%c", *ptr);
										ptr++;
									}
								}
								fwprintf(file_ptr, L"\n");
							}
							break;
					}
					ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, DBG_CONTINUE);
				}
				WaitForSingleObject(process_info.hProcess, INFINITE);
				CloseHandle(process_info.hThread);
				CloseHandle(process_info.hProcess);
				fwprintf(file_ptr, L"end: %s\n", output_path);
				fclose(file_ptr);
			}
			wprintf(L", Run[S]");
			{
				Char path[KUIN_MAX_PATH + 1];
				swprintf(path, KUIN_MAX_PATH + 1, L"../../test/correct/log%04d.txt", i);
				if (!Compare(log_path, path))
				{
					correct = False;
					wprintf(L", Compare[F]");
				}
				else
					wprintf(L", Compare[S]");
			}
			wprintf(L"\n");
		}
		if (!correct)
			goto Failure;
	}
	else if (type == -1)
	{
		if (!BuildFile(L"../../test/kn/test.kn", L"../../package/sys/", L"../../test/output/output.exe", L"../../package/sys/default.ico", NULL, False, L"cui", Log, LANG, 0, False))
			goto Failure;
	}
	else if (type == -2)
	{
		if (!BuildFile(L"../../test/kn/test.kn", L"../../package/sys/", L"../../test/output/output.exe", L"../../package/sys/default.ico", NULL, False, L"wnd", Log, LANG, 0, False))
			goto Failure;
	}
	else
		goto Failure;
	wprintf(L"\nSuccess.\n");
	_getch();
	return 0;
Failure:
	wprintf(L"\nFailure.\n");
	_getch();
	return 0;
}

static void Log(const Char* code, const Char* msg, const Char* src, int row, int col)
{
	if (code[0] == L'I')
		return;
	switch (code[0])
	{
		case L'E': wprintf(L"[Error] "); break;
		case L'W': wprintf(L"[Warning] "); break;
		case L'I': wprintf(L"[Info] "); break;
		default:
			ASSERT(False);
			break;
	}
	if (src == NULL)
		wprintf(L"%s: %s\n", code, msg);
	else
		wprintf(L"%s: %s (%s: %d, %d)\n", code, msg, src, row, col);
}

static Bool Compare(const Char* path1, const Char* path2)
{
	FILE* file_ptr1;
	FILE* file_ptr2;
	file_ptr1 = _wfopen(path1, L"rb");
	if (file_ptr1 == NULL)
		return False;
	file_ptr2 = _wfopen(path2, L"rb");
	if (file_ptr2 == NULL)
	{
		fclose(file_ptr1);
		return False;
	}
	for (; ; )
	{
		int c1;
		int c2;
		do
		{
			c1 = fgetc(file_ptr1);
		} while (c1 == '\r');
		do
		{
			c2 = fgetc(file_ptr2);
		} while (c2 == '\r');
		if (c1 != c2)
			goto Failure;
		if (c1 == EOF)
			break;
	}
	fclose(file_ptr2);
	fclose(file_ptr1);
	return True;
Failure:
	fclose(file_ptr2);
	fclose(file_ptr1);
	return False;
}
