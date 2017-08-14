//
// KuinCompiler.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "analyze.h"
#include "assemble.h"
#include "ast.h"
#include "deploy.h"
#include "dict.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "option.h"
#include "parse.h"
#include "util.h"

#include <DbgHelp.h> // 'StackWalk64'
#pragma comment(lib, "DbgHelp.lib")

typedef struct SIdentifier
{
	Char* Name;
	Char* Hint;
	int Row;
	int Col;
	int ScopeRowBegin;
	int ScopeRowEnd;
} SIdentifier;

typedef struct SIdentifierSet
{
	Char* Src;
	int IdentifierNum;
	SIdentifier* Identifiers;
} SIdentifierSet;

typedef struct SDbgInfoSet
{
	U64 Begin;
	U64 End;
	SAstFunc* Func;
} SDbgInfoSet;

static const void*(*FuncGetSrc)(const U8*) = NULL;
static void(*FuncLog)(const void*, S64, S64) = NULL;
static const void* Src = NULL;
static const void* SrcLine = NULL;
static const Char* SrcChar = NULL;
static int IdentifierSetNum = 0;
static SIdentifierSet* IdentifierSets = NULL;
static int DbgInfoSetNum = 0;
static SDbgInfoSet* DbgInfoSets = NULL;
static SPackAsm PackAsm;
static U64 DbgStartAddr;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static void DecSrc(void);
static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), Bool analyze_identifiers);
static FILE* BuildMemWfopen(const Char* file_name, const Char* mode);
static int BuildMemFclose(FILE* file_ptr);
static U16 BuildMemFgetwc(FILE* file_ptr);
static size_t BuildMemGetSize(FILE* file_ptr);
static void BuildMemLog(const Char* code, const Char* msg, const Char* src, int row, int col);
static size_t BuildFileGetSize(FILE* file_ptr);
static void MakeIdentifierSet(SDict* asts);
static const void* MakeIdentifierSet2(const Char* key, const void* value, void* param);
static const void* MakeIdentifierSet3(const Char* key, const void* value, void* param);
static const void* MakeIdentifierSet4(const Char* key, const void* value, void* param);
static const void* MakeIdentifierSet5(const Char* key, const void* value, void* param);
static int CmpIdentifierSet(const void* a, const void* b);
static int CmpIdentifier(const void* a, const void* b);
static SPos* AddrToPos(U64 addr, Char* name);
static const void* AddrToPosCallback(U64 key, const void* value, void* param);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col))
{
	Bool result;
	FuncGetSrc = func_get_src;
	FuncLog = func_log;
	result = Build(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, (const Char*)(path + 0x10), sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10), output == NULL ? NULL : (const Char*)(output + 0x10), icon == NULL ? NULL : (const Char*)(icon + 0x10), rls, env == NULL ? NULL : (const Char*)(env + 0x10), BuildMemLog, True);
	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return result;
}

EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col))
{
	Bool result;
	InitAllocator();
	result = Build(_wfopen, fclose, fgetwc, BuildFileGetSize, path, sys_dir, output, icon, rls, env, func_log, False);
	FinAllocator();
	return result;
}

EXPORT void Interpret1(const void* src, const void* color)
{
	InterpretImpl1(src, color);
}

EXPORT void Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col))
{
	const Char* sys_dir2 = sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10);

	FuncGetSrc = func_get_src;
	FuncLog = func_log;

	// Set the system directory.
	if (sys_dir2 == NULL)
	{
		Char sys_dir3[1024 + 1];
		GetModuleFileName(NULL, sys_dir3, 1024 + 1);
		sys_dir2 = GetDir(sys_dir3, False, L"sys/");
	}
	else
		sys_dir2 = GetDir(sys_dir2, True, NULL);

	SetLogFunc(BuildMemLog, 0, sys_dir2);
	ResetErrOccurred();

	{
		SOption option;
		SDict* asts;
		SDict* dlls;
		MakeOption(&option, (const Char*)(path + 0x10), NULL, sys_dir2, NULL, False, env == NULL ? NULL : (const Char*)(env + 0x10));
		if (!ErrOccurred())
		{
			asts = Parse(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, &option);
			if (!ErrOccurred())
			{
				Analyze(asts, &option, &dlls);
				if (!ErrOccurred())
					MakeIdentifierSet(asts);
			}
		}
	}

	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
}

