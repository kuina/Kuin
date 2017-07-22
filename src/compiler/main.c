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

typedef struct SIdentifier
{
	Char* Name;
	Char* Hint;
	int Row;
	int Col;
	int ScopeRowBegin;
	int ScopeRowEnd;
} SIdentifier;

typedef struct SIdentifierSet
{
	Char* Src;
	int IdentifierNum;
	SIdentifier* Identifiers;
} SIdentifierSet;

static const void*(*FuncGetSrc)(const U8*) = NULL;
static void(*FuncLog)(const void*, S64, S64) = NULL;
static const void* Src = NULL;
static const void* SrcLine = NULL;
static const Char* SrcChar = NULL;
static int IdentifierSetNum = 0;
static SIdentifierSet* IdentifierSets = NULL;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static void DecSrc(void);
static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), Bool analyze_identifiers);
static FILE* BuildMemWfopen(const Char* file_name, const Char* mode);
static int BuildMemFclose(FILE* file_ptr);
static U16 BuildMemFgetwc(FILE* file_ptr);
static size_t BuildMemGetSize(FILE* file_ptr);
static void BuildMemLog(const Char* code, const Char* msg, const Char* src, int row, int col);
static size_t BuildFileGetSize(FILE* file_ptr);
static void MakeIdentifierSet(SDict* asts);
static const void* MakeIdentifierSet2(const Char* key, const void* value, void* param);
static const void* MakeIdentifierSet3(const Char* key, const void* value, void* param);
static const void* MakeIdentifierSet4(const Char* key, const void* value, void* param);
static const void* MakeIdentifierSet5(const Char* key, const void* value, void* param);
static int CmpIdentifierSet(const void* a, const void* b);
static int CmpIdentifier(const void* a, const void* b);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT Bool BuildMem(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* output, const U8* icon, Bool rls, const U8* env, void(*func_log)(const void* args, S64 row, S64 col))
{
	Bool result;
	FuncGetSrc = func_get_src;
	FuncLog = func_log;
	result = Build(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, (const Char*)(path + 0x10), sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10), output == NULL ? NULL : (const Char*)(output + 0x10), icon == NULL ? NULL : (const Char*)(icon + 0x10), rls, env == NULL ? NULL : (const Char*)(env + 0x10), BuildMemLog, True);
	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return result;
}

EXPORT Bool BuildFile(const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col))
{
	Bool result;
	InitAllocator();
	result = Build(_wfopen, fclose, fgetwc, BuildFileGetSize, path, sys_dir, output, icon, rls, env, func_log, False);
	FinAllocator();
	return result;
}

EXPORT void Interpret1(const void* src, const void* color)
{
	InterpretImpl1(src, color);
}

EXPORT void Interpret2(const U8* path, const void*(*func_get_src)(const U8*), const U8* sys_dir, const U8* env, void(*func_log)(const void* args, S64 row, S64 col))
{
	const Char* sys_dir2 = sys_dir == NULL ? NULL : (const Char*)(sys_dir + 0x10);

	FuncGetSrc = func_get_src;
	FuncLog = func_log;

	// Set the system directory.
	if (sys_dir2 == NULL)
	{
		Char sys_dir3[1024 + 1];
		GetModuleFileName(NULL, sys_dir3, 1024 + 1);
		sys_dir2 = GetDir(sys_dir3, False, L"sys/");
	}
	else
		sys_dir2 = GetDir(sys_dir2, True, NULL);

	SetLogFunc(BuildMemLog, 0, sys_dir2);
	ResetErrOccurred();

	{
		SOption option;
		SDict* asts;
		SDict* dlls;
		MakeOption(&option, (const Char*)(path + 0x10), NULL, sys_dir2, NULL, False, env == NULL ? NULL : (const Char*)(env + 0x10));
		if (!ErrOccurred())
		{
			asts = Parse(BuildMemWfopen, BuildMemFclose, BuildMemFgetwc, BuildMemGetSize, &option);
			if (!ErrOccurred())
			{
				Analyze(asts, &option, &dlls);
				if (!ErrOccurred())
					MakeIdentifierSet(asts);
			}
		}
	}

	FuncGetSrc = NULL;
	FuncLog = NULL;
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
}

EXPORT void Version(S64* major, S64* minor, S64* micro)
{
	*major = 9;
	*minor = 17;
	*micro = 0;
}

EXPORT void InitMemAllocator(void)
{
	InitAllocator();
}

EXPORT void FinMemAllocator(void)
{
	FinAllocator();
}

EXPORT void ResetMemAllocator(void)
{
	ResetAllocator();
}

