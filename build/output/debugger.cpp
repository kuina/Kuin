#include "kuin_interpreter.h"

#include <DbgHelp.h> // 'StackWalk64'

#pragma comment(lib, "DbgHelp.lib")

void setDbgStartAddr(uint64_t);
void setBreakPointAddrs(Array_<uint64_t>*, Array_<uint8_t>*);
void getBreakPointAddrs(Array_<uint64_t>**, Array_<uint8_t>**);
int64_t getBreakPointPosesNum();
uint64_t posToAddr(int64_t);
bool addrToPos(Array_<char16_t>**, Array_<char16_t>**, int64_t*, int64_t*, uint64_t);
Array_<char16_t>* getExcptMsg(int64_t);
int64_t initDbgVars();
void getDbgVars(int64_t, Array_<char16_t>*, int64_t, int64_t, uint64_t, uint64_t(*)(int64_t, uint64_t), void(*)(Array_<char16_t>*, Array_<char16_t>*, int64_t), int64_t);

static void SetBreakPointOpes(HANDLE process_handle);
static void UnsetBreakPointOpes(HANDLE process_handle);
static uint64_t CallReadProcessMemory(int64_t process_handle, uint64_t addr);
static void CallCallbackForGetDbgVars(Array_<char16_t>* data1, Array_<char16_t>* data2, int64_t callback);

