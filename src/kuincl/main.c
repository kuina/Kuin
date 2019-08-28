// kuincl.exe
//
// (C)Kuina-chan
//

#include "main.h"

#include <fcntl.h>
#include <io.h>

// 0 = 'Ja', 1 = 'En'.
#define LANG (0)

typedef Bool(*TypeOfBuild)(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, const void* related_files, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), S64 lang, S64 app_code, Bool not_deploy);
typedef void(*TypeOfVersion)(int* major, int* minor, int* micro);
typedef void(*TypeOfInitCompiler)(S64 mem_num, S64 lang);
typedef void(*TypeOfFinCompiler)(void);
typedef Bool(*TypeOfArchive)(const U8* dst, const U8* src, S64 app_code);

static Bool Quiet;

static void Log(const Char* code, const Char* msg, const Char* src, int row, int col);
static U8* GetDir(const Char* path, Bool dir, const Char* add_name);

int wmain(int argc, Char** argv)
{
	const Char* input = NULL;
	const Char* output = NULL;
	const Char* sys_dir = NULL;
	const Char* icon = NULL;
	const void* related_files = NULL; // TODO: How to set.
	Bool rls = False;
	const Char* env = NULL;
	Bool help = False;
	Bool version = False;
	int ret_code = 0;
	S64 app_code = 0;
	Bool not_deploy = False;
	Quiet = False;

	_setmode(_fileno(stdout), _O_U8TEXT); // Set the output format to UTF-8.
	{
		int i;
		for (i = 1; i < argc; i++)
		{
			int len = (int)wcslen(argv[i]);
			if (len == 2 && argv[i][0] == L'-')
			{
				switch (argv[i][1])
				{
					case L'i':
						if (input != NULL || i + 1 >= argc)
						{
							wprintf(L"The option '-i' was used incorrectly.\n");
							return 1;
						}
						input = argv[i + 1];
						i++;
						break;
					case L'o':
						if (output != NULL || i + 1 >= argc)
						{
							wprintf(L"The option '-o' was used incorrectly.\n");
							return 1;
						}
						output = argv[i + 1];
						i++;
						break;
					case L's':
						if (sys_dir != NULL || i + 1 >= argc)
						{
							wprintf(L"The option '-s' was used incorrectly.\n");
							return 1;
						}
						sys_dir = argv[i + 1];
						i++;
						break;
					case L'c':
						if (icon != NULL || i + 1 >= argc)
						{
							wprintf(L"The option '-c' was used incorrectly.\n");
							return 1;
						}
						icon = argv[i + 1];
						i++;
						break;
					case L'r':
						if (rls != False)
						{
							wprintf(L"The option '-r' was used incorrectly.\n");
							return 1;
						}
						rls = True;
						break;
					case L'e':
						if (env != NULL || i + 1 >= argc)
						{
							wprintf(L"The option '-e' was used incorrectly.\n");
							return 1;
						}
						env = argv[i + 1];
						i++;
						break;
					case L'h':
						if (help != False)
						{
							wprintf(L"The option '-h' was used incorrectly.\n");
							return 1;
						}
						help = True;
						break;
					case L'v':
						if (version != False)
						{
							wprintf(L"The option '-v' was used incorrectly.\n");
							return 1;
						}
						version = True;
						break;
					case L'q':
						if (Quiet != False)
						{
							wprintf(L"The option '-q' was used incorrectly.\n");
							return 1;
						}
						Quiet = True;
						break;
					case L'a':
						if (app_code != 0)
						{
							wprintf(L"The option '-a' was used incorrectly.\n");
							return 1;
						}
						{
							Char* end_ptr;
							errno = 0;
							app_code = wcstol(argv[i + 1], &end_ptr, 10);
							i++;
							if (*end_ptr != L'\0' || errno == ERANGE || app_code == 0)
							{
								wprintf(L"The option '-a' was used incorrectly.\n");
								return 1;
							}
						}
						break;
					case L'd': // This option is only used in Kuin Editor builds.
						if (not_deploy != False)
						{
							wprintf(L"The option '-d' was used incorrectly.\n");
							return 1;
						}
						not_deploy = True;
						break;
					default:
						wprintf(L"Unexpected option: %s.\n", argv[i]);
						return 1;
				}
			}
			else
			{
				wprintf(L"Unexpected option: %s.\n", argv[i]);
				return 1;
			}
		}
	}
	{
		HMODULE library = LoadLibrary(L"data/d0917.knd");
		if (library == NULL)
		{
			wprintf(L"The file 'data/d0917.knd' could not be opened.\n");
			return 1;
		}
		{
			TypeOfBuild func_build = (TypeOfBuild)GetProcAddress(library, "BuildFile");
			TypeOfVersion func_version = (TypeOfVersion)GetProcAddress(library, "Version");
			TypeOfInitCompiler func_init_compiler = (TypeOfInitCompiler)GetProcAddress(library, "InitCompiler");
			TypeOfFinCompiler func_fin_compiler = (TypeOfFinCompiler)GetProcAddress(library, "FinCompiler");
			TypeOfArchive func_archive = (TypeOfArchive)GetProcAddress(library, "Archive");
			if (func_build == NULL || func_version == NULL)
			{
				wprintf(L"The file 'data/d0917.knd' was broken.\n");
				return 1;
			}
			if (version)
			{
				int major, minor, micro;
				func_version(&major, &minor, &micro);
				wprintf(L"Kuin Programming Language v.%d.%d.%d\n", major, minor, micro);
				wprintf(L"(C)Kuina-chan\n");
				return 0;
			}
			if (help || input == NULL)
			{
				wprintf(L"Usage: kuincl [-i input.kn] [-o output.kn] [-s 'sys' directory] [-c icon.ico] [-e environment] [-a appcode] [-r] [-h] [-v] [-q]\n");
				return 0;
			}
			if (func_build(input, sys_dir, output, icon, related_files, rls, env, Log, LANG, app_code, not_deploy))
			{
				if (rls)
				{
					U8* res_output = GetDir(output == NULL ? input : output, False, L"res.knd");
					U8* res_src = GetDir(input, False, L"res/");
					if (PathFileExists((Char*)(res_src + 0x10)) != 0)
					{
						func_init_compiler(1, -1);
						if (!func_archive(res_output, res_src, app_code))
							ret_code = 1;
						func_fin_compiler();
					}
					free(res_src);
					free(res_output);
				}
			}
			else
				ret_code = 1;
			if (ret_code == 0)
			{
				if (!Quiet)
					wprintf(L"Success.\n");
			}
			else
				wprintf(L"Failure.\n");
		}
		FreeLibrary(library);
	}
	return ret_code;
}

