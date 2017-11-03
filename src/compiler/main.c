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
#define HINT_MSG_NUM (2)
#define HINT_MSG_MAX (4096)
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

typedef struct SKeyword
{
	const Char* SrcName;
	const Char* Name;
	const SAst* Ast;
	int* First;
	int* Last;
} SKeyword;

typedef struct SKeywordList
{
	struct SKeywordList* Next;
	const SKeyword* Keyword;
} SKeywordList;

typedef struct SKeywordCallbackParam
{
	const Char* Src;
	SKeywordList** Top;
	SKeywordList** Bottom;
	int* Cnt;
	int* First;
	int* Last;
	EAstTypeId Type;
} SKeywordCallbackParam;

static const void*(*FuncGetSrc)(const U8*) = NULL;
static void(*FuncLog)(const void*, S64, S64) = NULL;
static const void* Src = NULL;
static const void* SrcLine = NULL;
static const Char* SrcChar = NULL;
static SPackAsm PackAsm;
static U64 DbgStartAddr;
static SErrMsg ExcptMsgs[MSG_NUM];
static Bool MsgLoaded = (Bool)0;
static Char HintBuf[HINT_MSG_NUM][0x08 + HINT_MSG_MAX + 1];
static Char HintSrcBuf[0x08 + KUIN_MAX_PATH + 1];
static int KeywordNum = 0;
static const SKeyword** Keywords = NULL;

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
static void WriteHintDef(Char* buf, size_t* len, const SAst* ast);
static void MakeKeywords(SDict* asts);
static const void* MakeKeywordsCallback(const Char* key, const void* value, void* param);
static void MakeKeywordsRecursion(SKeywordCallbackParam* param, const SAst* ast);
static int CmpKeyword(const void* a, const void* b);
static void SearchAst(int* first, int* last, const Char* src, const Char* keyword);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void InitCompiler(S64 mem_num, S64 lang)
{
	InitAllocator(mem_num);
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
	InitAllocator(1);
	result = Build(_wfopen, fclose, fgetwc, BuildFileGetSize, path, sys_dir, output, icon, rls, env, func_log, lang);
	FinAllocator();
	return result;
}

EXPORT void Interpret1(const void* src, const void* color)
{
	InterpretImpl1(src, color);
}

EXPORT Bool Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang, S64 blank_mem)
{
	Bool result = False;
	const Char* sys_dir2 = sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10);

	FuncGetSrc = func_get_src;
	FuncLog = func_log;

	ResetAllocator(blank_mem);

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
				MakeKeywords(asts);
				ResetAllocator(1 - blank_mem);
				Analyze(asts, &option, &dlls);
				result = True;
			}
		}
	}

	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return result;
}

EXPORT void Version(S64* major, S64* minor, S64* micro)
{
	*major = 2017;
	*minor = 11;
	*micro = 17;
}

EXPORT void ResetMemAllocator(S64 mem_idx)
{
	ResetAllocator(mem_idx);
}

EXPORT void ResetKeywords(void)
{
	Keywords = NULL;
}

