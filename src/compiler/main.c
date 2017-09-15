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

#pragma warning(push)
#pragma warning(disable:4091)
#include <DbgHelp.h> // 'StackWalk64'
#pragma warning(pop)

#pragma comment(lib, "DbgHelp.lib")

#define MSG_NUM (57 / 3)
#define FUNC_NAME_MAX (128)
#define MSG_MAX (128)
#define LANG_NUM (2)
#define HINT_MSG_MAX (1024)
#define EXCPT_MSG_MAX (4096)

typedef struct SExcptMsg
{
	S64 Code;
	Char Msg[MSG_MAX + 1];
} SErrMsg;

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
static SPackAsm PackAsm;
static U64 DbgStartAddr;
static SErrMsg ExcptMsgs[MSG_NUM];
static Bool MsgLoaded = (Bool)0;
static SDict* HintAsts = NULL;
static Char HintBuf[0x08 + HINT_MSG_MAX + 1];

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static void LoadExcptMsg(S64 lang);
static void DecSrc(void);
static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang);
static FILE* BuildMemWfopen(const Char* file_name, const Char* mode);
static int BuildMemFclose(FILE* file_ptr);
static U16 BuildMemFgetwc(FILE* file_ptr);
static size_t BuildMemGetSize(FILE* file_ptr);
static void BuildMemLog(const Char* code, const Char* msg, const Char* src, int row, int col);
static size_t BuildFileGetSize(FILE* file_ptr);
static SPos* AddrToPos(U64 addr, Char* name);
static const void* AddrToPosCallback(U64 key, const void* value, void* param);
static const SAst* SearchHint(const Char* src, int row, int col, const SAst* ast);
static const SAst* SearchHintList(const Char* src, int row, int col, SList* list);
static Bool CmpHintPos(const Char* src, int row, int col, const SPos* pos);
static const SAst* BetterHint(const SAst* a, const SAst* b);
static void WriteHint(Char* buf, size_t* len, const SAst* ast);
static void WriteHintDef(Char* buf, size_t* len, const SAst* ast);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void InitCompiler(S64 lang)
{
	InitAllocator();
	if (lang >= 0)
		LoadExcptMsg(lang);
}

EXPORT void FinCompiler(void)
{
	FinAllocator();
}

EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang)
{
	// This function is for the Kuin Editor.
	Bool result;
	FuncGetSrc = func_get_src;
	FuncLog = func_log;
	result = Build(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, (const Char*)(path + 0x10), sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10), output == NULL ? NULL : (const Char*)(output + 0x10), icon == NULL ? NULL : (const Char*)(icon + 0x10), rls, env == NULL ? NULL : (const Char*)(env + 0x10), BuildMemLog, lang);
	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return result;
}

EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang)
{
	// This function is for 'kuincl'.
	Bool result;
	InitAllocator();
	result = Build(_wfopen, fclose, fgetwc, BuildFileGetSize, path, sys_dir, output, icon, rls, env, func_log, lang);
	FinAllocator();
	return result;
}

EXPORT void Interpret1(const void* src, const void* color)
{
	InterpretImpl1(src, color);
}

EXPORT void Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang)
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

	SetLogFunc(BuildMemLog, (int)lang, sys_dir2);
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
				HintAsts = asts;
				Analyze(asts, &option, &dlls);
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
	*major = 2017;
	*minor = 9;
	*micro = 17;
}

EXPORT void ResetMemAllocator(void)
{
	ResetAllocator();
	HintAsts = NULL;
}

EXPORT void* GetHint(const U8* src, S64 row, S64 col)
{
	const Char* src2 = (const Char*)(src + 0x10);
	if (HintAsts == NULL)
		return NULL;
	const SAst* root = (const SAst*)DictSearch(HintAsts, src2);
	if (root == NULL)
		return NULL;
	const SAst* best = SearchHint(src2, (int)row, (int)col, root);
	if (best == NULL)
		return NULL;
	size_t len = 0;
	WriteHint(HintBuf + 0x08, &len, best);
	if (len == 0)
		return NULL;
	*(S64*)(HintBuf + 0x00) = DefaultRefCntFunc + 1;
	*(S64*)(HintBuf + 0x04) = (S64)len;
	return HintBuf;
}