EXPORT void FreeIdentifierSet(void)
{
	if (IdentifierSets == NULL)
		return;
	{
		int i, j;
		for (i = 0; i < IdentifierSetNum; i++)
		{
			for (j = 0; j < IdentifierSets[i].IdentifierNum; j++)
			{
				free(IdentifierSets[i].Identifiers[j].Name);
				free(IdentifierSets[i].Identifiers[j].Hint);
			}
			free(IdentifierSets[i].Identifiers);
			free(IdentifierSets[i].Src);
		}
	}
	free(IdentifierSets);
	IdentifierSets = NULL;
}

EXPORT void DumpIdentifierSet(const Char* path)
{
	if (IdentifierSets == NULL)
		return;
	{
		FILE* file_ptr = _wfopen(path, L"w, ccs=UTF-8");
		int i, j;
		for (i = 0; i < IdentifierSetNum; i++)
		{
			fwprintf(file_ptr, L"%s:\n", IdentifierSets[i].Src);
			for (j = 0; j < IdentifierSets[i].IdentifierNum; j++)
			{
				SIdentifier* identifier = &IdentifierSets[i].Identifiers[j];
				fwprintf(file_ptr, L"\t%s (%d, %d) [%d, %d]\n", identifier->Name, identifier->Row, identifier->Col, identifier->ScopeRowBegin, identifier->ScopeRowEnd);
				fwprintf(file_ptr, L"\t\t%s\n", identifier->Hint);
			}
		}
		fclose(file_ptr);
	}
}

EXPORT void* GetHint(const U8* name, const U8* src, S64 row)
{
	const Char* name2 = (const Char*)(name + 0x10);
	SIdentifierSet* identifier_set = NULL;
	int i;
	int ptr = -1;
	for (i = 0; i < IdentifierSetNum; i++)
	{
		if (wcscmp(IdentifierSets[i].Src, (const Char*)(src + 0x10)) != 0)
			continue;
		identifier_set = &IdentifierSets[i];
	}
	if (identifier_set == NULL)
		return NULL;
	for (i = 0; i < identifier_set->IdentifierNum; i++)
	{
		if (wcscmp(identifier_set->Identifiers[i].Name, name2) != 0)
			continue;
		ptr = i;
	}
	if (ptr == -1)
		return NULL;

	{
		int max = INT_MIN;
		int best = -1;
		if (row == -1)
			best = ptr;
		while (ptr < identifier_set->IdentifierNum && wcscmp(identifier_set->Identifiers[ptr].Name, name2) == 0)
		{
			int begin = identifier_set->Identifiers[ptr].ScopeRowBegin;
			int end = identifier_set->Identifiers[ptr].ScopeRowEnd;
			if (max < begin && begin <= row && row <= end)
			{
				max = begin;
				best = ptr;
			}
			ptr++;
		}
		if (best == -1)
			return NULL;
		{
			size_t len = wcslen(identifier_set->Identifiers[best].Hint);
			U8* result = (U8*)Alloc(0x10 + sizeof(Char) * (len + 1));
			*(S64*)(result + 0x00) = DefaultRefCntFunc + 1;
			*(S64*)(result + 0x08) = (S64)len;
			wcscpy((Char*)(result + 0x10), identifier_set->Identifiers[best].Hint);
			return result;
		}
	}
}

static void DecSrc(void)
{
	// Decrement 'Src', but do not release it here. It will be released in '.kn'.
	if (Src != NULL)
	{
		(*(S64*)Src)--;
		ASSERT(*(S64*)Src > 0);
	}
}

static Bool Build(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const Char* path, const Char* sys_dir, const Char* output, const Char* icon, Bool rls, const Char* env, void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), Bool analyze_identifiers)
{
	SOption option;
	SDict* asts;
	SAstFunc* entry;
	SDict* dlls;
	SPackAsm pack_asm;
	U32 begin_time;

	// Set the system directory.
	if (sys_dir == NULL)
	{
		Char sys_dir2[1024 + 1];
		GetModuleFileName(NULL, sys_dir2, 1024 + 1);
		sys_dir = GetDir(sys_dir2, False, L"sys/");
	}
	else
		sys_dir = GetDir(sys_dir, True, NULL);

	SetLogFunc(func_log, 0, sys_dir);
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
#if defined(_DEBUG)
	UNUSED(analyze_identifiers);
	MakeIdentifierSet(asts);
#else
	if (analyze_identifiers)
		MakeIdentifierSet(asts);
#endif
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
#if defined (_DEBUG)
	DumpIdentifierSet(NewStr(NULL, L"%s_identifiers.txt", option.OutputFile));
#endif
	return True;

ERR:
	timeEndPeriod(1);
	Err(L"IK0007", NULL);
	return False;
}