EXPORT void* GetHint(S64 buf_idx, const U8* src, S64 row, const U8* keyword, void** hint_src, S64* hint_row, S64* hint_col)
{
	ASSERT(0 <= buf_idx && buf_idx < HINT_MSG_NUM);
	const Char* keyword2 = (const Char*)(keyword + 0x10);
	int first;
	int last;
	Bool global = keyword2[0] == L'%' || keyword2[0] == L'.' || IsReserved(keyword2);
	SearchAst(&first, &last, global ? L"" : (const Char*)(src + 0x10), keyword2);
	if (first == -1)
		return NULL;
	size_t len = 0;
	const SAst* hint_ast = NULL;
	*hint_src = NULL;
	if (keyword2[0] == L'@' || keyword2[0] == L'%' || keyword2[0] == L'.')
	{
		int i;
		for (i = first; i <= last; i++)
		{
			if (Keywords[i]->Ast == NULL)
				len += swprintf(HintBuf[buf_idx] + 0x08 + len, HINT_MSG_MAX - len, L"%s", keyword2 + 1);
			else
			{
				WriteHintDef(HintBuf[buf_idx] + 0x08, &len, Keywords[i]->Ast);
				if (hint_ast == NULL)
					hint_ast = Keywords[i]->Ast;
			}
			if (i != last)
			{
				if (len < HINT_MSG_MAX)
				{
					HintBuf[buf_idx][0x08 + len] = L'\n';
					len++;
				}
			}
		}
	}
	else if (global) // Reserved.
		len += swprintf(HintBuf[buf_idx] + 0x08 + len, HINT_MSG_MAX - len, L"%s", keyword2);
	else
	{
		int i;
		const SAst* ast = NULL;
		const int row2 = (int)row;
		for (i = first; i <= last; i++)
		{
			if (*Keywords[i]->First <= row2 && row2 <= *Keywords[i]->Last)
			{
				ast = Keywords[i]->Ast;
				break;
			}
		}
		if (ast == NULL)
			return NULL;
		WriteHintDef(HintBuf[buf_idx] + 0x08, &len, ast);
		hint_ast = ast;
	}
	if (len == 0)
		return NULL;
	if (buf_idx == 0 && hint_ast != NULL && hint_ast->Pos != NULL)
	{
		size_t len2 = wcslen(hint_ast->Pos->SrcName);
		if (len2 <= KUIN_MAX_PATH)
		{
			*(S64*)(HintSrcBuf + 0x00) = 2;
			*(S64*)(HintSrcBuf + 0x04) = (S64)len2;
			wcscpy(HintSrcBuf + 0x08, hint_ast->Pos->SrcName);
			*hint_src = (U8*)HintSrcBuf;
			*hint_row = (S64)hint_ast->Pos->Row;
			*hint_col = (S64)hint_ast->Pos->Col;
		}
	}
	*(S64*)(HintBuf[buf_idx] + 0x00) = DefaultRefCntFunc + 1;
	*(S64*)(HintBuf[buf_idx] + 0x04) = (S64)len;
	return HintBuf[buf_idx];
}

EXPORT S64 GetKeywords(const U8* src, S64 row, const U8* keyword, void* callback)
{
	const Char* src2 = (const Char*)(src + 0x10);
	const Char* keyword2 = (const Char*)(keyword + 0x10);
	int i;
	Char buf[0x08 + 4096];
	S64 best = -1;
	S64 idx = 0;
	const Char* old = NULL;
	for (i = 0; i < KeywordNum; i++)
	{
		const Char* name = Keywords[i]->Name;
		if (keyword2[0] == L'@')
		{
			if (name[0] != L'@')
				continue;
			if (wcscmp(Keywords[i]->SrcName, src2) != 0)
				continue;
		}
		else if (keyword2[0] == L'%' || keyword2[0] == L'.')
		{
			if (name[0] != keyword2[0])
				continue;
		}
		else // Reserved or local identifier.
		{
			if (name[0] == L'@' || name[0] == L'%' || name[0] == L'.')
				continue;
			if (Keywords[i]->SrcName[0] != L'\0' && !(wcscmp(Keywords[i]->SrcName, src2) == 0 && *Keywords[i]->First <= (int)row && (int)row <= *Keywords[i]->Last))
				continue;
		}
		if (old != NULL && wcscmp(old, name) == 0)
			continue;
		old = name;
		size_t len = (size_t)swprintf(buf + 0x08, 4096, L"%s", name);
		((S64*)buf)[0] = 2;
		((S64*)buf)[1] = (S64)len;
		Call1Asm(buf, callback);
		if (best == -1 && wcsstr(buf + 0x08, keyword2) == buf + 0x08)
			best = idx;
		idx++;
	}
	return best;
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
		S64 excpt_last_occurred = _time64(NULL) - 2;
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
					if ((debug_event.u.Exception.ExceptionRecord.ExceptionCode & 0xffff0000) != 0xc0000000 && (debug_event.u.Exception.ExceptionRecord.ExceptionCode & 0xffff0000) != 0xe9170000)
						break;
					if (_time64(NULL) - excpt_last_occurred < 2)
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
#if defined(_DEBUG)
							PVOID addr = debug_event.u.Exception.ExceptionRecord.ExceptionAddress;
#endif
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
								Char name[256];
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
					excpt_last_occurred = _time64(NULL);
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
#if defined(_DEBUG)
	MakeKeywords(asts);
	{
		FILE* fp = _wfopen(NewStr(NULL, L"%s_keywords.txt", option.OutputFile), L"w, ccs=UTF-8");
		fwprintf(fp, L"%d\n", KeywordNum);
		int i;
		for (i = 0; i < KeywordNum; i++)
			fwprintf(fp, L"%s(%s) = %s: %d, %d - %d\n", Keywords[i]->Name, (Keywords[i]->Ast == NULL || Keywords[i]->Ast->RefName == NULL) ? L"" : Keywords[i]->Ast->RefName, Keywords[i]->SrcName, Keywords[i]->Ast == NULL ? -1 : Keywords[i]->Ast->Pos->Row, Keywords[i]->First == NULL ? -1 : *Keywords[i]->First, Keywords[i]->Last == NULL ? -1 : *Keywords[i]->Last);
		fclose(fp);
	}
#endif
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
		if (*result != NULL)
			swprintf(name, 255, L"%s@%s", (*result)->SrcName, ((SAst*)func)->Name);
		else
			wcscpy(name, ((SAst*)func)->Name);
	}
	return value;
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
		case AstTypeId_ExprValue:
			if (*len < HINT_MSG_MAX)
			{
				if (ast->Name == NULL)
					*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"%I64d", *(S64*)((SAstExprValue*)ast)->Value);
				else
					*len += swprintf(buf + *len, HINT_MSG_MAX - *len, L"%%%s :: %I64d", ast->Name, *(S64*)((SAstExprValue*)ast)->Value);
			}
			break;
	}
}