EXPORT void Version(S64* major, S64* minor, S64* micro)
{
	*major = 9;
	*minor = 17;
	*micro = 0;
}

EXPORT void InitMemAllocator(void)
{
	InitAllocator();
}

EXPORT void FinMemAllocator(void)
{
	FinAllocator();
}

EXPORT void ResetMemAllocator(void)
{
	ResetAllocator();
}

EXPORT void FreeIdentifierSet(void)
{
	if (IdentifierSets == NULL)
		return;
	{
		int i, j;
		for (i = 0; i < IdentifierSetNum; i++)
		{
			for (j = 0; j < IdentifierSets[i].IdentifierNum; j++)
			{
				free(IdentifierSets[i].Identifiers[j].Name);
				free(IdentifierSets[i].Identifiers[j].Hint);
			}
			free(IdentifierSets[i].Identifiers);
			free(IdentifierSets[i].Src);
		}
	}
	free(IdentifierSets);
	IdentifierSets = NULL;
}

EXPORT void DumpIdentifierSet(const Char* path)
{
	if (IdentifierSets == NULL)
		return;
	{
		FILE* file_ptr = _wfopen(path, L"w, ccs=UTF-8");
		int i, j;
		for (i = 0; i < IdentifierSetNum; i++)
		{
			fwprintf(file_ptr, L"%s:\n", IdentifierSets[i].Src);
			for (j = 0; j < IdentifierSets[i].IdentifierNum; j++)
			{
				SIdentifier* identifier = &IdentifierSets[i].Identifiers[j];
				fwprintf(file_ptr, L"\t%s (%d, %d) [%d, %d]\n", identifier->Name, identifier->Row, identifier->Col, identifier->ScopeRowBegin, identifier->ScopeRowEnd);
				fwprintf(file_ptr, L"\t\t%s\n", identifier->Hint);
			}
		}
		fclose(file_ptr);
	}
}

EXPORT void* GetHint(const U8* name, const U8* src, S64 row)
{
	const Char* name2 = (const Char*)(name + 0x10);
	SIdentifierSet* identifier_set = NULL;
	int i;
	int ptr = -1;
	for (i = 0; i < IdentifierSetNum; i++)
	{
		if (wcscmp(IdentifierSets[i].Src, (const Char*)(src + 0x10)) != 0)
			continue;
		identifier_set = &IdentifierSets[i];
	}
	if (identifier_set == NULL)
		return NULL;
	for (i = 0; i < identifier_set->IdentifierNum; i++)
	{
		if (wcscmp(identifier_set->Identifiers[i].Name, name2) != 0)
			continue;
		ptr = i;
	}
	if (ptr == -1)
		return NULL;

	{
		int max = INT_MIN;
		int best = -1;
		if (row == -1)
			best = ptr;
		while (ptr < identifier_set->IdentifierNum && wcscmp(identifier_set->Identifiers[ptr].Name, name2) == 0)
		{
			int begin = identifier_set->Identifiers[ptr].ScopeRowBegin;
			int end = identifier_set->Identifiers[ptr].ScopeRowEnd;
			if (max < begin && begin <= row && row <= end)
			{
				max = begin;
				best = ptr;
			}
			ptr++;
		}
		if (best == -1)
			return NULL;
		{
			size_t len = wcslen(identifier_set->Identifiers[best].Hint);
			U8* result = (U8*)Alloc(0x10 + sizeof(Char) * (len + 1));
			*(S64*)(result + 0x00) = DefaultRefCntFunc + 1;
			*(S64*)(result + 0x08) = (S64)len;
			wcscpy((Char*)(result + 0x10), identifier_set->Identifiers[best].Hint);
			return result;
		}
	}
}