bool RunDbgImpl(const uint8_t* path, const uint8_t* cmd_line, void* idle_func, void* event_func, void* break_points_func, void* break_func, void* dbg_func)
{
	const wchar_t* path2 = (const wchar_t*)(path + 0x10);
	wchar_t cur_dir[512 + 1];
	wchar_t* cmd_line_buf = nullptr;
	PROCESS_INFORMATION process_info;
	{
		wchar_t* ptr;
		memcpy(cur_dir, path2, sizeof(wchar_t) * static_cast<size_t>(*reinterpret_cast<const int64_t*>(path + 0x08) + 1));
		ptr = cur_dir + wcslen(cur_dir);
		while (ptr != cur_dir && *ptr != L'/' && *ptr != L'\\')
			ptr--;
		if (ptr != nullptr)
			*(ptr + 1) = L'\0';
	}
	if (cmd_line != nullptr)
	{
		size_t len = wcslen((const wchar_t*)(cmd_line + 0x10));
		cmd_line_buf = (wchar_t*)malloc(sizeof(wchar_t) * (len + 1));
		memcpy(cmd_line_buf, (const wchar_t*)(cmd_line + 0x10), sizeof(wchar_t) * static_cast<size_t>(*reinterpret_cast<const int64_t*>(cmd_line + 0x08) + 1));
	}
	{
		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(STARTUPINFO);
		if (!CreateProcess(path2, cmd_line_buf, nullptr, nullptr, FALSE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS | DEBUG_ONLY_THIS_PROCESS, nullptr, cur_dir, &startup_info, &process_info))
		{
			if (cmd_line_buf != nullptr)
				free(cmd_line_buf);
			return false;
		}
	}
	if (cmd_line_buf != nullptr)
		free(cmd_line_buf);

	int64_t interpret2_data = initDbgVars();
	{
		DEBUG_EVENT debug_event = { 0 };
		bool end = false;
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
					setDbgStartAddr(reinterpret_cast<uint64_t>(debug_event.u.CreateProcessInfo.lpBaseOfImage));
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
					end = true;
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
							Array_<uint64_t>* break_point_addrs;
							Array_<uint8_t>* break_point_opes;
							getBreakPointAddrs(&break_point_addrs, &break_point_opes);
							if (break_point_addrs != nullptr)
							{
								for (int64_t i = 0; i < break_point_addrs->L; i++)
								{
									if (break_point_addrs->B[i] != 0 && break_point_addrs->B[i] == reinterpret_cast<uint64_t>(debug_event.u.Exception.ExceptionRecord.ExceptionAddress))
									{
										break_point_idx = i;
										break;
									}
								}
							}
						}
						if (break_point_idx == -1 && (excpt_code & 0xffff0000) != 0xc0000000 && (excpt_code & 0xffff0000) != 0xe9170000)
							break;
						Call3Asm((void*)0, nullptr, nullptr, dbg_func);
						{
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
							SymInitialize(process_info.hProcess, nullptr, TRUE);

							bool excpt_pos_found = false;
							Array_<char16_t>* excpt_pos_src = nullptr;
							int64_t excpt_pos_row = -1;
							int64_t excpt_pos_col = -1;
							for (; ; )
							{
								if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process_info.hProcess, process_info.hThread, &stack, &context2, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
									break;
								symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
								symbol.MaxNameLength = 255;
								DWORD64 displacement;
								if (SymGetSymFromAddr64(process_info.hProcess, (DWORD64)stack.AddrPC.Offset, &displacement, &symbol))
								{
									// char name[1024];
									// UnDecorateSymbolName(symbol.Name, (PSTR)name, 1024, UNDNAME_COMPLETE);
								}
								else
								{
									Array_<char16_t>* name;
									Array_<char16_t>* src;
									int64_t row;
									int64_t col;
									bool found = addrToPos(&name, &src, &row, &col, static_cast<uint64_t>(context2.Rip));
									if (!excpt_pos_found)
									{
										if (wcschr(reinterpret_cast<wchar_t*>(name->B), L'@') == nullptr)
											break;
										if (found)
										{
											excpt_pos_found = true;
											excpt_pos_src = src;
											excpt_pos_row = row;
											excpt_pos_col = col;
										}
									}
									if (found)
									{
										wchar_t buf[0x08 + 1024];
										swprintf(buf + 0x08, 1024, L"%s (%s: %I64d, %I64d)", reinterpret_cast<wchar_t*>(name->B), reinterpret_cast<wchar_t*>(src->B), row, col);
										((int64_t*)buf)[0] = 2;
										((int64_t*)buf)[1] = (int64_t)wcslen(buf + 0x08);
										Call3Asm((void*)2, buf, nullptr, dbg_func);
									}
								}
								if (stack.AddrPC.Offset == 0)
									break;
							}
							if (excpt_pos_found)
							{
								getDbgVars(interpret2_data, excpt_pos_src, excpt_pos_row, reinterpret_cast<int64_t>(process_info.hProcess), static_cast<uint64_t>(context.Rsp), CallReadProcessMemory, CallCallbackForGetDbgVars, reinterpret_cast<int64_t>(dbg_func));
								{
									void* pos_ptr = nullptr;
									wchar_t pos_name[0x08 + 256];
									uint8_t pos_buf[0x28];
									{
										size_t pos_name_len = excpt_pos_src->L;
										((int64_t*)pos_name)[0] = 1;
										((int64_t*)pos_name)[1] = (int64_t)pos_name_len;
										memcpy((uint8_t*)pos_name + 0x10, excpt_pos_src->B, sizeof(wchar_t) * (pos_name_len + 1));

										((int64_t*)pos_buf)[0] = 2;
										((int64_t*)pos_buf)[1] = 0;
										((void**)pos_buf)[2] = pos_name;
										((int64_t*)pos_buf)[3] = excpt_pos_row;
										((int64_t*)pos_buf)[4] = excpt_pos_col;
										pos_ptr = pos_buf;
									}
									auto* excpt_msg = getExcptMsg(static_cast<int64_t>(excpt_code));
									uint8_t* buf = newPrimArray_(0x10 + sizeof(char16_t) * static_cast<size_t>(excpt_msg->L + 1), uint8_t);
									reinterpret_cast<int64_t*>(buf)[0] = 2;
									reinterpret_cast<int64_t*>(buf)[1] = excpt_msg->L;
									memcpy(buf + 0x10, excpt_msg->B, sizeof(char16_t) * static_cast<size_t>(excpt_msg->L + 1));
									Call3Asm((void*)(uint64_t)excpt_code, pos_ptr, buf, break_func);
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
						void* buf = nullptr;
						if (debug_event.u.DebugString.fUnicode == 0)
						{
							char* buf2 = (char*)malloc((size_t)debug_event.u.DebugString.nDebugStringLength);
							SIZE_T size = 0;
							if (!ReadProcessMemory(process_info.hProcess, debug_event.u.DebugString.lpDebugStringData, buf2, debug_event.u.DebugString.nDebugStringLength, &size) || size == 0)
							{
								free(buf2);
								break;
							}
							int size2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf2, (int)size, nullptr, 0);
							buf = malloc(0x10 + sizeof(wchar_t) * (size_t)size2);
							MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf2, (int)size, (wchar_t*)((uint8_t*)buf + 0x10), size2);
							((int64_t*)buf)[1] = (int64_t)size2 - 1;
							free(buf2);
						}
						else
						{
							buf = malloc(0x10 + sizeof(wchar_t) * (size_t)debug_event.u.DebugString.nDebugStringLength);
							SIZE_T size = 0;
							if (!ReadProcessMemory(process_info.hProcess, debug_event.u.DebugString.lpDebugStringData, (wchar_t*)((uint8_t*)buf + 0x10), debug_event.u.DebugString.nDebugStringLength, &size) || size == 0)
							{
								free(buf);
								break;
							}
							((int64_t*)buf)[1] = (int64_t)debug_event.u.DebugString.nDebugStringLength - 1;
						}
						*((int64_t*)buf) = 2;
						if (((int64_t*)buf)[1] >= 4)
						{
							const wchar_t* ptr = (const wchar_t*)((uint8_t*)buf + 0x10);
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

	if (process_info.hThread != nullptr)
		CloseHandle(process_info.hThread);
	if (process_info.hProcess != nullptr)
		CloseHandle(process_info.hProcess);
	return true;
}

static void SetBreakPointOpes(HANDLE process_handle)
{
	int64_t break_point_num = getBreakPointPosesNum();
	auto* break_point_addrs = new_(Array_<uint64_t>)();
	break_point_addrs->L = break_point_num;
	break_point_addrs->B = newPrimArray_(static_cast<size_t>(break_point_num), uint64_t);
	auto* break_point_opes = new_(Array_<uint8_t>)();
	break_point_opes->L = break_point_num;
	break_point_opes->B = newPrimArray_(static_cast<size_t>(break_point_num), uint8_t);
	const uint8_t int3_code = 0xcc;
	for (int64_t i = 0; i < break_point_num; i++)
	{
		uint64_t addr = posToAddr(i);
		if (addr == 0)
		{
			break_point_addrs->B[i] = 0;
			break_point_opes->B[i] = 0;
			continue;
		}
		uint8_t old_code;
		ReadProcessMemory(process_handle, reinterpret_cast<LPVOID>(addr), &old_code, 1, nullptr);
		WriteProcessMemory(process_handle, reinterpret_cast<LPVOID>(addr), &int3_code, 1, nullptr);
		break_point_addrs->B[i] = addr;
		break_point_opes->B[i] = old_code;
	}
	setBreakPointAddrs(break_point_addrs, break_point_opes);
	FlushInstructionCache(process_handle, nullptr, 0);
}

static void UnsetBreakPointOpes(HANDLE process_handle)
{
	Array_<uint64_t>* break_point_addrs;
	Array_<uint8_t>* break_point_opes;
	getBreakPointAddrs(&break_point_addrs, &break_point_opes);
	if (break_point_addrs == nullptr)
		return;
	for (int64_t i = break_point_addrs->L - 1; i >= 0; i--)
	{
		if (break_point_addrs->B[i] == 0)
			continue;
		uint8_t new_code = break_point_opes->B[i];
		WriteProcessMemory(process_handle, reinterpret_cast<LPVOID>(break_point_addrs->B[i]), &new_code, 1, nullptr);
	}
	FlushInstructionCache(process_handle, nullptr, 0);
}

static uint64_t CallReadProcessMemory(int64_t process_handle, uint64_t addr)
{
	uint64_t value;
	if (ReadProcessMemory(reinterpret_cast<HANDLE>(process_handle), reinterpret_cast<LPCVOID>(addr), &value, sizeof(value), nullptr))
		return value;
	return 0;
}

static void CallCallbackForGetDbgVars(Array_<char16_t>* data1, Array_<char16_t>* data2, int64_t callback)
{
	wchar_t buf1[0x08 + 256];
	wchar_t buf2[0x08 + 1024];
	reinterpret_cast<int64_t*>(buf1)[0] = 2;
	reinterpret_cast<int64_t*>(buf1)[1] = data1->L;
	memcpy(buf1 + 0x08, data1->B, sizeof(wchar_t) * static_cast<size_t>(data1->L + 1));
	reinterpret_cast<int64_t*>(buf2)[0] = 2;
	reinterpret_cast<int64_t*>(buf2)[1] = data2->L;
	memcpy(buf2 + 0x08, data2->B, sizeof(wchar_t) * static_cast<size_t>(data2->L + 1));
	Call3Asm(reinterpret_cast<void*>(1), buf1, buf2, reinterpret_cast<void*>(callback));
}