static void MakeKeywords(SDict* asts)
{
	SKeywordCallbackParam param;
	param.Src = NULL;
	SKeywordList* top = NULL;
	SKeywordList* bottom = NULL;
	param.Top = &top;
	param.Bottom = &bottom;
	int cnt = 0;
	param.Cnt = &cnt;
	int* first = (int*)Alloc(sizeof(int));
	int* last = (int*)Alloc(sizeof(int));
	*first = INT_MAX;
	*last = INT_MIN;
	param.First = first;
	param.Last = last;
	param.Type = AstTypeId_Ast;
	DictForEach(asts, MakeKeywordsCallback, &param);
	int reserved_num = GetReservedNum();
	int build_in_funcs_num = GetBuildInFuncsNum();
	cnt += reserved_num + build_in_funcs_num;
	Keywords = (SKeyword**)Alloc(sizeof(SKeyword*) * (size_t)cnt);
	{
		int idx = 0;
		SKeywordList* ptr = *param.Top;
		while (ptr != NULL)
		{
			Keywords[idx] = ptr->Keyword;
			idx++;
			ptr = ptr->Next;
		}
		{
			const Char** reserved = GetReserved();
			int i;
			for (i = 0; i < reserved_num; i++)
			{
				SKeyword* keyword = (SKeyword*)Alloc(sizeof(SKeyword));
				keyword->SrcName = L"";
				keyword->Name = reserved[i];
				keyword->Ast = NULL;
				keyword->First = NULL;
				keyword->Last = NULL;
				Keywords[idx] = keyword;
				idx++;
			}
		}
		{
			const Char** build_in_funcs = GetBuildInFuncs();
			int i;
			for (i = 0; i < build_in_funcs_num; i++)
			{
				SKeyword* keyword = (SKeyword*)Alloc(sizeof(SKeyword));
				keyword->SrcName = L"";
				keyword->Name = NewStr(NULL, L".%s", build_in_funcs[i]);
				keyword->Ast = NULL;
				keyword->First = NULL;
				keyword->Last = NULL;
				Keywords[idx] = keyword;
				idx++;
			}
		}
		ASSERT(idx == cnt);
		KeywordNum = cnt;
	}
	qsort((void*)Keywords, (size_t)KeywordNum, sizeof(SKeyword*), CmpKeyword);
}

static const void* MakeKeywordsCallback(const Char* key, const void* value, void* param)
{
	SKeywordCallbackParam* param2 = (SKeywordCallbackParam*)param;
	const SAst* ast = (const SAst*)value;
	SKeywordCallbackParam param3 = *param2;
	if (param3.Src == NULL)
		param3.Src = key;
	MakeKeywordsRecursion(&param3, ast);
	return value;
}