EXPORT Bool RunDbg(const U8* path, const U8* cmd_line, void* idle_func)
{
	const Char* path2 = (const Char*)(path + 0x10);
	Char cur_dir[MAX_PATH + 1];
	Char* cmd_line_buf = NULL;
	PROCESS_INFORMATION process_info;
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
		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(STARTUPINFO);
		if (!CreateProcess(path2, cmd_line_buf, NULL, NULL, FALSE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS | DEBUG_ONLY_THIS_PROCESS, NULL, cur_dir, &startup_info, &process_info))
		{
			if (cmd_line_buf != NULL)
				FreeMem(cmd_line_buf);
			return False;
		}
	}
	if (cmd_line_buf != NULL)
		FreeMem(cmd_line_buf);

	{
		DEBUG_EVENT debug_event;
		Bool end = False;
		DbgStartAddr = 0;
		ResumeThread(process_info.hThread);
		while (!end)
		{
			DWORD continue_status = DBG_EXCEPTION_NOT_HANDLED;
			Call0Asm(idle_func);
			WaitForDebugEvent(&debug_event, 0);
			switch (debug_event.dwDebugEventCode)
			{
				case CREATE_PROCESS_DEBUG_EVENT:
					DbgStartAddr = (U64)debug_event.u.CreateProcessInfo.lpBaseOfImage;
					CloseHandle(debug_event.u.CreateProcessInfo.hFile);
					break;
				case LOAD_DLL_DEBUG_EVENT:
					CloseHandle(debug_event.u.LoadDll.hFile);
					break;
				case EXIT_PROCESS_DEBUG_EVENT:
					end = True;
					break;
				case EXCEPTION_DEBUG_EVENT:
					if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == 0x80000003)
						break;
					{
						Char str[4096] = L"";
						CONTEXT context;
						STACKFRAME64 stack;
						IMAGEHLP_SYMBOL64 symbol;
						memset(&context, 0, sizeof(context));
						context.ContextFlags = CONTEXT_FULL;
						if (!GetThreadContext(process_info.hThread, &context))
							break;
						memset(&stack, 0, sizeof(stack));
						stack.AddrPC.Offset = context.Rip;
						stack.AddrPC.Mode = AddrModeFlat;
						stack.AddrStack.Offset = context.Rsp;
						stack.AddrStack.Mode = AddrModeFlat;
						stack.AddrFrame.Offset = context.Rbp;
						stack.AddrFrame.Mode = AddrModeFlat;
						SymInitialize(process_info.hProcess, NULL, TRUE);
						{
							DWORD code = debug_event.u.Exception.ExceptionRecord.ExceptionCode;
							PVOID addr = debug_event.u.Exception.ExceptionRecord.ExceptionAddress;
							const Char* text = L"Unknown exception.";
							DWORD param_num = debug_event.u.Exception.ExceptionRecord.NumberParameters;
							const ULONG_PTR* params = debug_event.u.Exception.ExceptionRecord.ExceptionInformation;
							if (code <= 0x0000ffff)
								text = L"User defined exception.";
							else if (0x09170000 <= code && code <= 0x0917ffff)
								text = L"Kuin library exception.";
							else
							{
								switch (code)
								{
									case 0xc0000005: text = L"Access violation."; break;
									case 0xc0000017: text = L"No memory."; break;
									case 0xc0000090: text = L"Float invalid operation."; break;
									case 0xc0000094: text = L"Integer division by zero."; break;
									case 0xc00000fd: text = L"Stack overflow."; break;
									case 0xc000013a: text = L"Ctrl-C exit."; break;
									case 0xc9170000: text = L"Assertion failed."; break;
									case 0xc9170001: text = L"Class cast failed."; break;
									case 0xc9170002: text = L"Array index out of range."; break;
									case 0xc9170003: text = L"Integer overflow."; break;
									case 0xc9170004: text = L"Invalid call of non inherited 'cmp' method."; break;
									case 0xc9170005: text = L"Invalid operation on standard library class."; break;
								}
							}
							swprintf(str, 1024, L"An exception of '0x%08X' occurred at '0x%016I64X'.\r\n> %s\r\n\r\n", debug_event.u.Exception.ExceptionRecord.ExceptionCode, (U64)debug_event.u.Exception.ExceptionRecord.ExceptionAddress, text);
						}

						for (; ; )
						{
							if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process_info.hProcess, process_info.hThread, &stack, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
								break;

							{
								Char str2[1024];
								swprintf(str2, 1024, L"0x%016I64X: \t", context.Rip);
								wcscat(str, str2);
							}

							symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
							symbol.MaxNameLength = 255;
							DWORD64 displacement;
							if (SymGetSymFromAddr64(process_info.hProcess, (DWORD64)stack.AddrPC.Offset, &displacement, &symbol))
							{
								char name[1024];
								UnDecorateSymbolName(symbol.Name, (PSTR)name, 1024, UNDNAME_COMPLETE);
								{
									Char* dst = str;
									char* src = name;
									while (*dst != L'\0')
										dst++;
									while (*src != L'\0')
									{
										*dst = (Char)*src;
										dst++;
										src++;
									}
									*dst = L'\r';
									dst++;
									*dst = L'\n';
									dst++;
									*dst = L'\0';
								}
							}
							else
							{
								Char name[129];
								SPos* pos = AddrToPos((U64)context.Rip, name);
								if (pos != NULL)
								{
									Char str2[1024];
									swprintf(str2, 1024, L"%s (%s: %d, %d)\r\n", name, pos->SrcName, pos->Row, pos->Col);
									wcscat(str, str2);
								}
								else
									wcscat(str, L"\r\n");
							}
							if (stack.AddrPC.Offset == 0)
								break;
						}
						MessageBox(NULL, str, NULL, 0);
					}
					continue_status = DBG_CONTINUE;
					break;
				case OUTPUT_DEBUG_STRING_EVENT:
					// TODO: Debug printing.
					break;
			}
			ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, continue_status);
		}
	}
	{
		DWORD exit_code;
		while (GetExitCodeThread(process_info.hThread, &exit_code) == STILL_ACTIVE)
			Call0Asm(idle_func);
	}

	if (process_info.hThread != NULL)
		CloseHandle(process_info.hThread);
	if (process_info.hProcess != NULL)
		CloseHandle(process_info.hProcess);
	return True;
}

