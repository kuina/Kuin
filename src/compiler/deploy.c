#include "deploy.h"

#include "log.h"
#include "util.h"

static const void* CopyDlls(const Char* key, const void* value, void* param);
static void CopyKuinFile(const Char* name, const SOption* option);

void Deploy(U64 app_code, const SOption* option, SDict* dlls)
{
#if defined(_DEBUG)
	// When doing tests, the program uses debugging Dlls so do not copy these.
	UNUSED(app_code);
	UNUSED(option);
	UNUSED(dlls);
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
#endif
}

static const void* CopyDlls(const Char* key, const void* value, void* param)
{
	const SOption* option = (SOption*)param;
	{
		Char src[KUIN_MAX_PATH + 1];
		Char dst[KUIN_MAX_PATH + 1];
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
	// Copy license files.
	if (wcscmp(key, L"d1000.knd") == 0)
		CopyKuinFile(L"license_ogg_vorbis.txt", option);
	return value;
}

static void CopyKuinFile(const Char* name, const SOption* option)
{
	Char src[KUIN_MAX_PATH + 1];
	Char dst[KUIN_MAX_PATH + 1];
	wcscpy(src, option->SysDir);
	wcscat(src, name);
	wcscpy(dst, option->OutputDir);
	wcscat(dst, L"data/");
	wcscat(dst, name);
	if (CopyFile(src, dst, FALSE) == 0)
		Err(L"EK0013", NULL, src, dst);
}