static void Log(const Char* code, const Char* msg, const Char* src, int row, int col)
{
	if (Quiet && code[0] == L'I')
		return;
	switch (code[0])
	{
		case L'E': wprintf(L"[Error] "); break;
		case L'W': wprintf(L"[Warning] "); break;
		case L'I': wprintf(L"[Info] "); break;
		default:
			ASSERT(False);
			break;
	}
	if (code[0] == L'I')
	{
		if (src == NULL)
			wprintf(L"%s\n", msg);
		else
			wprintf(L"%s (%s: %d, %d)\n", msg, src, row, col);
	}
	else
	{
		if (src == NULL)
			wprintf(L"%s: %s\n", code, msg);
		else
			wprintf(L"%s: %s (%s: %d, %d)\n", code, msg, src, row, col);
	}
}

static U8* GetDir(const Char* path, Bool dir, const Char* add_name)
{
	size_t len = wcslen(path);
	size_t len_add_name = add_name == NULL ? 0 : wcslen(add_name);
	U8* result;
	Char* result2;
	if (len == 0)
	{
		result = (U8*)malloc(0x10 + sizeof(Char) * (2 + len_add_name + 1));
		result2 = (Char*)(result + 0x10);
		((S64*)result)[0] = 1;
		((S64*)result)[1] = 2 + len_add_name;
		wcscpy(result2, L"./");
	}
	if (dir)
	{
		Bool backslash_eol = path[len - 1] == L'\\' || path[len - 1] == L'/';
		size_t i;
		result = (U8*)malloc(0x10 + sizeof(Char) * (len + len_add_name + (backslash_eol ? 0 : 1) + 1));
		result2 = (Char*)(result + 0x10);
		((S64*)result)[0] = 1;
		((S64*)result)[1] = len + len_add_name + (backslash_eol ? 0 : 1);
		for (i = 0; i < len; i++)
			result2[i] = path[i] == L'\\' ? L'/' : path[i];
		if (!backslash_eol)
		{
			result2[i] = L'/';
			i++;
		}
		result2[i] = L'\0';
	}
	else
	{
		const Char* ptr = path + len - 1;
		while (ptr != path && *ptr != L'\\' && *ptr != L'/')
			ptr--;
		if (ptr == path)
		{
			result = (U8*)malloc(0x10 + sizeof(Char) * (2 + len_add_name + 1));
			result2 = (Char*)(result + 0x10);
			((S64*)result)[0] = 1;
			((S64*)result)[1] = 2 + len_add_name;
			wcscpy(result2, L"./");
		}
		else
		{
			size_t len2 = ptr - path + 1;
			size_t i;
			result = (U8*)malloc(0x10 + sizeof(Char) * (len2 + len_add_name + 1));
			result2 = (Char*)(result + 0x10);
			((S64*)result)[0] = 1;
			((S64*)result)[1] = len2 + len_add_name;
			for (i = 0; i < len2; i++)
				result2[i] = path[i] == L'\\' ? L'/' : path[i];
			result2[i] = L'\0';
		}
	}
	if (len_add_name != 0)
		wcscat(result2, add_name);
	return result;
}