static void DecSrc(void)
{
	// Decrement 'Src', but do not release it here. It will be released in '.kn'.
	if (Src != NULL)
	{
		(*(S64*)Src)--;
		ASSERT(*(S64*)Src > 0);
	}
}

static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), Bool analyze_identifiers)
{
	SOption option;
	SDict* asts;
	SAstFunc* entry;
	SDict* dlls;
	U32 begin_time;

	// Set the system directory.
	if (sys_dir == NULL)
	{
		Char sys_dir2[1024 + 1];
		GetModuleFileName(NULL, sys_dir2, 1024 + 1);
		sys_dir = GetDir(sys_dir2, False, L"sys/");
	}
	else
		sys_dir = GetDir(sys_dir, True, NULL);

	SetLogFunc(func_log, 0, sys_dir);
	ResetErrOccurred();

	timeBeginPeriod(1);

	begin_time = timeGetTime();
	MakeOption(&option, path, output, sys_dir, icon, rls, env);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0000", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	asts = Parse(func_wfopen, func_fclose, func_fgetwc, func_size, &option);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0001", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	entry = Analyze(asts, &option, &dlls);
	if (ErrOccurred())
		goto ERR;
#if defined(_DEBUG)
	UNUSED(analyze_identifiers);
	MakeIdentifierSet(asts);
#else
	if (analyze_identifiers)
		MakeIdentifierSet(asts);
#endif
	Err(L"IK0002", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	Assemble(&PackAsm, entry, &option, dlls);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0003", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	ToMachineCode(&PackAsm, &option);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0004", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	Deploy(PackAsm.AppCode, &option, dlls);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0005", NULL, (double)(timeGetTime() - begin_time) / 1000.0);

	timeEndPeriod(1);
	Err(L"IK0006", NULL);
#if defined (_DEBUG)
	DumpIdentifierSet(NewStr(NULL, L"%s_identifiers.txt", option.OutputFile));
#endif
	return True;

ERR:
	timeEndPeriod(1);
	Err(L"IK0007", NULL);
	return False;
}

static FILE* BuildMemWfopen(const Char* file_name, const Char* mode)
{
	UNUSED(mode);
	{
		U8 file_name2[0x10 + sizeof(Char) * (MAX_PATH + 1)];
		*(S64*)(file_name2 + 0x00) = 2;
		*(S64*)(file_name2 + 0x08) = (S64)wcslen(file_name);
		wcscpy((Char*)(file_name2 + 0x10), file_name);
		DecSrc();
		Src = Call1Asm(file_name2, (void*)(U64)FuncGetSrc);
		if (Src == NULL)
			return NULL;
		SrcLine = (U8*)Src + 0x10;
		SrcChar = (Char*)((U8*)*(void**)SrcLine + 0x10);
		return (FILE*)DummyPtr;
	}
}

static int BuildMemFclose(FILE* file_ptr)
{
	UNUSED(file_ptr);
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return 0;
}

static U16 BuildMemFgetwc(FILE* file_ptr)
{
	const void* term;
	{
		S64 len = *(S64*)((U8*)Src + 0x08);
		term = (U8*)Src + 0x10 + len * 0x08;
	}
	UNUSED(file_ptr);
	if (SrcLine == term)
		return L'\0';
	{
		Char c = *SrcChar;
		if (c != L'\0')
		{
			SrcChar++;
			return c;
		}
		SrcLine = (U8*)SrcLine + 0x08;
		if (SrcLine == term)
			return L'\0';
		SrcChar = (Char*)((U8*)*(void**)SrcLine + 0x10);
		return L'\n';
	}
}

static size_t BuildMemGetSize(FILE* file_ptr)
{
	UNUSED(file_ptr);
	{
		size_t total = 0;
		S64 len = *(S64*)((U8*)Src + 0x08);
		void* ptr = (U8*)Src + 0x10;
		S64 i;
		for (i = 0; i < len; i++)
		{
			total += *(S64*)((U8*)*(void**)ptr + 0x08);
			if (total >= 2)
				return 2; // A value of 2 or more is not distinguished.
			ptr = (U8*)ptr + 0x08;
		}
		return total;
	}
}

static void BuildMemLog(const Char* code, const Char* msg, const Char* src, int row, int col)
{
	U8 code2[0x10 + sizeof(Char) * (MAX_PATH + 1)];
	size_t len_msg = wcslen(msg);
	U8* msg2 = (U8*)Alloc(0x10 + sizeof(Char) * (len_msg + 1));
	U8 src2[0x10 + sizeof(Char) * (MAX_PATH + 1)];
	U8 args[0x10 + 0x18];
	{
		*(S64*)(code2 + 0x00) = 1;
		*(S64*)(code2 + 0x08) = (S64)wcslen(code);
		wcscpy((Char*)(code2 + 0x10), code);
	}
	{
		*(S64*)(msg2 + 0x00) = 1;
		*(S64*)(msg2 + 0x08) = (S64)len_msg;
		wcscpy((Char*)(msg2 + 0x10), msg);
	}
	if (src != NULL)
	{
		*(S64*)(src2 + 0x00) = 1;
		*(S64*)(src2 + 0x08) = (S64)wcslen(src);
		wcscpy((Char*)(src2 + 0x10), src);
	}
	{
		*(S64*)(args + 0x00) = 2;
		*(S64*)(args + 0x08) = 3;
		*(void**)(args + 0x10) = code2;
		*(void**)(args + 0x18) = msg2;
		*(void**)(args + 0x20) = src == NULL ? NULL : src2;
	}
	Call3Asm(args, (void*)(S64)row, (void*)(S64)col, (void*)(U64)FuncLog);
}

static size_t BuildFileGetSize(FILE* file_ptr)
{
	int file_size;
	fseek(file_ptr, 0, SEEK_END);
	file_size = (int)ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);
	return (size_t)file_size;
}