EXPORT Bool RunDbg(const U8* path, const U8* cmd_line, void* idle_func, void* event_func)
{
	const Char* path2 = (const Char*)(path + 0x10);
	Char cur_dir[KUIN_MAX_PATH + 1];
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
		cmd_line_buf = (Char*)malloc(sizeof(Char) * (len + 1));
		wcscpy(cmd_line_buf, (const Char*)(cmd_line + 0x10));
	}
	{
		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(STARTUPINFO);
		if (!CreateProcess(path2, cmd_line_buf, NULL, NULL, FALSE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS | DEBUG_ONLY_THIS_PROCESS, NULL, cur_dir, &startup_info, &process_info))
		{
			if (cmd_line_buf != NULL)
				free(cmd_line_buf);
			return False;
		}
	}
	if (cmd_line_buf != NULL)
		free(cmd_line_buf);

	{
		DEBUG_EVENT debug_event;
		Bool end = False;
		DbgStartAddr = 0;
		ResumeThread(process_info.hThread);
		Bool excpt_occurred = False;
		Char dbg_code = L'\0';
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
					/*
					if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == 0x80000003)
						break;
					*/
					if ((debug_event.u.Exception.ExceptionRecord.ExceptionCode & 0xc0000000) != 0xc0000000)
						break;
					if (excpt_occurred)
						break;
					{
						Char str[EXCPT_MSG_MAX + 1];
						size_t len2;
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
							const Char* text = ExcptMsgs[0].Msg;
							if (code <= 0x0000ffff)
								text = ExcptMsgs[1].Msg;
							else
							{
								int min = 0;
								int max = MSG_NUM - 1;
								int found = -1;
								while (min <= max)
								{
									int mid = (min + max) / 2;
									if ((S64)code < ExcptMsgs[mid].Code)
										max = mid - 1;
									else if ((S64)code > ExcptMsgs[mid].Code)
										min = mid + 1;
									else
									{
										found = mid;
										break;
									}
								}
								if (found != -1)
									text = ExcptMsgs[found].Msg;
							}
#if defined(_DEBUG)
							len2 = swprintf(str, EXCPT_MSG_MAX, L"An exception '0x%08X' occurred at '0x%016I64X'.\r\n\r\n> %s\r\n\r\n", code, (U64)addr, text);
#else
							len2 = swprintf(str, EXCPT_MSG_MAX, L"An exception '0x%08X' occurred.\r\n\r\n> %s\r\n\r\n", code, text);
#endif
						}

						for (; ; )
						{
							if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process_info.hProcess, process_info.hThread, &stack, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
								break;

#if defined(_DEBUG)
							if (len2 < EXCPT_MSG_MAX)
								len2 += swprintf(str + len2, EXCPT_MSG_MAX - len2, L"0x%016I64X: \t", context.Rip);
#endif

							symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
							symbol.MaxNameLength = 255;
							DWORD64 displacement;
							if (SymGetSymFromAddr64(process_info.hProcess, (DWORD64)stack.AddrPC.Offset, &displacement, &symbol))
							{
#if defined(_DEBUG)
								char name[1024];
								UnDecorateSymbolName(symbol.Name, (PSTR)name, 1024, UNDNAME_COMPLETE);
								{
									char* src = name;
									if (len2 < EXCPT_MSG_MAX)
									{
										str[len2] = L'(';
										len2++;
									}
									while (*src != L'\0')
									{
										if (len2 < EXCPT_MSG_MAX)
											str[len2] = (Char)*src;
										len2++;
										src++;
									}
									if (len2 < EXCPT_MSG_MAX)
									{
										str[len2] = L')';
										len2++;
									}
									if (len2 < EXCPT_MSG_MAX)
									{
										str[len2] = L'\r';
										len2++;
									}
									if (len2 < EXCPT_MSG_MAX)
									{
										str[len2] = L'\n';
										len2++;
									}
								}
#endif
							}
							else
							{
								Char name[129];
								SPos* pos = AddrToPos((U64)context.Rip, name);
								if (pos != NULL)
								{
									if (len2 < EXCPT_MSG_MAX)
										len2 += swprintf(str + len2, EXCPT_MSG_MAX - len2, L"%s (%s: %d, %d)\r\n", name, pos->SrcName, pos->Row, pos->Col);
								}
								else
								{
									if (len2 < EXCPT_MSG_MAX)
									{
										str[len2] = L'\r';
										len2++;
									}
									if (len2 < EXCPT_MSG_MAX)
									{
										str[len2] = L'\n';
										len2++;
									}
								}
							}
							if (stack.AddrPC.Offset == 0)
								break;
						}
						str[len2] = L'\0';
						MessageBox(NULL, str, NULL, MB_ICONEXCLAMATION | MB_SETFOREGROUND);
					}
					excpt_occurred = True;
					break;
				case OUTPUT_DEBUG_STRING_EVENT:
					{
						void* buf = NULL;
						if (debug_event.u.DebugString.fUnicode == 0)
						{
							char* buf2 = (char*)malloc((size_t)debug_event.u.DebugString.nDebugStringLength);
							SIZE_T size = 0;
							if (!ReadProcessMemory(process_info.hProcess, debug_event.u.DebugString.lpDebugStringData, buf2, debug_event.u.DebugString.nDebugStringLength, &size) || size == 0)
							{
								free(buf2);
								break;
							}
							int size2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf2, (int)size, NULL, 0);
							buf = malloc(0x10 + sizeof(Char) * (size_t)size2);
							MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf2, (int)size, (Char*)((U8*)buf + 0x10), size2);
							((S64*)buf)[1] = (S64)size2 - 1;
							free(buf2);
						}
						else
						{
							buf = malloc(0x10 + sizeof(Char) * (size_t)debug_event.u.DebugString.nDebugStringLength);
							SIZE_T size = 0;
							if (!ReadProcessMemory(process_info.hProcess, debug_event.u.DebugString.lpDebugStringData, (Char*)((U8*)buf + 0x10), debug_event.u.DebugString.nDebugStringLength, &size) || size == 0)
							{
								free(buf);
								break;
							}
							((S64*)buf)[1] = (S64)debug_event.u.DebugString.nDebugStringLength - 1;
						}
						*((S64*)buf) = 2;
						if (((S64*)buf)[1] >= 5)
						{
							const Char* ptr = (const Char*)((U8*)buf + 0x10);
							if (ptr[0] == L'd' && ptr[1] == L'b' && ptr[2] == L'g' && L'0' <= ptr[3] && ptr[3] <= L'9' && ptr[3] != dbg_code && ptr[4] == L'!')
							{
								dbg_code = ptr[3];
								Call2Asm(0, buf, event_func);
							}
						}
						free(buf);
					}
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

