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

#define MSG_NUM (60 / 3)
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

typedef struct SArchiveFileList
{
	struct SArchiveFileList* Next;
	const Char* Path;
} SArchiveFileList;

typedef struct SKeywordListItem
{
	const Char* Name;
	const SAst* Ast;
	int* First;
	int* Last;
} SKeywordListItem;

typedef struct SKeywordList
{
	struct SKeywordList* Next;
	const SKeywordListItem* Keyword;
} SKeywordList;

typedef struct SKeywordListCallbackParam
{
	const Char* Src;
	SKeywordList** Top;
	SKeywordList** Bottom;
	int* Cnt;
	int* First;
	int* Last;
	EAstTypeId ParentType;
} SKeywordListCallbackParam;

typedef struct SBreakPointAddr
{
	U64 Addr;
	U8 Ope;
} SBreakPointAddr;

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
static int KeywordListNum = 0;
static const SKeywordListItem** KeywordList = NULL;
static S64 GlobalHeapCnt = 0;
static S64 BreakPointNum = 0;
static SPos* BreakPointPoses = NULL;
static int BreakPointAddrNum = 0;
static SBreakPointAddr* BreakPointAddrs = NULL;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static void LoadExcptMsg(S64 lang);
static void DecSrc(void);
static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, const void* related_files, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang, S64 app_code, Bool not_deploy);
static FILE* BuildMemWfopen(const Char* file_name, const Char* mode);
static int BuildMemFclose(FILE* file_ptr);
static U16 BuildMemFgetwc(FILE* file_ptr);
static size_t BuildMemGetSize(FILE* file_ptr);
static void BuildMemLog(const Char* code, const Char* msg, const Char* src, int row, int col);
static size_t BuildFileGetSize(FILE* file_ptr);
static SPos* AddrToPos(U64 addr, Char* name);
static const void* AddrToPosCallback(U64 key, const void* value, void* param);
static SPos* AddrToPosCallbackRecursion(const SList* stats, U64 addr);
static U64 PosToAddr(const SPos* pos);
static const void* PosToAddrCallback(U64 key, const void* value, void* param);
static U64 PosToAddrCallbackRecursion(const SList* stats, const SPos* target_pos);
static SArchiveFileList* SearchFiles(int* len, const Char* src);
static Bool SearchFilesRecursion(int* len, size_t src_base_len, const Char* src, SArchiveFileList** top, SArchiveFileList** bottom);
static U8 GetKey(U64 key, U8 data, U64 pos);
static void MakeKeywordList(SDict* asts);
static const void* MakeKeywordListCallback(const Char* key, const void* value, void* param);
static void MakeKeywordListRecursion(SKeywordListCallbackParam* param, const SAst* ast);
static int CmpKeywordListItem(const void* a, const void* b);
static void FreeBreakPoints(void);
static void SetBreakPointOpes(HANDLE process_handle);
static void UnsetBreakPointOpes(HANDLE process_handle);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void InitCompiler(S64 lang)
{
	if (!InitEnvVars(GetProcessHeap(), &GlobalHeapCnt, 0, NULL))
		return;

	InitAllocator();
	if (lang >= 0)
		LoadExcptMsg(lang);

	BreakPointNum = 0;
	BreakPointPoses = NULL;
	BreakPointAddrs = NULL;
}

EXPORT void FinCompiler(void)
{
	if (BreakPointAddrs != NULL)
		FreeMem(BreakPointAddrs);
	FreeBreakPoints();

	FinAllocator();
}

EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, const void* related_files, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang, S64 app_code)
{
	// This function is for the Kuin Editor.
	Bool result;
	FuncGetSrc = func_get_src;
	FuncLog = func_log;
	const Char* icon2;
	if (icon == NULL)
		icon2 = NULL;
	else
	{
		icon2 = (const Char*)(icon + 0x10);
		if (icon2[0] == L'\0')
			icon2 = NULL;
	}
	result = Build(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, (const Char*)(path + 0x10), sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10), output == NULL ? NULL : (const Char*)(output + 0x10), icon2, related_files, rls, env == NULL ? NULL : (const Char*)(env + 0x10), BuildMemLog, lang, app_code, False);
	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return result;
}

EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, const void* related_files, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang, S64 app_code, Bool not_deploy)
{
	// This function is for 'kuincl'.
	Bool result;
	InitAllocator();
	result = Build(_wfopen, fclose, fgetwc, BuildFileGetSize, path, sys_dir, output, icon, related_files, rls, env, func_log, lang, app_code, not_deploy);
	FinAllocator();
	return result;
}

