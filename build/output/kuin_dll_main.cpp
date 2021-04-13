#define _CRT_SECURE_NO_WARNINGS
#define STRICT
#define _WIN32_DCOM
#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCKAPI_
#include <Windows.h>

#include "kuin_interpreter.h"

#define UNUSED(var) (void)(var)
#define EXPORT_CPP extern "C" _declspec(dllexport)

void initLib();
void finLib();
bool build();
void getVersion(int64_t*, int64_t*, int64_t*);
void setLogFunc(void(*)(int64_t, Array_<char16_t>*, Array_<char16_t>*, int64_t, int64_t));
bool acquireOption(Array_<Array_<char16_t>*>*, bool);
void setFileFuncs(int64_t(*)(Array_<char16_t>*), void(*)(int64_t), int64_t(*)(int64_t), char16_t(*)(int64_t));
void setBreakPointPoses(Array_<Array_<char16_t>*>*, Array_<int64_t>*, Array_<int64_t>*);

int64_t interpret2(Array_<char16_t>*);
Array_<char16_t>* getKeywordsRoot(int64_t, Array_<char16_t>*, Array_<char16_t>*, int64_t, int64_t, void(*)(int64_t, Array_<char16_t>*), int64_t);

struct Interpret2Arg
{
	void(*FuncComplete)();
	Array_<char16_t>* PrioritizedCode;
};

static const void* (*FuncGetSrc)(const uint8_t*);
static void(*FuncLog)(const void*, int64_t, int64_t);
static void* Src = nullptr;
static const void* SrcLine = nullptr;
static const wchar_t* SrcChar = nullptr;
static CRITICAL_SECTION CriticalSection;
static HANDLE Interpret2ThreadHandle = nullptr;
static int ReadingLetterCnt = 0;
static int64_t Interpret2Data = 0;

static void SetOption(const uint8_t* option);
static void OutputLog(int64_t code, Array_<char16_t>* msg, Array_<char16_t>* src, int64_t row, int64_t col);
static void DecSrc();
static int64_t FileOpen(Array_<char16_t>* path);
static void FileClose(int64_t handle);
static int64_t FileSize(int64_t handle);
static char16_t FileReadLetter(int64_t handle);
static void CallCallbackForGetKeywords(int64_t callback, Array_<char16_t>* keyword);
static DWORD WINAPI RunInterpret2(LPVOID data);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void InitCompiler()
{
	InitializeCriticalSection(&CriticalSection);
	EnterCriticalSection(&CriticalSection);
	initLib();
}

EXPORT_CPP void FinCompiler()
{
	finLib();
	LeaveCriticalSection(&CriticalSection);
	DeleteCriticalSection(&CriticalSection);
}

EXPORT_CPP bool BuildMem(const uint8_t* option, const void* (*func_get_src)(const uint8_t*), void(*func_log)(const void* args, int64_t row, int64_t col))
{
	FuncGetSrc = func_get_src;
	FuncLog = func_log;
	setLogFunc(OutputLog);
	SetOption(option);
	setFileFuncs(FileOpen, FileClose, FileSize, FileReadLetter);
	bool result = build();
	FuncGetSrc = nullptr;
	FuncLog = nullptr;
	DecSrc();
	Src = nullptr;
	SrcLine = nullptr;
	SrcChar = nullptr;
	return result;
}

EXPORT_CPP void Interpret1(void* src, int64_t line, void* me, void* replace_func, int64_t cursor_x, int64_t cursor_y, int64_t* new_cursor_x, int64_t old_line, int64_t new_line)
{
	for (; ; )
	{
		void* str = *(void**)((uint8_t*)src + 0x10);
		void* color = *(void**)((uint8_t*)src + 0x18);
		void* comment_level = *(void**)((uint8_t*)src + 0x20);
		void* flags = *(void**)((uint8_t*)src + 0x28);
		if (InterpretImpl1(str, color, comment_level, flags, line, me, replace_func, cursor_x, cursor_y, new_cursor_x, old_line, new_line))
			break;
		old_line = -1;
	}
}