static void LoadExcptMsg(S64 lang)
{
	if (!MsgLoaded)
	{
		FILE* file_ptr;
		ASSERT(0 <= lang && lang < LANG_NUM); // 0 = 'Ja', 1 = 'En'.
		{
			file_ptr = _wfopen(L"sys/excpt.knd", L"r, ccs=UTF-8");
			if (file_ptr == NULL)
				return;
		}
		{
			int i;
			int j;
			Char buf[256];
#if defined(_DEBUG)
			S64 prev_code = 0;
#endif
			for (i = 0; i < MSG_NUM; i++)
			{
				ReadFileLine(buf, 256, file_ptr);
				if (wcscmp(buf, L"none") == 0)
					ExcptMsgs[i].Code = -1;
				else
				{
					ASSERT(wcslen(buf) == 8);
					ExcptMsgs[i].Code = (S64)(U32)_wcstoui64(buf, NULL, 16);
				}
#if defined(_DEBUG)
				ASSERT(i <= 1 || prev_code < ExcptMsgs[i].Code);
				prev_code = ExcptMsgs[i].Code;
#endif
				for (j = 0; j < LANG_NUM; j++)
				{
					ReadFileLine(buf, 256, file_ptr);
					ASSERT(wcslen(buf) < MSG_MAX);
					if (j == lang)
						wcscpy(ExcptMsgs[i].Msg, buf);
				}
			}
			ASSERT(fgetwc(file_ptr) == WEOF);
		}
		fclose(file_ptr);
		MsgLoaded = True;
	}
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

static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang)
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

	SetLogFunc(func_log, (int)lang, sys_dir);
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
		U8 file_name2[0x10 + sizeof(Char) * (KUIN_MAX_PATH + 1)];
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
	U8 code2[0x10 + sizeof(Char) * (KUIN_MAX_PATH + 1)];
	size_t len_msg = wcslen(msg);
	U8* msg2 = (U8*)Alloc(0x10 + sizeof(Char) * (len_msg + 1));
	U8 src2[0x10 + sizeof(Char) * (KUIN_MAX_PATH + 1)];
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
	S64 file_size;
	_fseeki64(file_ptr, 0, SEEK_END);
	file_size = _ftelli64(file_ptr);
	_fseeki64(file_ptr, 0, SEEK_SET);
	return (size_t)file_size;
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

