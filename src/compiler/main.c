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

static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void*(*allocator)(size_t size), void(*log_func)(const Char* code, const Char* msg, const Char* src, int row, int col));
static size_t GetSize(FILE* file_ptr);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void*(*allocator)(size_t size), void(*log_func)(const Char* code, const Char* msg, const Char* src, int row, int col))
{
	return Build(_wfopen, fclose, fgetwc, GetSize, path, sys_dir, output, icon, rls, env, allocator, log_func);
}

EXPORT void Version(int* major, int* minor, int* micro)
{
	*major = 9;
	*minor = 17;
	*micro = 0;
}

static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void*(*allocator)(size_t size), void(*log_func)(const Char* code, const Char* msg, const Char* src, int row, int col))
{
	SOption option;
	SDict* asts;
	SAstFunc* entry;
	SDict* dlls;
	SPackAsm pack_asm;
	U32 begin_time;

	SetAllocator(allocator);

	// Set the system directory.
	if (sys_dir == NULL)
	{
		Char sys_dir2[1024 + 1];
		GetModuleFileName(NULL, sys_dir2, 1024 + 1);
		sys_dir = GetDir(sys_dir2, False, L"sys/");
	}
	else
		sys_dir = GetDir(sys_dir, True, NULL);

	SetLogFunc(log_func, 0, sys_dir);
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
	Assemble(&pack_asm, entry, &option, dlls);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0003", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	ToMachineCode(&pack_asm, &option);
	if (ErrOccurred())
		goto ERR;
	Err(L"IK0004", NULL, (double)(timeGetTime() - begin_time) / 1000.0);
	Deploy(pack_asm.AppCode, &option, dlls);
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

static size_t GetSize(FILE* file_ptr)
{
	int file_size;
	fseek(file_ptr, 0, SEEK_END);
	file_size = (int)ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);
	return (size_t)file_size;
}