EXPORT_CPP void Interpret2(const uint8_t* option, const void* (*func_get_src)(const uint8_t*), void(*func_log)(const void* args, int64_t row, int64_t col), void(*func_complete)(), const uint8_t* prioritized_code)
{
	FuncGetSrc = func_get_src;
	FuncLog = func_log;
	setLogFunc(OutputLog);
	SetOption(option);
	setFileFuncs(FileOpen, FileClose, FileSize, FileReadLetter);
	Interpret2Arg* arg = reinterpret_cast<Interpret2Arg*>(newPrimArray_(sizeof(Interpret2Arg), uint8_t));
	arg->FuncComplete = func_complete;
	if (prioritized_code == nullptr)
		arg->PrioritizedCode = nullptr;
	else
	{
		arg->PrioritizedCode = new_(Array_<char16_t>)();
		arg->PrioritizedCode->L = *reinterpret_cast<const int64_t*>(prioritized_code + 0x08);
		arg->PrioritizedCode->B = newPrimArray_(arg->PrioritizedCode->L + 1, char16_t);
		memcpy(arg->PrioritizedCode->B, prioritized_code + 0x10, sizeof(char16_t) * static_cast<size_t>(arg->PrioritizedCode->L + 1));
	}
	DWORD id;
	Interpret2ThreadHandle = CreateThread(nullptr, 0, RunInterpret2, arg, 0, &id);
}

EXPORT_CPP void Version(int64_t* major, int64_t* minor, int64_t* micro)
{
	getVersion(major, minor, micro);
}

EXPORT_CPP void ResetMemAllocator()
{
	finLib();
	initLib();
}

EXPORT_CPP void* GetKeywords(void* src, const uint8_t* src_name, int64_t x, int64_t y, void* callback)
{
	void* str = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(src) + 0x10);
	void** str2 = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(str) + 0x10 + 0x08 * y);
	uint8_t* str3 = reinterpret_cast<uint8_t*>(*str2);
	auto* str4 = new_(Array_<char16_t>)();
	str4->L = *reinterpret_cast<int64_t*>(str3 + 0x08);
	str4->B = newPrimArray_(static_cast<size_t>(str4->L + 1), char16_t);
	memcpy(str4->B, str3 + 0x10, sizeof(char16_t) * static_cast<size_t>(str4->L + 1));

	auto* src_name2 = new_(Array_<char16_t>)();
	src_name2->L = *reinterpret_cast<const int64_t*>(src_name + 0x08);
	src_name2->B = newPrimArray_(static_cast<size_t>(src_name2->L + 1), char16_t);
	memcpy(src_name2->B, src_name + 0x10, sizeof(char16_t) * static_cast<size_t>(src_name2->L + 1));

	auto* hint = getKeywordsRoot(Interpret2Data, str4, src_name2, x, y, CallCallbackForGetKeywords, reinterpret_cast<int64_t>(callback));
	if (hint == nullptr)
		return nullptr;
	auto* result = newPrimArray_(0x10 + sizeof(wchar_t) * static_cast<size_t>(hint->L + 1), uint8_t);
	reinterpret_cast<int64_t*>(result)[0] = 1;
	reinterpret_cast<int64_t*>(result)[1] = hint->L;
	memcpy(result + 0x10, hint->B, sizeof(wchar_t) * static_cast<size_t>(hint->L + 1));
	return result;
}

EXPORT_CPP bool RunDbg(const uint8_t* path, const uint8_t* cmd_line, void* idle_func, void* event_func, void* break_points_func, void* break_func, void* dbg_func)
{
	return RunDbgImpl(path, cmd_line, idle_func, event_func, break_points_func, break_func, dbg_func);
}

EXPORT_CPP void SetBreakPoints(const void* break_points)
{
	int64_t len = ((int64_t*)break_points)[1];
	auto* poses_src_names = new_(Array_<Array_<char16_t>*>)();
	poses_src_names->L = len;
	poses_src_names->B = newPrimArray_(len, Array_<char16_t>*);
	auto* poses_rows = new_(Array_<int64_t>)();
	poses_rows->L = len;
	poses_rows->B = newPrimArray_(len, int64_t);
	auto* poses_cols = new_(Array_<int64_t>)();
	poses_cols->L = len;
	poses_cols->B = newPrimArray_(len, int64_t);

	void* const* ptr = reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(break_points) + 0x10);
	for (int64_t i = 0; i < len; i++)
	{
		wchar_t* src_name = reinterpret_cast<wchar_t*>(reinterpret_cast<uint8_t*>(*reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr[i]) + 0x10)) + 0x10);
		int64_t row = *reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(ptr[i]) + 0x18);
		int64_t col = *reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(ptr[i]) + 0x20);
		int64_t name_len = (reinterpret_cast<int64_t*>(*reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr[i]) + 0x10)))[1];
		poses_src_names->B[i] = new_(Array_<char16_t>)();
		poses_src_names->B[i]->L = name_len;
		poses_src_names->B[i]->B = newPrimArray_(name_len + 1, char16_t);
		memcpy(poses_src_names->B[i]->B, src_name, sizeof(char16_t) * static_cast<size_t>(name_len + 1));
		poses_rows->B[i] = row;
		poses_cols->B[i] = col;
	}

	setBreakPointPoses(poses_src_names, poses_rows, poses_cols);

	for (int64_t i = 0; i < len; i++)
		*reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(ptr[i]) + 0x18) = poses_rows->B[i];
}