static const SAst* SearchHint(const Char* src, int row, int col, const SAst* ast)
{
	if (ast == NULL)
		return NULL;
	const SAst* best = NULL;
	switch (ast->TypeId)
	{
		case AstTypeId_Root:
			best = SearchHintList(src, row, col, ((const SAstRoot*)ast)->Items);
			break;
		case AstTypeId_Func:
		case AstTypeId_FuncRaw:
			{
				const SAstFunc* ast2 = (const SAstFunc*)ast;
				best = SearchHintList(src, row, col, ast2->Args);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Ret));
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
			}
			break;
		case AstTypeId_Var:
			best = SearchHint(src, row, col, (const SAst*)((const SAstVar*)ast)->Var);
			break;
		case AstTypeId_Const:
			best = SearchHint(src, row, col, (const SAst*)((const SAstConst*)ast)->Var);
			break;
		case AstTypeId_Alias:
			best = SearchHint(src, row, col, (const SAst*)((const SAstAlias*)ast)->Type);
			break;
		case AstTypeId_Class:
			{
				const SAstClass* ast2 = (const SAstClass*)ast;
				SListNode* ptr = ast2->Items->Bottom;
				const SAst* found = NULL;
				while (ptr != NULL)
				{
					const SAst* data = ((const SAstClassItem*)ptr->Data)->Def;
					if (CmpHintPos(src, row, col, data->Pos))
					{
						found = data;
						break;
					}
					ptr = ptr->Prev;
				}
				if (found != NULL)
					best = SearchHint(src, row, col, found);
			}
			break;
		case AstTypeId_Enum:
			best = SearchHintList(src, row, col, ((const SAstEnum*)ast)->Items);
			break;
		case AstTypeId_Arg:
			{
				const SAstArg* ast2 = (const SAstArg*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Type);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Expr));
			}
			break;
		case AstTypeId_StatFunc:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatFunc*)ast)->Def);
			break;
		case AstTypeId_StatVar:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatVar*)ast)->Def);
			break;
		case AstTypeId_StatConst:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatConst*)ast)->Def);
			break;
		case AstTypeId_StatAlias:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatAlias*)ast)->Def);
			break;
		case AstTypeId_StatClass:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatClass*)ast)->Def);
			break;
		case AstTypeId_StatEnum:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatEnum*)ast)->Def);
			break;
		case AstTypeId_StatIf:
			{
				const SAstStatIf* ast2 = (const SAstStatIf*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Cond);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
				best = BetterHint(best, SearchHintList(src, row, col, ast2->ElIfs));
				best = BetterHint(best, SearchHintList(src, row, col, ast2->ElseStats));
			}
			break;
		case AstTypeId_StatElIf:
			{
				const SAstStatElIf* ast2 = (const SAstStatElIf*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Cond);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
			}
			break;
		case AstTypeId_StatSwitch:
			{
				const SAstStatSwitch* ast2 = (const SAstStatSwitch*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Cond);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Cases));
				best = BetterHint(best, SearchHintList(src, row, col, ast2->DefaultStats));
			}
			break;
		case AstTypeId_StatCase:
			{
				const SAstStatCase* ast2 = (const SAstStatCase*)ast;
				const SAst* found = NULL;
				SListNode* ptr = ast2->Conds->Bottom;
				while (ptr != NULL)
				{
					const SAstExpr** exprs = (const SAstExpr**)ptr->Data;
					if (CmpHintPos(src, row, col, ((const SAst*)exprs[0])->Pos))
					{
						found = (const SAst*)exprs[0];
						break;
					}
					if (exprs[1] != NULL && CmpHintPos(src, row, col, ((const SAst*)exprs[1])->Pos))
					{
						found = (const SAst*)exprs[1];
						break;
					}
					ptr = ptr->Prev;
				}
				if (found != NULL)
					best = SearchHint(src, row, col, found);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
			}
			break;
		case AstTypeId_StatWhile:
			{
				const SAstStatWhile* ast2 = (const SAstStatWhile*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Cond);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
			}
			break;
		case AstTypeId_StatFor:
			{
				const SAstStatFor* ast2 = (const SAstStatFor*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Start);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Cond));
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Step));
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
			}
			break;
		case AstTypeId_StatTry:
			{
				const SAstStatTry* ast2 = (const SAstStatTry*)ast;
				best = SearchHintList(src, row, col, ast2->Stats);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Catches));
				best = BetterHint(best, SearchHintList(src, row, col, ast2->FinallyStats));
			}
			break;
		case AstTypeId_StatCatch:
			{
				const SAstStatCatch* ast2 = (const SAstStatCatch*)ast;
				const SAst* found = NULL;
				SListNode* ptr = ast2->Conds->Bottom;
				while (ptr != NULL)
				{
					const SAstExpr** exprs = (const SAstExpr**)ptr->Data;
					if (CmpHintPos(src, row, col, ((const SAst*)exprs[0])->Pos))
					{
						found = (const SAst*)exprs[0];
						break;
					}
					if (exprs[1] != NULL && CmpHintPos(src, row, col, ((const SAst*)exprs[1])->Pos))
					{
						found = (const SAst*)exprs[1];
						break;
					}
					ptr = ptr->Prev;
				}
				if (found != NULL)
					best = SearchHint(src, row, col, found);
				best = BetterHint(best, SearchHintList(src, row, col, ast2->Stats));
			}
			break;
		case AstTypeId_StatThrow:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatThrow*)ast)->Code);
			break;
		case AstTypeId_StatBlock:
			best = SearchHintList(src, row, col, ((const SAstStatBlock*)ast)->Stats);
			break;
		case AstTypeId_StatRet:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatRet*)ast)->Value);
			break;
		case AstTypeId_StatDo:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatDo*)ast)->Expr);
			break;
		case AstTypeId_StatAssert:
			best = SearchHint(src, row, col, (const SAst*)((const SAstStatAssert*)ast)->Cond);
			break;
		case AstTypeId_TypeArray:
			best = SearchHint(src, row, col, (const SAst*)((const SAstTypeArray*)ast)->ItemType);
			break;
		case AstTypeId_TypeFunc:
			{
				const SAstTypeFunc* ast2 = (const SAstTypeFunc*)ast;
				const SAst* found = NULL;
				SListNode* ptr = ast2->Args->Bottom;
				while (ptr != NULL)
				{
					const SAstTypeFuncArg* arg = (const SAstTypeFuncArg*)ptr->Data;
					if (CmpHintPos(src, row, col, ((const SAst*)arg->Arg)->Pos))
					{
						found = (const SAst*)arg->Arg;
						break;
					}
					ptr = ptr->Prev;
				}
				if (found != NULL)
					best = SearchHint(src, row, col, found);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Ret));
			}
			break;
		case AstTypeId_TypeGen:
			best = SearchHint(src, row, col, (const SAst*)((const SAstTypeGen*)ast)->ItemType);
			break;
		case AstTypeId_TypeDict:
			{
				const SAstTypeDict* ast2 = (const SAstTypeDict*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->ItemTypeKey);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->ItemTypeValue));
			}
			break;
		case AstTypeId_Expr1:
			best = SearchHint(src, row, col, (const SAst*)((const SAstExpr1*)ast)->Child);
			break;
		case AstTypeId_Expr2:
			{
				const SAstExpr2* ast2 = (const SAstExpr2*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Children[0]);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Children[1]));
			}
			break;
		case AstTypeId_Expr3:
			{
				const SAstExpr3* ast2 = (const SAstExpr3*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Children[0]);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Children[1]));
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Children[2]));
			}
			break;
		case AstTypeId_ExprNew:
			best = SearchHint(src, row, col, (const SAst*)((const SAstExprNew*)ast)->ItemType);
			break;
		case AstTypeId_ExprNewArray:
			{
				const SAstExprNewArray* ast2 = (const SAstExprNewArray*)ast;
				best = SearchHintList(src, row, col, ast2->Idces);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->ItemType));
			}
			break;
		case AstTypeId_ExprAs:
			{
				const SAstExprAs* ast2 = (const SAstExprAs*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Child);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->ChildType));
			}
			break;
		case AstTypeId_ExprToBin:
			{
				const SAstExprToBin* ast2 = (const SAstExprToBin*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Child);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->ChildType));
			}
			break;
		case AstTypeId_ExprFromBin:
			{
				const SAstExprFromBin* ast2 = (const SAstExprFromBin*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Child);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->ChildType));
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Offset));
			}
			break;
		case AstTypeId_ExprCall:
			{
				const SAstExprCall* ast2 = (const SAstExprCall*)ast;
				const SAst* found = NULL;
				SListNode* ptr = ast2->Args->Bottom;
				while (ptr != NULL)
				{
					const SAstExprCallArg* arg = (const SAstExprCallArg*)ptr->Data;
					if (CmpHintPos(src, row, col, ((const SAst*)arg->Arg)->Pos))
					{
						found = (const SAst*)arg->Arg;
						break;
					}
					ptr = ptr->Prev;
				}
				if (found != NULL)
					best = SearchHint(src, row, col, found);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Func));
			}
			break;
		case AstTypeId_ExprArray:
			{
				const SAstExprArray* ast2 = (const SAstExprArray*)ast;
				best = SearchHint(src, row, col, (const SAst*)ast2->Var);
				best = BetterHint(best, SearchHint(src, row, col, (const SAst*)ast2->Idx));
			}
			break;
		case AstTypeId_ExprDot:
			best = SearchHint(src, row, col, (const SAst*)((const SAstExprDot*)ast)->Var);
			break;
		case AstTypeId_ExprValueArray:
			best = SearchHintList(src, row, col, ((const SAstExprValueArray*)ast)->Values);
			break;
	}
	if (ast->Pos != NULL && ast->Pos->Row == row && ast->Pos->Col <= col && wcscmp(ast->Pos->SrcName, src) == 0)
	{
		if (best == NULL || best->Pos->Col < ast->Pos->Col)
			best = ast;
	}
	return best;
}

