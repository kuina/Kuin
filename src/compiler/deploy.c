#include "deploy.h"

#include "log.h"
#include "util.h"

static void CopyDlls(const Char* key, const void* value, void* param);

void Deploy(U64 app_code, const SOption* option, SDict* dlls)
{
#if defined(_DEBUG)
	// When doing tests, the program uses debugging Dlls so do not copy these.
	UNUSED(app_code);
	UNUSED(option);
#else
	{
		Char path[1024];
		wcscpy(path, option->OutputDir);
		wcscat(path, L"data/");
		if (!DelDir(path))
			Err(L"EK0011", NULL, path);
		if (CreateDirectory(path, NULL) == 0)
			Err(L"EK0012", NULL, path);
	}
	DictForEach(dlls, CopyDlls, (void*)option);
	// TODO: Deploy the resource folder.
#endif
}

static void CopyDlls(const Char* key, const void* value, void* param)
{
	const SOption* option = param;
	Char src[1024];
	Char dst[1024];
	wcscpy(src, option->SysDir);
	if (option->Rls)
		wcscat(src, L"rls/");
	else
		wcscat(src, L"dbg/");
	wcscat(src, key);
	wcscpy(dst, option->OutputDir);
	wcscat(dst, L"data/");
	wcscat(dst, key);
	if (CopyFile(src, dst, FALSE) == 0)
		Err(L"EK0013", NULL, src, dst);
}
