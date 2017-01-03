// kuincl.exe
//
// (C)Kuina-chan
//

#include "main.h"

#include <fcntl.h>
#include <io.h>

#define MEM_SIZE (32 * 1024 * 1024)

typedef Bool(*TypeOfBuild)(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void*(*allocator)(size_t size), void(*log_func)(const Char* code, const Char* msg, const Char* src, int row, int col));
typedef void(*TypeOfVersion)(int* major, int* minor, int* micro);

typedef struct SMemList
{
	void* Mem;
	struct SMemList* Next;
} SMemList;

static Bool Quiet;
static SMemList* TopMem;
static SMemList* BottomMem;
static void* CurMem;

static void* Allocator(size_t size);
static void Log(const Char* code, const Char* msg, const Char* src, int row, int col);

int wmain(int argc, Char** argv)
{
	const Char* input = NULL;
	const Char* output = NULL;
	const Char* sys_dir = NULL;
	const Char* icon = NULL;
	Bool rls = False;
	const Char* env = NULL;
	Bool help = False;
	Bool version = False;
	Quiet = False;
	int retCode = 0;

	_setmode(_fileno(stdout), _O_U16TEXT); // Set the output format to UTF-16.
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
		HMODULE library = LoadLibrary(L"sys/d0917.knd");
		if (library == NULL)
		{
			wprintf(L"The file 'sys/d0917.knd' could not be opened.\n");
			return 1;
		}
		{
			TypeOfBuild func_build = (TypeOfBuild)GetProcAddress(library, "Build");
			TypeOfVersion func_version = (TypeOfVersion)GetProcAddress(library, "Version");
			if (func_build == NULL || func_version == NULL)
			{
				wprintf(L"The file 'sys/d0917.knd' was broken.\n");
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
				wprintf(L"Usage: kuincl [-i input.kn] [-o output.kn] [-s 'sys' directory] [-c icon.ico] [-e environment] [-r] [-h] [-v] [-q]\n");
				return 0;
			}
			{
				void* mem = malloc(MEM_SIZE);
				SMemList* node = (SMemList*)malloc(sizeof(SMemList));
				node->Mem = mem;
				node->Next = NULL;
				TopMem = node;
				BottomMem = node;
				CurMem = mem;
			}
			if (func_build(input, sys_dir, output, icon, rls, env, Allocator, Log))
			{
				if (!Quiet) {
					wprintf(L"Success.\n");
				}
			}
			else {
				wprintf(L"Failure.\n");
				retCode = 1;
			}
			{
				SMemList* ptr = TopMem;
				while (ptr != NULL)
				{
					SMemList* ptr2 = ptr;
					free(ptr->Mem);
					ptr = ptr->Next;
					free(ptr2);
				}
			}
		}
		FreeLibrary(library);
	}
	return retCode;
}

static void* Allocator(size_t size)
{
	if ((U8*)CurMem + size >= (U8*)BottomMem->Mem + MEM_SIZE)
	{
		void* mem = malloc(MEM_SIZE);
		SMemList* node = (SMemList*)malloc(sizeof(SMemList));
		node->Mem = mem;
		node->Next = NULL;
		BottomMem->Next = node;
		BottomMem = node;
		CurMem = mem;
	}
	{
		void* result = CurMem;
		CurMem = ((U8*)CurMem) + size;
		return result;
	}
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