static const SAst* SearchHintList(const Char* src, int row, int col, SList* list)
{
	SListNode* ptr = list->Bottom;
	const SAst* found = NULL;
	while (ptr != NULL)
	{
		const SAst* data = (const SAst*)ptr->Data;
		if (CmpHintPos(src, row, col, data->Pos))
		{
			found = data;
			break;
		}
		ptr = ptr->Prev;
	}
	if (found == NULL)
		return NULL;
	return SearchHint(src, row, col, found);
}

static Bool CmpHintPos(const Char* src, int row, int col, const SPos* pos)
{
	return pos != NULL && (pos->Row < row || pos->Row == row && pos->Col <= col) && wcscmp(pos->SrcName, src) == 0;
}

static const SAst* BetterHint(const SAst* a, const SAst* b)
{
	if (a == NULL)
		return b;
	if (b == NULL)
		return a;
	if (a->Pos->Col >= b->Pos->Col)
		return a;
	return b;
}

static void WriteHint(Char* buf, size_t* len, const SAst* ast)
{
	switch (ast->TypeId)
	{
		case AstTypeId_StatBreak:
		case AstTypeId_StatSkip:
		case AstTypeId_TypeUser:
		case AstTypeId_ExprRef:
			if (ast->RefItem != NULL)
			{
				if (ast->RefItem->Pos != NULL && *len < HINT_MSG_MAX)
					*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"[%s: %d, %d] ", ast->RefItem->Pos->SrcName, ast->RefItem->Pos->Row, ast->RefItem->Pos->Col);
				WriteHintDef(buf, len, ast->RefItem);
			}
			break;
		case AstTypeId_ExprDot:
			{
				const SAstExprDot* ast2 = (const SAstExprDot*)ast;
				if (ast2->ClassItem != NULL && ast2->ClassItem->Def != NULL)
				{
					const SAst* item = ast2->ClassItem->Def;
					if (item->Pos != NULL && *len < HINT_MSG_MAX)
						*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"[%s: %d, %d] ", item->Pos->SrcName, item->Pos->Row, item->Pos->Col);
					WriteHintDef(buf, len, item);
				}
			}
			break;
	}
}