EXPORT_CPP void LockThread()
{
	LeaveCriticalSection(&CriticalSection);
	Sleep(1);
	EnterCriticalSection(&CriticalSection);
}

EXPORT_CPP bool Interpret2Running()
{
	return Interpret2ThreadHandle != nullptr;
}

EXPORT_CPP void WaitEndOfInterpret2()
{
	while (Interpret2ThreadHandle != nullptr)
	{
		LeaveCriticalSection(&CriticalSection);
		Sleep(1);
		EnterCriticalSection(&CriticalSection);
	}
}

static void SetOption(const uint8_t* option)
{
	int64_t len = *reinterpret_cast<const int64_t*>(option + 0x08);
	auto* args = new_(Array_<Array_<char16_t>*>)();
	args->L = len;
	args->B = newPrimArray_(len, Array_<char16_t>*);
	for (int64_t i = 0; i < len; i++)
	{
		const uint8_t* item = static_cast<const uint8_t*>(*reinterpret_cast<void* const*>(option + 0x10 + i * 0x08));
		int64_t len2 = *reinterpret_cast<const int64_t*>(item + 0x08);
		args->B[i] = new_(Array_<char16_t>)();
		args->B[i]->L = len2;
		args->B[i]->B = newPrimArray_(len2 + 1, char16_t);
		memcpy(args->B[i]->B, item + 0x10, sizeof(char16_t) * static_cast<size_t>(len2 + 1));
	}
	acquireOption(args, false);
}

static void OutputLog(int64_t code, Array_<char16_t>* msg, Array_<char16_t>* src, int64_t row, int64_t col)
{
	uint8_t args[0x10 + 0x18];
	*reinterpret_cast<int64_t*>(args + 0x00) = 2;
	*reinterpret_cast<int64_t*>(args + 0x08) = 3;
	uint8_t code2[0x10 + sizeof(wchar_t) * 11];
	{
		*reinterpret_cast<int64_t*>(code2 + 0x00) = 1;
		*reinterpret_cast<int64_t*>(code2 + 0x08) = 10;
		swprintf(reinterpret_cast<wchar_t*>(code2 + 0x10), 11, L"0x%08X", static_cast<uint32_t>(code));
		*reinterpret_cast<void**>(args + 0x10) = code2;
	}
	uint8_t* msg2 = newPrimArray_(0x10 + sizeof(wchar_t) * static_cast<size_t>(msg->L + 1), uint8_t);
	{
		*reinterpret_cast<int64_t*>(msg2 + 0x00) = 1;
		*reinterpret_cast<int64_t*>(msg2 + 0x08) = msg->L;
		memcpy(msg2 + 0x10, msg->B, sizeof(wchar_t) * static_cast<size_t>(msg->L + 1));
		*reinterpret_cast<void**>(args + 0x18) = msg2;
	}
	uint8_t src2[0x10 + sizeof(wchar_t) * 260];
	if (src == nullptr)
		*reinterpret_cast<void**>(args + 0x20) = nullptr;
	else
	{
		*reinterpret_cast<int64_t*>(src2 + 0x00) = 1;
		*reinterpret_cast<int64_t*>(src2 + 0x08) = src->L;
		memcpy(src2 + 0x10, src->B, sizeof(wchar_t) * static_cast<size_t>(src->L + 1));
		*reinterpret_cast<void**>(args + 0x20) = src2;
	}
	Call3Asm(args, reinterpret_cast<void*>(row), reinterpret_cast<void*>(col), FuncLog);
}

