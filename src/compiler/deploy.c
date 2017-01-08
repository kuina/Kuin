#include "deploy.h"

#include "log.h"
#include "util.h"

static void CopyDll(const Char* name, const SOption* option);

void Deploy(U64 app_code, const SOption* option)
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
	CopyDll(L"d0000.knd", option);
	switch (option->Env)
	{
		case Env_Wnd:
			CopyDll(L"d0001.knd", option);
			break;
		case Env_Cui:
			CopyDll(L"d0002.knd", option);
			break;
		case Env_Web:
			break;
		default:
			ASSERT(False);
			break;
	}
	// TODO: Deploy the resource folder.
#endif
}

static void CopyDll(const Char* name, const SOption* option)
{
	Char src[1024];
	Char dst[1024];
	wcscpy(src, option->SysDir);
	if (option->Rls)
		wcscat(src, L"rls/");
	else
		wcscat(src, L"dbg/");
	wcscat(src, name);
	wcscpy(dst, option->OutputDir);
	wcscat(dst, L"data/");
	wcscat(dst, name);
	if (CopyFile(src, dst, FALSE) == 0)
		Err(L"EK0013", NULL, src, dst);
}