static FILE* BuildMemWfopen(const Char* file_name, const Char* mode)
{
	UNUSED(mode);
	{
		U8 file_name2[0x10 + sizeof(Char) * (MAX_PATH + 1)];
		*(S64*)(file_name2 + 0x00) = 2;
		*(S64*)(file_name2 + 0x08) = (S64)wcslen(file_name);
		wcscpy((Char*)(file_name2 + 0x10), file_name);
		DecSrc();
		Src = Call1Asm(file_name2, (void*)(U64)FuncGetSrc);
		if (Src == NULL)
			return NULL;
		SrcLine = (U8*)Src + 0x10;
		SrcChar = (Char*)((U8*)*(void**)SrcLine + 0x10);
		return (FILE*)DummyPtr;
	}
}

static int BuildMemFclose(FILE* file_ptr)
{
	UNUSED(file_ptr);
	DecSrc();
	Src = NULL;
	SrcLine = NULL;
	SrcChar = NULL;
	return 0;
}

static U16 BuildMemFgetwc(FILE* file_ptr)
{
	const void* term;
	{
		S64 len = *(S64*)((U8*)Src + 0x08);
		term = (U8*)Src + 0x10 + len * 0x08;
	}
	UNUSED(file_ptr);
	if (SrcLine == term)
		return L'\0';
	{
		Char c = *SrcChar;
		if (c != L'\0')
		{
			SrcChar++;
			return c;
		}
		SrcLine = (U8*)SrcLine + 0x08;
		if (SrcLine == term)
			return L'\0';
		SrcChar = (Char*)((U8*)*(void**)SrcLine + 0x10);
		return L'\n';
	}
}

static size_t BuildMemGetSize(FILE* file_ptr)
{
	UNUSED(file_ptr);
	{
		size_t total = 0;
		S64 len = *(S64*)((U8*)Src + 0x08);
		void* ptr = (U8*)Src + 0x10;
		S64 i;
		for (i = 0; i < len; i++)
		{
			total += *(S64*)((U8*)*(void**)ptr + 0x08);
			if (total >= 2)
				return 2; // A value of 2 or more is not distinguished.
			ptr = (U8*)ptr + 0x08;
		}
		return total;
	}
}

static void BuildMemLog(const Char* code, const Char* msg, const Char* src, int row, int col)
{
	U8 code2[0x10 + sizeof(Char) * (MAX_PATH + 1)];
	size_t len_msg = wcslen(msg);
	U8* msg2 = (U8*)Alloc(0x10 + sizeof(Char) * (len_msg + 1));
	U8 src2[0x10 + sizeof(Char) * (MAX_PATH + 1)];
	U8 args[0x10 + 0x18];
	{
		*(S64*)(code2 + 0x00) = 1;
		*(S64*)(code2 + 0x08) = (S64)wcslen(code);
		wcscpy((Char*)(code2 + 0x10), code);
	}
	{
		*(S64*)(msg2 + 0x00) = 1;
		*(S64*)(msg2 + 0x08) = (S64)len_msg;
		wcscpy((Char*)(msg2 + 0x10), msg);
	}
	if (src != NULL)
	{
		*(S64*)(src2 + 0x00) = 1;
		*(S64*)(src2 + 0x08) = (S64)wcslen(src);
		wcscpy((Char*)(src2 + 0x10), src);
	}
	{
		*(S64*)(args + 0x00) = 2;
		*(S64*)(args + 0x08) = 3;
		*(void**)(args + 0x10) = code2;
		*(void**)(args + 0x18) = msg2;
		*(void**)(args + 0x20) = src == NULL ? NULL : src2;
	}
	Call3Asm(args, (void*)(S64)row, (void*)(S64)col, (void*)(U64)FuncLog);
}

static size_t BuildFileGetSize(FILE* file_ptr)
{
	int file_size;
	fseek(file_ptr, 0, SEEK_END);
	file_size = (int)ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);
	return (size_t)file_size;
}

static void MakeIdentifierSet(SDict* asts)
{
	int len = 0;
	FreeIdentifierSet();
	DictForEach(asts, MakeIdentifierSet2, &len);
	IdentifierSetNum = len;
	IdentifierSets = (SIdentifierSet*)malloc(sizeof(SIdentifierSet) * (size_t)len);
	{
		int cnt = 0;
		DictForEach(asts, MakeIdentifierSet3, &cnt);
		qsort(IdentifierSets, len, sizeof(SIdentifierSet), CmpIdentifierSet);
	}
}