static void DecSrc()
{
	// Decrement 'Src', but do not release it here. It will be released in '.kn'.
	if (Src != nullptr)
		(*reinterpret_cast<int64_t*>(Src))--;
}

static int64_t FileOpen(Array_<char16_t>* path)
{
	if (Src != nullptr)
		return 0;
	uint8_t path2[0x10 + sizeof(wchar_t) * (512 + 1)];
	*reinterpret_cast<int64_t*>(path2 + 0x00) = 2;
	*reinterpret_cast<int64_t*>(path2 + 0x08) = path->L;
	memcpy(path2 + 0x10, path->B, sizeof(wchar_t) * static_cast<size_t>(path->L + 1));
	DecSrc();
	Src = Call1Asm(path2, reinterpret_cast<void*>(reinterpret_cast<uint64_t>(FuncGetSrc)));
	if (Src == nullptr)
		return 0;
	SrcLine = reinterpret_cast<uint8_t*>(Src) + 0x10;
	SrcChar = reinterpret_cast<wchar_t*>(reinterpret_cast<uint8_t*>(*reinterpret_cast<void* const*>(SrcLine)) + 0x10);
	return 1; // An unused handle which is not zero.
}

static void FileClose(int64_t handle)
{
	UNUSED(handle);
	DecSrc();
	Src = nullptr;
	SrcLine = nullptr;
	SrcChar = nullptr;
}

static int64_t FileSize(int64_t handle)
{
	UNUSED(handle);
	size_t total = 0;
	int64_t len = *reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(Src) + 0x08);
	void* ptr = reinterpret_cast<uint8_t*>(Src) + 0x10;
	int64_t i;
	for (i = 0; i < len; i++)
	{
		total += *reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(*reinterpret_cast<void**>(ptr)) + 0x08);
		if (total >= 2)
			return 2; // A value of 2 or more is not distinguished.
		ptr = reinterpret_cast<uint8_t*>(ptr) + 0x08;
	}
	return total;
}

static char16_t FileReadLetter(int64_t handle)
{
	ReadingLetterCnt++;
	if (ReadingLetterCnt == 10000)
	{
		ReadingLetterCnt = 0;
		LeaveCriticalSection(&CriticalSection);
		Sleep(1);
		EnterCriticalSection(&CriticalSection);
	}
	UNUSED(handle);
	const void* term;
	{
		int64_t len = *reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(Src) + 0x08);
		term = reinterpret_cast<uint8_t*>(Src) + 0x10 + len * 0x08;
	}
	if (SrcLine == term)
		return L'\0';
	wchar_t c = *SrcChar;
	if (c != L'\0')
	{
		SrcChar++;
		return c;
	}
	SrcLine = reinterpret_cast<const uint8_t*>(SrcLine) + 0x08;
	if (SrcLine == term)
		return L'\0';
	SrcChar = reinterpret_cast<wchar_t*>(reinterpret_cast<uint8_t*>(*reinterpret_cast<void* const*>(SrcLine)) + 0x10);
	return L'\n';
}

static void CallCallbackForGetKeywords(int64_t callback, Array_<char16_t>* keyword)
{
	wchar_t buf[256];
	int64_t len = keyword->L;
	reinterpret_cast<int64_t*>(buf)[0] = 2;
	reinterpret_cast<int64_t*>(buf)[1] = len;
	memcpy(buf + 0x08, keyword->B, sizeof(wchar_t) * static_cast<size_t>(len + 1));
	Call1Asm(buf, reinterpret_cast<void*>(callback));
}

static DWORD WINAPI RunInterpret2(LPVOID data)
{
	Interpret2Arg* arg = static_cast<Interpret2Arg*>(data);
	EnterCriticalSection(&CriticalSection);
	ReadingLetterCnt = 0;
	try
	{
		Interpret2Data = interpret2(arg->PrioritizedCode);
	}
	catch (...)
	{
	}
	FuncGetSrc = nullptr;
	FuncLog = nullptr;
	DecSrc();
	Src = nullptr;
	SrcLine = nullptr;
	SrcChar = nullptr;
	Interpret2ThreadHandle = nullptr;
	arg->FuncComplete();
	LeaveCriticalSection(&CriticalSection);
	return TRUE;
}