static void MakeIdentifierSet(SDict* asts)
{
	int len = 0;
	FreeIdentifierSet();
	DictForEach(asts, MakeIdentifierSet2, &len);
	IdentifierSetNum = len;
	IdentifierSets = (SIdentifierSet*)malloc(sizeof(SIdentifierSet) * (size_t)len);
	{
		int cnt = 0;
		DictForEach(asts, MakeIdentifierSet3, &cnt);
		qsort(IdentifierSets, len, sizeof(SIdentifierSet), CmpIdentifierSet);
	}
}

static const void* MakeIdentifierSet2(const Char* key, const void* value, void* param)
{
	int* len = (int*)param;
	UNUSED(key);
	(*len)++;
	return value;
}

static const void* MakeIdentifierSet3(const Char* key, const void* value, void* param)
{
	int* cnt = (int*)param;
	size_t name_len = wcslen(key);
	SAst* ast = (SAst*)value;
	int len = 0;
	IdentifierSets[*cnt].Src = (Char*)malloc(sizeof(Char) * (name_len + 1));
	wcscpy(IdentifierSets[*cnt].Src, key);
	DictForEach(ast->ScopeChildren, MakeIdentifierSet4, &len);
	IdentifierSets[*cnt].IdentifierNum = len;
	IdentifierSets[*cnt].Identifiers = (SIdentifier*)malloc(sizeof(SIdentifier) * (size_t)(len));
	{
		int cnt2[3];
		cnt2[0] = *cnt;
		cnt2[1] = 0;
		cnt2[2] = 1;
		DictForEach(ast->ScopeChildren, MakeIdentifierSet5, cnt2);
		qsort(IdentifierSets[*cnt].Identifiers, len, sizeof(SIdentifier), CmpIdentifier);
	}
	(*cnt)++;
	return value;
}