EXPORT void Interpret1(void* src, S64 line, void* me, void* replace_func, S64 cursor_x, S64 cursor_y, S64* new_cursor_x, S64 old_line, S64 new_line)
{
	for (; ; )
	{
		void* str = *(void**)((U8*)src + 0x10);
		void* color = *(void**)((U8*)src + 0x18);
		void* comment_level = *(void**)((U8*)src + 0x20);
		void* flags = *(void**)((U8*)src + 0x28);
		if (InterpretImpl1(str, color, comment_level, flags, line, me, replace_func, cursor_x, cursor_y, new_cursor_x, old_line, new_line))
			break;
		old_line = -1;
	}
}

EXPORT Bool Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col), S64 lang)
{
	Bool result = False;
	const Char* sys_dir2 = sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10);

	FuncGetSrc = func_get_src;
	FuncLog = func_log;

	ResetMemAllocator();

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
		MakeOption(&option, (const Char*)(path + 0x10), NULL, sys_dir2, NULL, False, env == NULL ? NULL : (const Char*)(env + 0x10), False);
		if (!ErrOccurred())
		{
			U8 use_res_flags[USE_RES_FLAGS_LEN] = { 0 };
			asts = Parse(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, &option, use_res_flags);
			if (asts != NULL)
			{
				Analyze(asts, &option, &dlls);
				MakeKeywordList(asts);
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
	*major = 2019;
	*minor = 9;
	*micro = 17;
}

EXPORT void ResetMemAllocator(void)
{
	ResetAllocator();
	KeywordListNum = 0;
	KeywordList = NULL;
}

EXPORT void* GetKeywords(void* src, const U8* src_name, S64 x, S64 y, void* callback)
{
	void* str = *(void**)((U8*)src + 0x10);
	// void* comment_level = *(void**)((U8*)src + 0x20);
	void* flags = *(void**)((U8*)src + 0x28);

	void** str2 = (void**)((U8*)str + 0x10 + 0x08 * (size_t)y);
	// S64 comment_level2 = *(S64*)((U8*)comment_level + 0x10 + 0x08 * (size_t)y);
	U64 flags2 = *(U64*)((U8*)flags + 0x10 + 0x08 * (size_t)y);

	const Char* str3 = (Char*)((U8*)*str2 + 0x10);

	return GetKeywordsRoot(&str3, str3 + x + 1, (const Char*)(src_name + 0x10), (int)x, (int)y, flags2, callback, KeywordListNum, (const void*)KeywordList);
}

EXPORT Bool RunDbg(const U8* path, const U8* cmd_line, void* idle_func, void* event_func, void* break_points_func, void* break_func, void* dbg_func)
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
		DEBUG_EVENT debug_event = { 0 };
		Bool end = False;
		DbgStartAddr = 0;
		ResumeThread(process_info.hThread);
		while (!end)
		{
			DWORD continue_status = DBG_EXCEPTION_NOT_HANDLED;
			Call0Asm(idle_func);
			Sleep(1);
			for (; ; )
			{
				WaitForDebugEvent(&debug_event, 0);
				if (debug_event.dwProcessId != process_info.dwProcessId)
				{
					ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
					continue;
				}
				break;
			}
			switch (debug_event.dwDebugEventCode)
			{
				case CREATE_PROCESS_DEBUG_EVENT:
					DbgStartAddr = (U64)debug_event.u.CreateProcessInfo.lpBaseOfImage;
					if (debug_event.u.CreateProcessInfo.hFile != 0)
						CloseHandle(debug_event.u.CreateProcessInfo.hFile);
					Call0Asm(break_points_func);
					SetBreakPointOpes(process_info.hProcess);
					break;
				case LOAD_DLL_DEBUG_EVENT:
					if (debug_event.u.LoadDll.hFile != 0)
					{
						__try
						{
							CloseHandle(debug_event.u.LoadDll.hFile);
						}
						__except (EXCEPTION_EXECUTE_HANDLER)
						{
							// Do nothing.
						}
					}
					break;
				case EXIT_PROCESS_DEBUG_EVENT:
					end = True;
					break;
				case EXCEPTION_DEBUG_EVENT:
					{
						const DWORD excpt_code = debug_event.u.Exception.ExceptionRecord.ExceptionCode;
						if (excpt_code == EXCEPTION_SINGLE_STEP)
						{
							CONTEXT context;
							context.ContextFlags = CONTEXT_CONTROL;
							GetThreadContext(process_info.hThread, &context);
							Call0Asm(break_points_func);
							SetBreakPointOpes(process_info.hProcess);
							context.EFlags &= ~0x00000100;
							SetThreadContext(process_info.hThread, &context);
							continue_status = DBG_CONTINUE;
							break;
						}
						int break_point_idx = -1;
						if (excpt_code == EXCEPTION_BREAKPOINT)
						{
							if (BreakPointAddrs != NULL)
							{
								int i;
								for (i = 0; i < BreakPointAddrNum; i++)
								{
									if (BreakPointAddrs[i].Addr != 0 && BreakPointAddrs[i].Addr == (U64)debug_event.u.Exception.ExceptionRecord.ExceptionAddress)
									{
										break_point_idx = i;
										break;
									}
								}
							}
						}
						if (break_point_idx == -1 && (excpt_code & 0xffff0000) != 0xc0000000 && (excpt_code & 0xffff0000) != 0xe9170000)
							break;
						Call3Asm((void*)0, NULL, NULL, dbg_func);
						{
							Char str_buf[0x08 + EXCPT_MSG_MAX + 1];
							Char* str = str_buf + 0x08;
							CONTEXT context;
							CONTEXT context2;
							STACKFRAME64 stack;
							IMAGEHLP_SYMBOL64 symbol;
							memset(&context, 0, sizeof(context));
							context.ContextFlags = CONTEXT_FULL;
							if (!GetThreadContext(process_info.hThread, &context))
								break;
							context2 = context;
							memset(&stack, 0, sizeof(stack));
							stack.AddrPC.Offset = context2.Rip;
							stack.AddrPC.Mode = AddrModeFlat;
							stack.AddrStack.Offset = context2.Rsp;
							stack.AddrStack.Mode = AddrModeFlat;
							stack.AddrFrame.Offset = context2.Rbp;
							stack.AddrFrame.Mode = AddrModeFlat;
							SymInitialize(process_info.hProcess, NULL, TRUE);
							{
								const Char* text = ExcptMsgs[0].Msg;
								if (excpt_code <= 0x0000ffff)
									text = ExcptMsgs[1].Msg;
								else
								{
									int min = 0;
									int max = MSG_NUM - 1;
									int found = -1;
									while (min <= max)
									{
										int mid = (min + max) / 2;
										if ((S64)excpt_code < ExcptMsgs[mid].Code)
											max = mid - 1;
										else if ((S64)excpt_code > ExcptMsgs[mid].Code)
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
								swprintf(str, EXCPT_MSG_MAX, L"%s\nAn exception '0x%08X' occurred.", text, excpt_code);
							}

							SPos* excpt_pos = NULL;
							for (; ; )
							{
								if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process_info.hProcess, process_info.hThread, &stack, &context2, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
									break;
								symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
								symbol.MaxNameLength = 255;
								DWORD64 displacement;
								if (SymGetSymFromAddr64(process_info.hProcess, (DWORD64)stack.AddrPC.Offset, &displacement, &symbol))
								{
									/*
									char name[1024];
									UnDecorateSymbolName(symbol.Name, (PSTR)name, 1024, UNDNAME_COMPLETE);
									*/
								}
								else
								{
									Char name[256];
									SPos* pos = AddrToPos((U64)context2.Rip, name);
									if (excpt_pos == NULL)
									{
										if (wcschr(name, L'@') == NULL)
											break;
										else
											excpt_pos = pos;
									}
									if (pos != NULL)
									{
										Char buf[0x08 + 1024];
										swprintf(buf + 0x08, 1024, L"%s (%s: %d, %d)", name, pos->SrcName, pos->Row, pos->Col);
										((S64*)buf)[0] = 2;
										((S64*)buf)[1] = (S64)wcslen(buf + 0x08);
										Call3Asm((void*)2, buf, NULL, dbg_func);
									}
								}
								if (stack.AddrPC.Offset == 0)
									break;
							}
							if (excpt_pos != NULL)
							{
								GetDbgVars(KeywordListNum, KeywordList, excpt_pos->SrcName, excpt_pos->Row, process_info.hProcess, DbgStartAddr, &context, dbg_func);
								{
									void* pos_ptr = NULL;
									Char pos_name[0x08 + 256];
									U8 pos_buf[0x28];
									{
										size_t pos_name_len = wcslen(excpt_pos->SrcName);
										((S64*)pos_name)[0] = 1;
										((S64*)pos_name)[1] = (S64)pos_name_len;
										memcpy((U8*)pos_name + 0x10, excpt_pos->SrcName, sizeof(Char) * (pos_name_len + 1));

										((S64*)pos_buf)[0] = 2;
										((S64*)pos_buf)[1] = 0;
										((void**)pos_buf)[2] = pos_name;
										((S64*)pos_buf)[3] = excpt_pos->Row;
										((S64*)pos_buf)[4] = excpt_pos->Col;
										pos_ptr = pos_buf;
									}
									((S64*)str_buf)[0] = 2;
									((S64*)str_buf)[1] = wcslen(str);
									Call3Asm((void*)(U64)excpt_code, pos_ptr, str_buf, break_func);
								}
								UnsetBreakPointOpes(process_info.hProcess);
							}
							if (break_point_idx != -1)
							{
								context.Rip--;
								context.EFlags |= 0x00000100;
								SetThreadContext(process_info.hThread, &context);
								continue_status = DBG_CONTINUE;
							}
							else
							{
								Call0Asm(break_points_func);
								SetBreakPointOpes(process_info.hProcess);
							}
						}
					}
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
						if (((S64*)buf)[1] >= 4)
						{
							const Char* ptr = (const Char*)((U8*)buf + 0x10);
							if (ptr[0] == L'd' && ptr[1] == L'b' && ptr[2] == L'g' && ptr[3] == L'!')
								Call2Asm(0, buf, event_func);
						}
						free(buf);
					}
					continue_status = DBG_CONTINUE;
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

EXPORT void SetBreakPoints(const void* break_points)
{
	S64 len = ((S64*)break_points)[1];
	FreeBreakPoints();
	BreakPointNum = len;
	BreakPointPoses = (SPos*)AllocMem(sizeof(SPos) * (size_t)len);

	void** ptr = (void**)((U8*)break_points + 0x10);
	S64 i;
	for (i = 0; i < len; i++)
	{
		Char* src_name = (Char*)((U8*)*(void**)((U8*)ptr[i] + 0x10) + 0x10);
		int row = (int)*(S64*)((U8*)ptr[i] + 0x18);
		int col = (int)*(S64*)((U8*)ptr[i] + 0x20);
		S64 name_len = ((S64*)*(void**)((U8*)ptr[i] + 0x10))[1];

		Bool success = False;
		for (; ; )
		{
			Char name[256];
			U64 addr;
			SPos pos;
			pos.SrcName = src_name;
			pos.Row = row;
			pos.Col = col;
			addr = PosToAddr(&pos);
			if (addr != 0)
			{
				SPos* pos2 = AddrToPos(addr, name);
				if (pos2 != NULL && wcscmp(pos.SrcName, pos2->SrcName) == 0)
				{
					if (pos.Row != pos2->Row)
					{
						row = pos2->Row;
						*(S64*)((U8*)ptr[i] + 0x18) = (S64)pos2->Row;
						continue;
					}
					success = True;
				}
			}
			break;
		}
		if (!success)
		{
			BreakPointPoses[i].SrcName = L"";
			BreakPointPoses[i].Row = -1;
			BreakPointPoses[i].Col = -1;
			*(S64*)((U8*)ptr[i] + 0x18) = -1;
			continue;
		}

		Char* buf = (Char*)AllocMem(sizeof(Char) * (size_t)(name_len + 1));
		memcpy(buf, src_name, sizeof(Char) * (size_t)(name_len + 1));
		BreakPointPoses[i].SrcName = buf;
		BreakPointPoses[i].Row = row;
		BreakPointPoses[i].Col = col;
	}
}

EXPORT Bool Archive(const U8* dst, const U8* src, S64 app_code)
{
	FILE* file_ptr = _wfopen((const Char*)(dst + 0x10), L"wb");
	if (file_ptr == NULL)
		return False;
	const Char* src2 = (const Char*)(src + 0x10);
	int len;
	SArchiveFileList* files = SearchFiles(&len, src2);
	if (files == (SArchiveFileList*)DummyPtr || len > 65535)
	{
		fclose(file_ptr);
		return False;
	}
	U64 key = ((U64)(U32)timeGetTime() | ((U64)(U32)time(NULL) << 32)) ^ 0x8364ff023819442e;
	fwrite(&key, sizeof(U64), 1, file_ptr);
	key ^= (U64)app_code * 0x9271ac8394027acb + 0x35718394ca72849e;
	{
		U64 signature = 0x83261772fa0c01a7 ^ key;
		fwrite(&signature, sizeof(U64), 1, file_ptr);
	}
	{
		U64 len2 = (U64)len ^ 0x9c4cab83ce74a67e ^ key;
		fwrite(&len2, sizeof(U64), 1, file_ptr);
	}
	U64 pos = 0;
	{
		SArchiveFileList* ptr = files;
		while (ptr != NULL)
		{
			Char path[KUIN_MAX_PATH + 1];
			swprintf(path, KUIN_MAX_PATH + 1, L"%s%s", src2, ptr->Path);
			FILE* file_ptr2 = _wfopen(path, L"rb");
			if (file_ptr2 == NULL)
			{
				fclose(file_ptr);
				return False;
			}
			_fseeki64(file_ptr2, 0, SEEK_END);
			U64 size = (U64)_ftelli64(file_ptr2);
			_fseeki64(file_ptr2, 0, SEEK_SET);
			{
				U64 path_len = (U64)wcslen(ptr->Path);
				if (path_len > 255)
				{
					fclose(file_ptr);
					fclose(file_ptr2);
					return False;
				}
				U64 i;
				{
					U8* ptr2 = (U8*)&size;
					for (i = 0; i < 8; i++)
					{
						U8 data = GetKey(key, ptr2[i], pos);
						pos++;
						fwrite(&data, sizeof(U8), 1, file_ptr);
					}
				}
				{
					U8* ptr2 = (U8*)&path_len;
					for (i = 0; i < 8; i++)
					{
						U8 data = GetKey(key, ptr2[i], pos);
						pos++;
						fwrite(&data, sizeof(U8), 1, file_ptr);
					}
				}
				{
					U8* ptr2 = (U8*)ptr->Path;
					for (i = 0; i < path_len * 2; i++)
					{
						U8 data = GetKey(key, ptr2[i], pos);
						pos++;
						fwrite(&data, sizeof(U8), 1, file_ptr);
					}
				}
				for (i = 0; i < size; i++)
				{
					U8 data;
					fread(&data, sizeof(U8), 1, file_ptr2);
					data = GetKey(key, data, pos);
					pos++;
					fwrite(&data, sizeof(U8), 1, file_ptr);
				}
			}
			fclose(file_ptr2);
			ptr = ptr->Next;
		}
	}
	fclose(file_ptr);
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

static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, const void* related_files, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang, S64 app_code, Bool not_deploy)
{
	SOption option;
	SDict* asts;
	U8 use_res_flags[USE_RES_FLAGS_LEN] = { 0 };
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
	MakeOption(&option, path, output, sys_dir, icon, rls, env, not_deploy);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0000", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	asts = Parse(func_wfopen, func_fclose, func_fgetwc, func_size, &option, use_res_flags);
	if (asts == NULL || option.Rls && ErrOccurred())
		goto ERR;
	Err(L"IK0001", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	entry = Analyze(asts, &option, &dlls);
	if (ErrOccurred())
		goto ERR;
	if (!option.Rls)
		MakeKeywordList(asts);
	Err(L"IK0002", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	Assemble(&PackAsm, entry, &option, dlls, app_code, use_res_flags);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0003", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	ToMachineCode(&PackAsm, &option);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0004", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	if (!option.NotDeploy)
		Deploy(PackAsm.AppCode, &option, dlls, related_files);
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
	name[0] = '\0';
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
	if ((U64)*func->AddrTop + DbgStartAddr <= addr && addr <= (U64)func->AddrBottom + DbgStartAddr)
	{
		*result = (SPos*)((SAst*)func)->Pos;
		{
			SPos* result2 = AddrToPosCallbackRecursion(func->Stats, addr);
			if (result2 != NULL)
			{
				*result = result2;
				swprintf(name, 255, L"%s@%s", (*result)->SrcName, ((SAst*)func)->Name);
			}
			else
				wcscpy(name, ((SAst*)func)->Name);
		}
	}
	return value;
}

static SPos* AddrToPosCallbackRecursion(const SList* stats, U64 addr)
{
	SListNode* ptr = stats->Top;
	while (ptr != NULL)
	{
		SAstStat* stat = (SAstStat*)ptr->Data;
		SPos* result;
		if (stat->AsmTop != NULL && stat->AsmBottom != NULL && stat->AsmTop->Addr != NULL && stat->AsmBottom->Addr != NULL && (U64)*stat->AsmTop->Addr + DbgStartAddr <= addr && addr <= (U64)*stat->AsmBottom->Addr + DbgStartAddr)
		{
			SListNode* ptr2;
			switch (((SAst*)stat)->TypeId)
			{
				case AstTypeId_StatIf:
					{
						SAstStatIf* stat2 = (SAstStatIf*)stat;
						result = AddrToPosCallbackRecursion(stat2->StatBlock->Stats, addr);
						if (result != NULL)
							return result;
						ptr2 = stat2->ElIfs->Top;
						while (ptr2 != NULL)
						{
							SAstStatElIf* elif = (SAstStatElIf*)ptr2->Data;
							result = AddrToPosCallbackRecursion(elif->StatBlock->Stats, addr);
							if (result != NULL)
								return result;
							ptr2 = ptr2->Next;
						}
						if (stat2->ElseStatBlock != NULL)
						{
							result = AddrToPosCallbackRecursion(stat2->ElseStatBlock->Stats, addr);
							if (result != NULL)
								return result;
						}
					}
					break;
				case AstTypeId_StatSwitch:
					{
						SAstStatSwitch* stat2 = (SAstStatSwitch*)stat;
						ptr2 = stat2->Cases->Top;
						while (ptr2 != NULL)
						{
							SAstStatCase* case_ = (SAstStatCase*)ptr2->Data;
							result = AddrToPosCallbackRecursion(case_->StatBlock->Stats, addr);
							if (result != NULL)
								return result;
							ptr2 = ptr2->Next;
						}
						if (stat2->DefaultStatBlock != NULL)
						{
							result = AddrToPosCallbackRecursion(stat2->DefaultStatBlock->Stats, addr);
							if (result != NULL)
								return result;
						}
					}
					break;
				case AstTypeId_StatWhile:
					{
						SAstStatWhile* stat2 = (SAstStatWhile*)stat;
						result = AddrToPosCallbackRecursion(stat2->Stats, addr);
						if (result != NULL)
							return result;
					}
					break;
				case AstTypeId_StatFor:
					{
						SAstStatFor* stat2 = (SAstStatFor*)stat;
						result = AddrToPosCallbackRecursion(stat2->Stats, addr);
						if (result != NULL)
							return result;
					}
					break;
				case AstTypeId_StatTry:
					{
						SAstStatTry* stat2 = (SAstStatTry*)stat;
						result = AddrToPosCallbackRecursion(stat2->StatBlock->Stats, addr);
						if (result != NULL)
							return result;
						ptr2 = stat2->Catches->Top;
						while (ptr2 != NULL)
						{
							SAstStatCatch* catch_ = (SAstStatCatch*)ptr2->Data;
							result = AddrToPosCallbackRecursion(catch_->StatBlock->Stats, addr);
							if (result != NULL)
								return result;
							ptr2 = ptr2->Next;
						}
						if (stat2->FinallyStatBlock != NULL)
						{
							result = AddrToPosCallbackRecursion(stat2->FinallyStatBlock->Stats, addr);
							if (result != NULL)
								return result;
						}
					}
					break;
				case AstTypeId_StatBlock:
					{
						SAstStatBlock* stat2 = (SAstStatBlock*)stat;
						result = AddrToPosCallbackRecursion(stat2->Stats, addr);
						if (result != NULL)
							return result;
					}
					break;
			}
			return (SPos*)((SAst*)stat)->Pos;
		}
		ptr = ptr->Next;
	}
	return NULL;
}

static U64 PosToAddr(const SPos* pos)
{
	U64 addr = 0;
	void* params[2];
	params[0] = &addr;
	params[1] = (void*)pos;
	DictIForEach(PackAsm.FuncAddrs, PosToAddrCallback, params);
	return addr;
}

static const void* PosToAddrCallback(U64 key, const void* value, void* param)
{
	SAstFunc* func = (SAstFunc*)key;
	void** params = (void**)param;
	U64* addr = (U64*)params[0];
	const SPos* pos = (const SPos*)params[1];
	const SPos* func_pos = ((SAst*)func)->Pos;
	if (*addr == 0 && func_pos != NULL && wcscmp(func_pos->SrcName, pos->SrcName) == 0 && func_pos->Row <= pos->Row && pos->Row <= func->PosRowBottom)
		*addr = PosToAddrCallbackRecursion(func->Stats, pos);
	return value;
}

static U64 PosToAddrCallbackRecursion(const SList* stats, const SPos* target_pos)
{
	SListNode* ptr = stats->Top;
	while (ptr != NULL)
	{
		SAstStat* stat = (SAstStat*)ptr->Data;
		const SPos* stat_pos = ((SAst*)stat)->Pos;
		if (stat->AsmTop != NULL && stat_pos != NULL && wcscmp(stat_pos->SrcName, target_pos->SrcName) == 0 && stat_pos->Row <= target_pos->Row && target_pos->Row <= stat->PosRowBottom)
		{
			U64 result;
			SListNode* ptr2;
			switch (((SAst*)stat)->TypeId)
			{
				case AstTypeId_StatIf:
					{
						SAstStatIf* stat2 = (SAstStatIf*)stat;
						result = PosToAddrCallbackRecursion(stat2->StatBlock->Stats, target_pos);
						if (result != 0)
							return result;
						ptr2 = stat2->ElIfs->Top;
						while (ptr2 != NULL)
						{
							SAstStatElIf* elif = (SAstStatElIf*)ptr2->Data;
							result = PosToAddrCallbackRecursion(elif->StatBlock->Stats, target_pos);
							if (result != 0)
								return result;
							ptr2 = ptr2->Next;
						}
						if (stat2->ElseStatBlock != NULL)
						{
							result = PosToAddrCallbackRecursion(stat2->ElseStatBlock->Stats, target_pos);
							if (result != 0)
								return result;
						}
					}
					break;
				case AstTypeId_StatSwitch:
					{
						SAstStatSwitch* stat2 = (SAstStatSwitch*)stat;
						ptr2 = stat2->Cases->Top;
						while (ptr2 != NULL)
						{
							SAstStatCase* case_ = (SAstStatCase*)ptr2->Data;
							result = PosToAddrCallbackRecursion(case_->StatBlock->Stats, target_pos);
							if (result != 0)
								return result;
							ptr2 = ptr2->Next;
						}
						if (stat2->DefaultStatBlock != NULL)
						{
							result = PosToAddrCallbackRecursion(stat2->DefaultStatBlock->Stats, target_pos);
							if (result != 0)
								return result;
						}
					}
					break;
				case AstTypeId_StatWhile:
					{
						SAstStatWhile* stat2 = (SAstStatWhile*)stat;
						result = PosToAddrCallbackRecursion(stat2->Stats, target_pos);
						if (result != 0)
							return result;
					}
					break;
				case AstTypeId_StatFor:
					{
						SAstStatFor* stat2 = (SAstStatFor*)stat;
						result = PosToAddrCallbackRecursion(stat2->Stats, target_pos);
						if (result != 0)
							return result;
					}
					break;
				case AstTypeId_StatTry:
					{
						SAstStatTry* stat2 = (SAstStatTry*)stat;
						result = PosToAddrCallbackRecursion(stat2->StatBlock->Stats, target_pos);
						if (result != 0)
							return result;
						ptr2 = stat2->Catches->Top;
						while (ptr2 != NULL)
						{
							SAstStatCatch* catch_ = (SAstStatCatch*)ptr2->Data;
							result = PosToAddrCallbackRecursion(catch_->StatBlock->Stats, target_pos);
							if (result != 0)
								return result;
							ptr2 = ptr2->Next;
						}
						if (stat2->FinallyStatBlock != NULL)
						{
							result = PosToAddrCallbackRecursion(stat2->FinallyStatBlock->Stats, target_pos);
							if (result != 0)
								return result;
						}
					}
					break;
				case AstTypeId_StatBlock:
					{
						SAstStatBlock* stat2 = (SAstStatBlock*)stat;
						result = PosToAddrCallbackRecursion(stat2->Stats, target_pos);
						if (result != 0)
							return result;
					}
					break;
			}
			return (U64)*stat->AsmTop->Addr + DbgStartAddr;
		}
		ptr = ptr->Next;
	}
	return 0;
}

static SArchiveFileList* SearchFiles(int* len, const Char* src)
{
	SArchiveFileList* top = NULL;
	SArchiveFileList* bottom = NULL;
	*len = 0;
	if (!SearchFilesRecursion(len, wcslen(src), src, &top, &bottom))
		return (SArchiveFileList*)DummyPtr;
	return top;
}

static Bool SearchFilesRecursion(int* len, size_t src_base_len, const Char* src, SArchiveFileList** top, SArchiveFileList** bottom)
{
	Char src2[KUIN_MAX_PATH + 1];
	if (wcslen(src) > KUIN_MAX_PATH)
		return False;
	if (!PathFileExists(src))
		return False;
	wcscpy(src2, src);
	wcscat(src2, L"*");
	{
		WIN32_FIND_DATA find_data;
		HANDLE handle = FindFirstFile(src2, &find_data);
		if (handle == INVALID_HANDLE_VALUE)
			return False;
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;
			{
				wcscpy(src2, src);
				wcscat(src2, find_data.cFileName);
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					wcscat(src2, L"/");
					if (!SearchFilesRecursion(len, src_base_len, src2, top, bottom))
					{
						FindClose(handle);
						return False;
					}
				}
				else
				{
					SArchiveFileList* node = (SArchiveFileList*)Alloc(sizeof(SArchiveFileList));
					node->Next = NULL;
					node->Path = NewStr(NULL, L"%s", src2 + src_base_len);
					if ((*top) == NULL)
						(*top) = node;
					else
						(*bottom)->Next = node;
					(*bottom) = node;
					(*len)++;
				}
			}
		} while (FindNextFile(handle, &find_data));
		FindClose(handle);
	}
	return True;
}

static U8 GetKey(U64 key, U8 data, U64 pos)
{
	U64 rnd = ((pos ^ key) * 0x351cd819923acae7) >> 32;
	return (U8)(data ^ rnd);
}

static void MakeKeywordList(SDict* asts)
{
	SKeywordListCallbackParam param;
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
	param.ParentType = AstTypeId_Ast;
	DictForEach(asts, MakeKeywordListCallback, &param);

	KeywordList = (SKeywordListItem**)Alloc(sizeof(SKeywordListItem*) * (size_t)cnt);
	{
		int idx = 0;
		SKeywordList* ptr = *param.Top;
		while (ptr != NULL)
		{
			KeywordList[idx] = ptr->Keyword;
			idx++;
			ptr = ptr->Next;
		}
		ASSERT(idx == cnt);
	}
	KeywordListNum = cnt;
	qsort((void*)KeywordList, (size_t)KeywordListNum, sizeof(SKeywordListItem*), CmpKeywordListItem);
}

static const void* MakeKeywordListCallback(const Char* key, const void* value, void* param)
{
	SKeywordListCallbackParam* param2 = (SKeywordListCallbackParam*)param;
	const SAst* ast = (const SAst*)value;
	SKeywordListCallbackParam param3 = *param2;
	if (param3.Src == NULL)
		param3.Src = key;
	if (value != DummyPtr)
		MakeKeywordListRecursion(&param3, ast);
	return value;
}

static void MakeKeywordListRecursion(SKeywordListCallbackParam* param, const SAst* ast)
{
	if (ast->Pos != NULL && wcscmp(ast->Pos->SrcName, param->Src) == 0 && ast->Pos->Row != -1 &&
		(ast->Pos->SrcName[0] == L'\\' || param->ParentType == AstTypeId_Root || param->ParentType == AstTypeId_Enum || param->ParentType == AstTypeId_Class))
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
			SKeywordListItem* keyword = (SKeywordListItem*)Alloc(sizeof(SKeywordListItem));
			switch (param->ParentType)
			{
				case AstTypeId_Root:
					keyword->Name = NewStr(NULL, L"@%s", ast->Name);
					break;
				case AstTypeId_Enum:
					keyword->Name = NewStr(NULL, L"%%%s", ast->Name);
					break;
				case AstTypeId_Class:
					if (ast->TypeId == AstTypeId_Arg || ast->TypeId == AstTypeId_Func)
						keyword->Name = NewStr(NULL, L".%s", ast->Name);
					else
						keyword->Name = ast->Name;
					break;
				default:
					keyword->Name = ast->Name;
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
		SKeywordListCallbackParam param2 = *param;
		int* first = (int*)Alloc(sizeof(int));
		int* last = (int*)Alloc(sizeof(int));
		*first = INT_MAX;
		*last = INT_MIN;
		param2.First = first;
		param2.Last = last;
		param2.ParentType = ast->TypeId;
		DictForEach(ast->ScopeChildren, MakeKeywordListCallback, &param2);
	}
}

static int CmpKeywordListItem(const void* a, const void* b)
{
	const SKeywordListItem* a2 = *(const SKeywordListItem**)a;
	const SKeywordListItem* b2 = *(const SKeywordListItem**)b;
	int cmp;
	cmp = wcscmp(a2->Name, b2->Name);
	if (cmp != 0)
		return cmp;
	cmp = wcscmp(a2->Ast->Pos->SrcName, b2->Ast->Pos->SrcName);
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

static void FreeBreakPoints(void)
{
	if (BreakPointPoses != NULL)
	{
		S64 i;
		for (i = 0; i < BreakPointNum; i++)
		{
			if (BreakPointPoses[i].SrcName[0] != L'\0')
				FreeMem((void*)BreakPointPoses[i].SrcName);
		}
		FreeMem(BreakPointPoses);
	}
}

static void SetBreakPointOpes(HANDLE process_handle)
{
	if (BreakPointAddrs != NULL)
		FreeMem(BreakPointAddrs);
	BreakPointAddrNum = (int)BreakPointNum;
	BreakPointAddrs = (SBreakPointAddr*)AllocMem(sizeof(SBreakPointAddr) * (size_t)(BreakPointNum));
	S64 i;
	for (i = 0; i < BreakPointNum; i++)
	{
		U64 addr = PosToAddr(&BreakPointPoses[i]);
		if (addr == 0)
		{
			BreakPointAddrs[i].Addr = 0;
			BreakPointAddrs[i].Ope = 0;
			continue;
		}
		U8 old_code;
		U8 int3_code = 0xcc;
		ReadProcessMemory(process_handle, (LPVOID)addr, &old_code, 1, NULL);
		WriteProcessMemory(process_handle, (LPVOID)addr, &int3_code, 1, NULL);
		BreakPointAddrs[i].Addr = addr;
		BreakPointAddrs[i].Ope = old_code;
	}
	FlushInstructionCache(process_handle, NULL, 0);
}

static void UnsetBreakPointOpes(HANDLE process_handle)
{
	if (BreakPointAddrs == NULL)
		return;
	S64 i;
	for (i = BreakPointNum - 1; i >= 0; i--)
	{
		if (BreakPointAddrs[i].Addr == 0)
			continue;
		U8 new_code = BreakPointAddrs[i].Ope;
		WriteProcessMemory(process_handle, (LPVOID)BreakPointAddrs[i].Addr, &new_code, 1, NULL);
	}
	FlushInstructionCache(process_handle, NULL, 0);
}