static void WriteHintDef(Char* buf, size_t* len, const SAst* ast)
{
	switch (ast->TypeId)
	{
		case AstTypeId_Func:
		case AstTypeId_FuncRaw:
			{
				const SAstFunc* ast2 = (const SAstFunc*)ast;
				if (*len < HINT_MSG_MAX)
					*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"func %s(", ast->Name);
				SListNode* ptr = ast2->Args->Top;
				while (ptr != NULL)
				{
					if (ptr != ast2->Args->Top && *len < HINT_MSG_MAX)
						*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L", ");
					WriteHintDef(buf, len, (const SAst*)ptr->Data);
					ptr = ptr->Next;
				}
				if (ast2->Ret == NULL)
				{
					if (*len < HINT_MSG_MAX)
						*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L")");
				}
				else
				{
					if (*len < HINT_MSG_MAX)
						*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"): ");
					WriteHintDef(buf, len, (const SAst*)ast2->Ret);
				}
			}
			break;
		case AstTypeId_Var:
			WriteHintDef(buf, len, (const SAst*)((const SAstVar*)ast)->Var);
			break;
		case AstTypeId_Alias:
			{
				const SAstAlias* ast2 = (const SAstAlias*)ast;
				if (*len < HINT_MSG_MAX)
					*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"alias %s: ", ast->Name);
				WriteHintDef(buf, len, (const SAst*)ast2->Type);
			}
			break;
		case AstTypeId_Class:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"class %s(%s)", ast->Name, ast->RefName != NULL ? ast->RefName : L"");
			break;
		case AstTypeId_Enum:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"enum %s", ast->Name);
			break;
		case AstTypeId_Arg:
			{
				const SAstArg* ast2 = (const SAstArg*)ast;
				if (*len < HINT_MSG_MAX)
					*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"%s: %s", ast->Name, ast2->RefVar ? L"&" : L"");
				WriteHintDef(buf, len, (const SAst*)ast2->Type);
				if (ast2->Expr != NULL)
				{
					if (*len < HINT_MSG_MAX)
						*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L" :: ");
					WriteHintDef(buf, len, (const SAst*)ast2->Expr);
				}
			}
			break;
		case AstTypeId_StatIf:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"if %s()", ast->Name);
			break;
		case AstTypeId_StatSwitch:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"switch %s()", ast->Name);
			break;
		case AstTypeId_StatWhile:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"while %s()", ast->Name);
			break;
		case AstTypeId_StatFor:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"for %s()", ast->Name);
			break;
		case AstTypeId_StatTry:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"try %s", ast->Name);
			break;
		case AstTypeId_StatBlock:
			if (*len < HINT_MSG_MAX)
				*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"block %s", ast->Name);
			break;
		case AstTypeId_TypeArray:
		case AstTypeId_TypeBit:
		case AstTypeId_TypeFunc:
		case AstTypeId_TypeGen:
		case AstTypeId_TypeDict:
		case AstTypeId_TypePrim:
		case AstTypeId_TypeUser:
			GetTypeName(buf, len, HINT_MSG_MAX, (const SAstType*)ast);
			break;
	}
}