static const void* MakeIdentifierSet2(const Char* key, const void* value, void* param)
{
	int* len = (int*)param;
	UNUSED(key);
	(*len)++;
	return value;
}

static const void* MakeIdentifierSet3(const Char* key, const void* value, void* param)
{
	int* cnt = (int*)param;
	size_t name_len = wcslen(key);
	SAst* ast = (SAst*)value;
	int len = 0;
	IdentifierSets[*cnt].Src = (Char*)malloc(sizeof(Char) * (name_len + 1));
	wcscpy(IdentifierSets[*cnt].Src, key);
	DictForEach(ast->ScopeChildren, MakeIdentifierSet4, &len);
	IdentifierSets[*cnt].IdentifierNum = len;
	IdentifierSets[*cnt].Identifiers = (SIdentifier*)malloc(sizeof(SIdentifier) * (size_t)(len));
	{
		int cnt2[3];
		cnt2[0] = *cnt;
		cnt2[1] = 0;
		cnt2[2] = 1;
		DictForEach(ast->ScopeChildren, MakeIdentifierSet5, cnt2);
		qsort(IdentifierSets[*cnt].Identifiers, len, sizeof(SIdentifier), CmpIdentifier);
	}
	(*cnt)++;
	return value;
}

static const void* MakeIdentifierSet4(const Char* key, const void* value, void* param)
{
	int* len = (int*)param;
	SAst* ast = (SAst*)value;
	UNUSED(key);
	if (ast->Name != NULL)
		(*len)++;
	if (ast->ScopeChildren != NULL)
		DictForEach(ast->ScopeChildren, MakeIdentifierSet4, len);
	return value;
}

static const void* MakeIdentifierSet5(const Char* key, const void* value, void* param)
{
	int* cnt = (int*)param;
	SAst* ast = (SAst*)value;
	if (ast->Name != NULL)
	{
		size_t name_len = wcslen(ast->Name);
		SIdentifier* identifier = &IdentifierSets[cnt[0]].Identifiers[cnt[1]];
		UNUSED(key);
		if (cnt[2] == 1)
		{
			identifier->Name = (Char*)malloc(sizeof(Char) * (name_len + 2));
			identifier->Name[0] = L'@';
			wcscpy(identifier->Name + 1, ast->Name);
		}
		else
		{
			identifier->Name = (Char*)malloc(sizeof(Char) * (name_len + 1));
			wcscpy(identifier->Name, ast->Name);
		}
		identifier->Row = ast->Pos->Row;
		identifier->Col = ast->Pos->Col;
		ASSERT(ast->ScopeRowBegin == NULL && ast->ScopeRowEnd == NULL || ast->ScopeRowBegin != NULL && ast->ScopeRowEnd != NULL);
		identifier->ScopeRowBegin = ast->ScopeRowBegin == NULL ? -1 : (*ast->ScopeRowBegin)->Pos->Row;
		identifier->ScopeRowEnd = ast->ScopeRowEnd == NULL ? -1 : (*ast->ScopeRowEnd)->Pos->Row;
		ASSERT(identifier->ScopeRowBegin == -1 || identifier->ScopeRowBegin <= identifier->Row && identifier->Row <= identifier->ScopeRowEnd);
		{
			int hint_len;
			const Char* hint;
			if (ast->AnalyzedCache == NULL)
			{
				hint = L"unreferenced";
				hint_len = (int)wcslen(hint);
			}
			else
				hint = GetDefinition(&hint_len, ast->AnalyzedCache);
			identifier->Hint = (Char*)malloc(sizeof(Char) * (size_t)(hint_len + 1));
			wcscpy(identifier->Hint, hint);
		}
		cnt[1]++;
	}
	if (ast->ScopeChildren != NULL)
	{
		int cnt2 = cnt[2];
		cnt[2] = 0;
		DictForEach(ast->ScopeChildren, MakeIdentifierSet5, cnt);
		cnt[2] = cnt2;
	}
	return value;
}

static int CmpIdentifierSet(const void* a, const void* b)
{
	const SIdentifierSet* a2 = (const SIdentifierSet*)a;
	const SIdentifierSet* b2 = (const SIdentifierSet*)b;
	return wcscmp(a2->Src, b2->Src);
}

static int CmpIdentifier(const void* a, const void* b)
{
	const SIdentifier* a2 = (const SIdentifier*)a;
	const SIdentifier* b2 = (const SIdentifier*)b;
	int result = wcscmp(a2->Name, b2->Name);
	if (result == 0)
		result = a2->Row - b2->Row;
	return result;
}
