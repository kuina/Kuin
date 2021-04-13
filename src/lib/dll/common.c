#include "common.h"

SEnvVars EnvVars;

void InitEnvVars(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (EnvVars.Heap != NULL)
		return;

	EnvVars.Heap = heap;
#if defined(_DEBUG)
	EnvVars.HeapCnt = heap_cnt;
#else
	UNUSED(heap_cnt);
#endif
	EnvVars.AppCode = app_code;
	EnvVars.UseResFlags = use_res_flags;

	// The resource root directory.
	{
		Char res_dir_path[KUIN_MAX_PATH + 12 + 1];
		GetModuleFileName(NULL, res_dir_path, KUIN_MAX_PATH);
		{
			Char* ptr = wcsrchr(res_dir_path, L'\\');
			if (ptr != NULL)
				*(ptr + 1) = L'\0';
		}
		wcscat(res_dir_path, L"_resdir_.txt");
		if (PathFileExists(res_dir_path))
		{
			Char path[KUIN_MAX_PATH + 1];
			FILE* file_ptr = _wfopen(res_dir_path, L"r, ccs=UTF-8");
			fgetws(path, KUIN_MAX_PATH, file_ptr);
			Char* ptr = path;
			while (ptr[1] != L'\0')
				ptr++;
			while (ptr >= path && (*ptr == L'\n' || *ptr == L'\r'))
			{
				*ptr = L'\0';
				ptr--;
			}
			wcscpy(EnvVars.ResRoot, path);
			SetCurrentDirectory(path);
		}
		else
		{
			Char* ptr;
			GetModuleFileName(NULL, EnvVars.ResRoot, KUIN_MAX_PATH);
			ptr = wcsrchr(EnvVars.ResRoot, L'\\');
			if (ptr != NULL)
				*(ptr + 1) = L'\0';
			ptr = EnvVars.ResRoot;
			while (*ptr != L'\0')
			{
				if (*ptr == L'\\')
					*ptr = L'/';
				ptr++;
			}
		}
	}
}

void* AllocMem(size_t size)
{
	void* result = HeapAlloc(EnvVars.Heap, HEAP_GENERATE_EXCEPTIONS, (SIZE_T)size);
#if defined(_DEBUG)
	memset(result, 0xcd, size);
	if (EnvVars.HeapCnt != NULL)
		(*EnvVars.HeapCnt)++;
#endif
	return result;
}

void FreeMem(void* ptr)
{
	HeapFree(EnvVars.Heap, 0, ptr);
#if defined(_DEBUG)
	if (EnvVars.HeapCnt != NULL)
	{
		(*EnvVars.HeapCnt)--;
		ASSERT(*EnvVars.HeapCnt >= 0);
}
#endif
}

void ThrowImpl(U32 code)
{
	RaiseException((DWORD)code, 0, 0, NULL);
}

Bool IsResUsed(EUseResFlagsKind kind)
{
	S64 idx = (S64)kind;
	ASSERT(1 <= idx && (idx - 1) / 8 < USE_RES_FLAGS_LEN);
	return (EnvVars.UseResFlags[(idx - 1) / 8] & (1 << ((idx - 1) % 8))) != 0;
}