static const void* MakeIdentifierSet4(const Char* key, const void* value, void* param)
{
	int* len = (int*)param;
	SAst* ast = (SAst*)value;
	UNUSED(key);
	if (ast->Name != NULL)
		(*len)++;
	if (ast->ScopeChildren != NULL)
		DictForEach(ast->ScopeChildren, MakeIdentifierSet4, len);
	return value;
}

static const void* MakeIdentifierSet5(const Char* key, const void* value, void* param)
{
	int* cnt = (int*)param;
	SAst* ast = (SAst*)value;
	if (ast->Name != NULL)
	{
		size_t name_len = wcslen(ast->Name);
		SIdentifier* identifier = &IdentifierSets[cnt[0]].Identifiers[cnt[1]];
		UNUSED(key);
		if (cnt[2] == 1)
		{
			identifier->Name = (Char*)malloc(sizeof(Char) * (name_len + 2));
			identifier->Name[0] = L'@';
			wcscpy(identifier->Name + 1, ast->Name);
		}
		else
		{
			identifier->Name = (Char*)malloc(sizeof(Char) * (name_len + 1));
			wcscpy(identifier->Name, ast->Name);
		}
		identifier->Row = ast->Pos->Row;
		identifier->Col = ast->Pos->Col;
		ASSERT(ast->ScopeRowBegin == NULL && ast->ScopeRowEnd == NULL || ast->ScopeRowBegin != NULL && ast->ScopeRowEnd != NULL);
		identifier->ScopeRowBegin = ast->ScopeRowBegin == NULL ? -1 : (*ast->ScopeRowBegin)->Pos->Row;
		identifier->ScopeRowEnd = ast->ScopeRowEnd == NULL ? -1 : (*ast->ScopeRowEnd)->Pos->Row;
		ASSERT(identifier->ScopeRowBegin == -1 || identifier->ScopeRowBegin <= identifier->Row && identifier->Row <= identifier->ScopeRowEnd);
		{
			int hint_len;
			const Char* hint;
			if (ast->AnalyzedCache == NULL)
			{
				hint = L"unreferenced";
				hint_len = (int)wcslen(hint);
			}
			else
				hint = GetDefinition(&hint_len, ast->AnalyzedCache);
			identifier->Hint = (Char*)malloc(sizeof(Char) * (size_t)(hint_len + 1));
			wcscpy(identifier->Hint, hint);
		}
		cnt[1]++;
	}
	if (ast->ScopeChildren != NULL)
	{
		int cnt2 = cnt[2];
		cnt[2] = 0;
		DictForEach(ast->ScopeChildren, MakeIdentifierSet5, cnt);
		cnt[2] = cnt2;
	}
	return value;
}

static int CmpIdentifierSet(const void* a, const void* b)
{
	const SIdentifierSet* a2 = (const SIdentifierSet*)a;
	const SIdentifierSet* b2 = (const SIdentifierSet*)b;
	return wcscmp(a2->Src, b2->Src);
}

static int CmpIdentifier(const void* a, const void* b)
{
	const SIdentifier* a2 = (const SIdentifier*)a;
	const SIdentifier* b2 = (const SIdentifier*)b;
	int result = wcscmp(a2->Name, b2->Name);
	if (result == 0)
		result = a2->Row - b2->Row;
	return result;
}

static SPos* AddrToPos(U64 addr, Char* name)
{
	SPos* result = NULL;
	void* params[3];
	params[0] = (void*)&result;
	params[1] = (void*)addr;
	params[2] = name;
	DictIForEach(PackAsm.FuncAddrs, AddrToPosCallback, params);
	return result;
}

static const void* AddrToPosCallback(U64 key, const void* value, void* param)
{
	SAstFunc* func = (SAstFunc*)key;
	void** params = (void**)param;
	SPos** result = (SPos**)params[0];
	U64 addr = (U64)params[1];
	Char* name = (Char*)params[2];
	UNUSED(value);
	if ((U64)*func->AddrTop + DbgStartAddr <= addr && addr <= (U64)func->AddrBottom + DbgStartAddr)
	{
		*result = (SPos*)((SAst*)func)->Pos;
		wcscpy(name, ((SAst*)func)->Name);
	}
	return value;
}
