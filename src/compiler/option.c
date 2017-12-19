#include "option.h"

#include "log.h"
#include "mem.h"
#include "util.h"

void MakeOption(SOption* option, const Char* path, const Char* output, const Char* sys_dir, const Char* icon, Bool rls, const Char* env, Bool not_deploy)
{
	option->Rls = rls;
	option->IconFile = icon;
	option->SysDir = sys_dir;
	option->OutputFile = output;
	option->NotDeploy = not_deploy;

	ASSERT(path != NULL);

	if (option->OutputFile == NULL)
	{
		Char* path2 = GetDir(path, False, L"out.exe");
		option->OutputFile = path2;
	}
	else
	{
		size_t len = wcslen(output);
		Char* path2 = (Char*)Alloc(sizeof(Char) * (len + 1));
		size_t i;
		for (i = 0; i < len; i++)
			path2[i] = output[i] == L'\\' ? L'/' : output[i];
		path2[i] = L'\0';
		option->OutputFile = path2;
	}
	option->OutputDir = GetDir(option->OutputFile, False, NULL);

	if (env == NULL || wcscmp(env, L"wnd") == 0)
		option->Env = Env_Wnd;
	else if (wcscmp(env, L"cui") == 0)
		option->Env = Env_Cui;
	else if (wcscmp(env, L"web") == 0)
		option->Env = Env_Web;
	else
	{
		Err(L"EK0010", NULL, env);
		return;
	}

	if (option->IconFile == NULL)
	{
		size_t len = wcslen(sys_dir);
		Char* path2 = (Char*)Alloc(sizeof(Char) * (len + 11 + 1));
		wcscpy(path2, sys_dir);
		wcscat(path2, L"default.ico");
		option->IconFile = path2;
	}


	if (wcslen(path) > KUIN_MAX_PATH)
	{
		Err(L"EK0002", NULL, path);
		return;
	}

	// Get 'option->Dir' and 'option->SrcName'.
	{
		Char path2[KUIN_MAX_PATH + 1];
		Char* ptr = path2;
		Char* ptr2;
		wcscpy(path2, path);

		// Replace backslashes with slashes.
		while (*ptr != L'\0')
		{
			if (*ptr == L'\\')
				*ptr = L'/';
			ptr++;
		}

		// Check whether the extension is '.kn'.
		if (ptr - path2 < 4)
		{
			Err(L"EK0003", NULL, path);
			return;
		}
		if (ptr[-3] != L'.' || ptr[-2] != L'k' || ptr[-1] != L'n')
		{
			Err(L"EK0003", NULL, path);
			return;
		}
		ptr -= 4;
		ptr2 = ptr;

		// Move the pointer right after the last slash.
		while (*ptr2 != L'/' && ptr2 != path2)
			ptr2--;
		if (ptr2 != path2)
			ptr2++;

		// Divide the path into the directory name and the file name.
		{
			int len = (int)(ptr2 - path2);
			Char* buf = (Char*)Alloc(sizeof(Char) * (size_t)(len + 1));
			memcpy(buf, path2, sizeof(Char) * (size_t)(len));
			buf[len] = L'\0';
			option->SrcDir = buf;
		}
		{
			int len = (int)(ptr - ptr2 + 1);
			Char* buf = (Char*)Alloc(sizeof(Char) * (size_t)(len + 1));
			memcpy(buf, ptr2, sizeof(Char) * (size_t)(len));
			buf[len] = L'\0';
			option->SrcName = buf;
		}
	}
}