static void MakeKeywordsRecursion(SKeywordCallbackParam* param, const SAst* ast)
{
	if (ast->Pos != NULL && wcscmp(ast->Pos->SrcName, param->Src) == 0 && ast->Pos->Row != -1 &&
		(ast->Pos->SrcName[0] == L'\\' || param->Type == AstTypeId_Root || param->Type == AstTypeId_Enum || param->Type == AstTypeId_Class))
	{
		const int row = ast->Pos->Row;
		if (*param->First > row)
			*param->First = row;
		if (*param->Last < row)
			*param->Last = row;
		if (ast->Name != NULL && wcscmp(ast->Name, L"me") != 0 &&
			!(ast->Pos->SrcName[0] != L'\\' && ast->Name[0] == L'_' && !(L'0' <= ast->Name[1] && ast->Name[1] <= L'9')))
		{
			SKeywordList* node = (SKeywordList*)Alloc(sizeof(SKeywordList));
			SKeyword* keyword = (SKeyword*)Alloc(sizeof(SKeyword));
			keyword->SrcName = ast->Pos->SrcName;
			keyword->Name = ast->Name;
			switch (param->Type)
			{
				case AstTypeId_Root:
					keyword->Name = NewStr(NULL, L"@%s", ast->Name);
					break;
				case AstTypeId_Enum:
					keyword->SrcName = L"";
					keyword->Name = NewStr(NULL, L"%%%s", ast->Name);
					break;
				case AstTypeId_Class:
					if (ast->TypeId == AstTypeId_Arg || ast->TypeId == AstTypeId_Func)
					{
						keyword->SrcName = L"";
						keyword->Name = NewStr(NULL, L".%s", ast->Name);
					}
					break;
			}
			keyword->Ast = ast;
			keyword->First = param->First;
			keyword->Last = param->Last;
			node->Keyword = keyword;
			node->Next = NULL;
			if (*param->Top == NULL)
				*param->Top = node;
			else
				(*param->Bottom)->Next = node;
			*param->Bottom = node;
			(*param->Cnt)++;
		}
	}
	if (ast->ScopeChildren != NULL)
	{
		SKeywordCallbackParam param2 = *param;
		int* first = (int*)Alloc(sizeof(int));
		int* last = (int*)Alloc(sizeof(int));
		*first = INT_MAX;
		*last = INT_MIN;
		param2.First = first;
		param2.Last = last;
		param2.Type = ast->TypeId;
		DictForEach(ast->ScopeChildren, MakeKeywordsCallback, &param2);
	}
}

static int CmpKeyword(const void* a, const void* b)
{
	const SKeyword* a2 = *(const SKeyword**)a;
	const SKeyword* b2 = *(const SKeyword**)b;
	int cmp;
	cmp = wcscmp(a2->Name, b2->Name);
	if (cmp != 0)
		return cmp;
	cmp = wcscmp(a2->SrcName, b2->SrcName);
	if (cmp != 0)
		return cmp;
	if (a2->First != NULL && b2->First != NULL)
	{
		cmp = *a2->First - *b2->First;
		if (cmp != 0)
			return cmp;
		if (a2->Last != NULL && b2->Last != NULL)
			cmp = *b2->Last - *a2->Last;
	}
	return cmp;
}

static void SearchAst(int* first, int* last, const Char* src, const Char* keyword)
{
	int min = 0;
	int max = KeywordNum - 1;
	while (min <= max)
	{
		int mid = (min + max) / 2;
		int cmp = wcscmp(keyword, Keywords[mid]->Name);
		if (cmp == 0)
			cmp = wcscmp(src, Keywords[mid]->SrcName);
		if (cmp < 0)
			max = mid - 1;
		else if (cmp > 0)
			min = mid + 1;
		else
		{
			*first = mid;
			while (*first > 0 && wcscmp(Keywords[*first - 1]->SrcName, src) == 0 && wcscmp(Keywords[*first - 1]->Name, keyword) == 0)
				(*first)--;
			*last = mid;
			while (*last < KeywordNum - 1 && wcscmp(Keywords[*last + 1]->SrcName, src) == 0 && wcscmp(Keywords[*last + 1]->Name, keyword) == 0)
				(*last)++;
			return;
		}
	}
	*first = -1;
	*last = -1;
}
