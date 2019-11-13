#include "parse.h"

#include "ast.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "pos.h"
#include "stack.h"
#include "util.h"

#define AUXILIARY_BUF_SIZE (4096)

static const Char* Reserved[] =
{
	L"alias",
	L"assert",
	L"bit16",
	L"bit32",
	L"bit64",
	L"bit8",
	L"block",
	L"bool",
	L"break",
	L"case",
	L"catch",
	L"char",
	L"class",
	L"const",
	L"dbg",
	L"default",
	L"dict",
	L"do",
	L"elif",
	L"else",
	L"end",
	L"enum",
	L"env",
	L"false",
	L"finally",
	L"float",
	L"for",
	L"func",
	L"if",
	L"include",
	L"inf",
	L"int",
	L"list",
	L"me",
	L"null",
	L"queue",
	L"ret",
	L"skip",
	L"stack",
	L"super",
	L"switch",
	L"throw",
	L"to",
	L"true",
	L"try",
	L"var",
	L"while",
};

typedef enum ECharColor
{
	CharColor_None,
	CharColor_Identifier,
	CharColor_Global,
	CharColor_Reserved,
	CharColor_Number,
	CharColor_Str,
	CharColor_Char,
	CharColor_LineComment,
	CharColor_Comment,
	CharColor_Symbol,
	CharColor_Err,
} ECharColor;

typedef enum EAlignmentToken
{
	AlignmentToken_None = 0x01,
	AlignmentToken_Value = 0x02,
	AlignmentToken_Ope2 = 0x04,
	AlignmentToken_Pr = 0x08,
	AlignmentToken_Comma = 0x10,
} EAlignmentToken;

typedef struct SKeywordListItem
{
	const Char* Name;
	const SAst* Ast;
	int* First;
	int* Last;
} SKeywordListItem;

typedef struct SKeywordTypeList
{
	struct SKeywordTypeList* Next;
	Char* Type;
} SKeywordTypeList;

// Assembly functions.
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

static FILE*(*FuncWfopen)(const Char*, const Char*);
static int(*FuncFclose)(FILE*);
static U16(*FuncFgetwc)(FILE*);
static size_t(*FuncSize)(FILE*);
static SDict* Srces;
static SDict* Srces2;
static SDict* SrcRefPos;
static const SOption* Option;
static FILE* FilePtr;
static const Char* SrcName;
static int Row;
static int Col;
static Bool SubSrc;
static Char FileBuf;
static Char FileBufTmp; // For single line comments and line breaking.
static Bool IsLast;
static SStack* Scope;
static U32 UniqueCnt;
static U8* MakeUseResFlags;

static const Char* GetKeywordsEnd; 
static const Char* GetKeywordsSrcName;
static int GetKeywordsCursorX;
static int GetKeywordsCursorY;
static U64 GetKeywordsFlags;
static void* GetKeywordsCallback;
static int GetKeywordsKeywordListNum;
static const SKeywordListItem** GetKeywordsKeywordList;
static SKeywordTypeList* KeywordTypeListTop;
static SKeywordTypeList* KeywordTypeListBottom;
static SAstFunc* KeywordHintFunc;

static void GetKeywordsRootImpl(const Char** str, const Char* end, const Char* src_name, int x, int y, U64 flags, void* callback, int keyword_list_num, const void* keyword_list);
static const void* ParseSrc(const Char* src_name, const void* ast, void* param);
static Char ReadBuf(void);
static Char Read(void);
static void ReadComment(void);
static Char ReadChar(void);
static Char ReadStrict(void);
static const Char* ReadIdentifier(Bool skip_spaces, Bool ref);
static const Char* ReadFuncAttr(void);
static void ReadUntilRet(Char c);
static void InitAst(SAst* ast, EAstTypeId type_id, const SPos* pos, const Char* name, Bool set_parent, Bool init_refeds);
static void InitAstExpr(SAstExpr* ast, EAstTypeId type_id, const SPos* pos);
static void AddScopeName(SAst* ast, Bool refuse_reserved);
static void AssertNextChar(Char c, Bool skip_spaces);
static void NextCharErr(Char c, Char c2);
static void AddScopeRefeds(SAst* ast);
static void PushDummyScope(SAst* ast);
static void AddEndPosScope(void);
static SAstArg* MakeBlockVar(int row, int col);
static void ObtainBlockName(SAst* ast);
static SAstExprValue* ObtainPrimValue(const SPos* pos, EAstTypePrimKind kind, const void* value);
static SAstExprValue* ObtainStrValue(const SPos* pos, const Char* value);
static Char EscChar(Char c);
static Bool IsCorrectSrcName(const Char* name, Bool skip_dot);
static void AddSrc(const Char* name);
static Bool IsReserved(const Char* word);
static SAstStatBlock* ParseDummyBlock(SAstStat** out_stat, EAstTypeId* out_type_id, EAstTypeId type_id, SAst* block);
static SAstRoot* ParseRoot(SAstRoot* ast);
static SAstFunc* ParseFunc(const Char* parent_class, Bool overridden);
static SAstVar* ParseVar(EAstArgKind kind, const Char* parent_class);
static SAstConst* ParseConst(void);
static SAstAlias* ParseAlias(void);
static void ParseInclude(void);
static SAstClass* ParseClass(void);
static SAstEnum* ParseEnum(void);
static SAstArg* ParseArg(EAstArgKind kind, const Char* parent_class);
static SAstStat* ParseStat(SAst* block);
static SAstStat* ParseStatEnd(int row, int col, SAst* block);
static SAstStat* ParseStatFunc(void);
static SAstStat* ParseStatVar(void);
static SAstStat* ParseStatConst(void);
static SAstStat* ParseStatAlias(void);
static SAstStat* ParseStatClass(void);
static SAstStat* ParseStatEnum(void);
static SAstStat* ParseStatIf(void);
static SAstStat* ParseStatElIf(int row, int col, const SAst* block);
static SAstStat* ParseStatElse(int row, int col, const SAst* block);
static SAstStat* ParseStatSwitch(int row, int col);
static SAstStat* ParseStatCase(int row, int col, const SAst* block);
static SAstStat* ParseStatDefault(int row, int col, const SAst* block);
static SAstStat* ParseStatWhile(void);
static SAstStat* ParseStatFor(int row, int col);
static SAstStat* ParseStatTry(int row, int col);
static SAstStat* ParseStatCatch(int row, int col, const SAst* block);
static SAstStat* ParseStatFinally(int row, int col, const SAst* block);
static SAstStat* ParseStatThrow(void);
static SAstStat* ParseStatBlock(void);
static SAstStat* ParseStatRet(void);
static SAstStat* ParseStatDo(void);
static SAstStat* ParseStatBreak(void);
static SAstStat* ParseStatSkip(void);
static SAstStat* ParseStatAssert(void);
static SAstType* ParseType(void);
static SAstExpr* ParseExpr(void);
static SAstExpr* ParseExprThree(void);
static SAstExpr* ParseExprOr(void);
static SAstExpr* ParseExprAnd(void);
static SAstExpr* ParseExprCmp(void);
static SAstExpr* ParseExprCat(void);
static SAstExpr* ParseExprAdd(void);
static SAstExpr* ParseExprMul(void);
static SAstExpr* ParseExprPlus(void);
static SAstExpr* ParseExprPow(void);
static SAstExpr* ParseExprCall(void);
static SAstExpr* ParseExprValue(void);
static SAstExpr* ParseExprNumber(int row, int col, Char c);
static void InterpretImpl1Color(int* ptr, int str_level, const Char* str, U8* color, S64 comment_level, U64 flags);
static void InterpretImpl1Align(int* ptr_buf, int* ptr_str, Char* buf, const Char* str, S64* comment_level, U64* flags, S64* tab_context, S64 cursor_x, S64* new_cursor_x, Char* add_end);
static void InterpretImpl1AlignRecursion(int* ptr_buf, int* ptr_str, int str_level, int type_level, Char* buf, const Char* str, S64* comment_level, U64* flags, S64* tab_context, EAlignmentToken* prev, S64 cursor_x, S64* new_cursor_x);
static void InterpretImpl1Write(int* ptr, Char* buf, Char c);
static void Interpret1Impl1UpdateCursor(S64 cursor_x, S64* new_cursor_x, int* ptr_str, int* ptr_buf);
static Char GetKeywordsRead(const Char** str);
static void GetKeywordsReadComment(const Char** str);
static Char GetKeywordsReadChar(const Char** str);
static Char GetKeywordsReadStrict(const Char** str);
static Bool GetKeywordsReadIdentifier(Char* buf, const Char** str, Bool skip_spaces, Bool ref);
static Bool GetKeywordsReadUntil(const Char** str, Char c);
static void GetKeywordsAdd(const Char* str);
static Bool GetKeywordsReadType(const Char** str, Char** type);
static Bool GetKeywordsReadExpr(const Char** str, Char** type);
static Bool GetKeywordsReadExprThree(const Char** str, Char** type);
static Bool GetKeywordsReadExprOr(const Char** str, Char** type);
static Bool GetKeywordsReadExprAnd(const Char** str, Char** type);
static Bool GetKeywordsReadExprCmp(const Char** str, Char** type);
static Bool GetKeywordsReadExprCat(const Char** str, Char** type);
static Bool GetKeywordsReadExprAdd(const Char** str, Char** type);
static Bool GetKeywordsReadExprMul(const Char** str, Char** type);
static Bool GetKeywordsReadExprPlus(const Char** str, Char** type);
static Bool GetKeywordsReadExprPow(const Char** str, Char** type);
static Bool GetKeywordsReadExprCall(const Char** str, Char** type);
static Bool GetKeywordsReadExprValue(const Char** str, Char** type);
static Bool GetKeywordsReadExprNumber(const Char** str, Char** type, Char c);
static void GetKeywordsAddEnum(void);
static void GetKeywordsAddMember(const Char* type);
static void GetKeywordsAddKeywords(Char kind);
static Char* NewKeywordType(const Char* keyword_type);
static Char* CatKeywordType(const Char* keyword_type1, const Char* keyword_type2);
static Char* GetKeywordType(const Char* identifier);
static Char* GetKeywordTypeRecursion(const SAstType* type);
static void PtrToStr(Char* str, const void* ptr);
static void StrToPtr(void* ptr, const Char* str);
static void GetKeywordHintTypeNameRecursion(size_t* len, Char* buf, const SAstType* type);

SDict* Parse(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const SOption* option, U8* use_res_flags)
{
	Bool end_flag = False;
	FuncWfopen = func_wfopen;
	FuncFclose = func_fclose;
	FuncFgetwc = func_fgetwc;
	FuncSize = func_size;
	MakeUseResFlags = use_res_flags;

#if defined(_DEBUG)
	{
		int len = (int)(sizeof(Reserved) / sizeof(Char*));
		int i;
		for (i = 0; i < len - 1; i++)
			ASSERT(wcscmp(Reserved[i], Reserved[i + 1]) < 0);
	}
#endif

	Srces = NULL;
	SrcRefPos = NULL;
	{
		const Char* path = NewStr(NULL, L"\\%s", option->SrcName);
		if (!IsCorrectSrcName(path, False))
		{
			Err(L"EP0060", NULL, path);
			return NULL;
		}
		Srces = DictAdd(Srces, path, NULL);
	}
	Srces = DictAdd(Srces, L"kuin", NULL);
	Option = option;
	switch (Option->Env)
	{
		case Env_Wnd:
			Srces = DictAdd(Srces, L"wnd", NULL);
			break;
		case Env_Cui:
			Srces = DictAdd(Srces, L"cui", NULL);
			break;
		default:
			ASSERT(False);
			break;
	}
	while (!end_flag)
	{
		end_flag = True;
		Srces2 = NULL;
		DictForEach(Srces, ParseSrc, &end_flag);
		Srces = Srces2;
	}
#if defined(_DEBUG)
	if (!ErrOccurred())
	{
		const SAst* main_ast = (const SAst*)DictSearch(Srces, NewStr(NULL, L"\\%s", option->SrcName));
		ASSERT(main_ast != NULL);
		Dump1(NewStr(NULL, L"%s_parsed.xml", Option->OutputFile), main_ast);
	}
#endif
	return Srces;
}

Bool InterpretImpl1(void* str, void* color, void* comment_level, void* flags, S64 line, void* me, void* replace_func, S64 cursor_x, S64 cursor_y, S64* new_cursor_x, S64 old_line, S64 new_line)
{
#if defined(_DEBUG)
	{
		S64 line_len = *(S64*)((U8*)str + 0x08);
		ASSERT(-1 <= line && line < line_len);
	}
#endif
	if (line == -1)
	{
		S64 line_len = *(S64*)((U8*)str + 0x08);
		S64 comment_level_context = 0;
		U64 flags_context = 0;
		S64 tab_context = 0;
		S64 i;
		Char buf_str[0x10 + AUXILIARY_BUF_SIZE + 1];
		Char add_end[32];
		add_end[0] = L'\0';
		for (i = 0; i < line_len; i++)
		{
			void** str2 = (void**)((U8*)str + 0x10 + 0x08 * (size_t)i);
			void** color2 = (void**)((U8*)color + 0x10 + 0x08 * (size_t)i);
			S64* comment_level2 = (S64*)((U8*)comment_level + 0x10 + 0x08 * (size_t)i);
			U64* flags2 = (U64*)((U8*)flags + 0x10 + 0x08 * (size_t)i);

			const Char* str3 = (Char*)((U8*)*str2 + 0x10);
			Char* dst_str = buf_str + 0x08;

			S64 len;
			{
				int ptr_buf = 0;
				int ptr_str = 0;
				InterpretImpl1Align(&ptr_buf, &ptr_str, dst_str, (Char*)((U8*)*str2 + 0x10), &comment_level_context, &flags_context, &tab_context, cursor_x, i == cursor_y ? new_cursor_x : NULL, i == old_line ? add_end : NULL );
				len = (S64)ptr_buf;
			}
			if (add_end[0] != L'\0' && tab_context == 0)
				add_end[0] = L'\0';

			if (wcscmp(str3, dst_str) != 0)
			{
				((S64*)buf_str)[0] = 2;
				((S64*)buf_str)[1] = len;
				if (me != NULL)
					(*(S64*)me)++;
				Call3Asm(me, (void*)i, (void*)buf_str, replace_func);

				str2 = (void**)((U8*)str + 0x10 + 0x08 * (size_t)i);
				color2 = (void**)((U8*)color + 0x10 + 0x08 * (size_t)i);
				comment_level2 = (S64*)((U8*)comment_level + 0x10 + 0x08 * (size_t)i);
				flags2 = (U64*)((U8*)flags + 0x10 + 0x08 * (size_t)i);
			}
			*comment_level2 = comment_level_context;
			*flags2 = (*flags2 & 0x02) | flags_context;

			{
				int ptr = 0;
				InterpretImpl1Color(&ptr, 0, (Char*)((U8*)*str2 + 0x10), (U8*)*color2 + 0x10, *comment_level2, *flags2);
			}
		}
		if (add_end[0] != L'\0')
		{
			if (me != NULL)
				(*(S64*)me)++;
			Call3Asm(me, (void*)(-1 - new_line), (void*)add_end, replace_func);
			return False;
		}
	}
	else
	{
		void* str2 = *(void**)((U8*)str + 0x10 + 0x08 * (size_t)line);
		const Char* str3 = (Char*)((U8*)str2 + 0x10);
		U8* color2 = (U8*)*(void**)((U8*)color + 0x10 + 0x08 * (size_t)line) + 0x10;
		S64 comment_level2 = *(S64*)((U8*)comment_level + 0x10 + 0x08 * (size_t)line);
		U64 flags2 = *(U64*)((U8*)flags + 0x10 + 0x08 * (size_t)line);
		int ptr = 0;
		InterpretImpl1Color(&ptr, 0, str3, color2, comment_level2, flags2);
	}
	return True;
}

void* GetKeywordsRoot(const Char** str, const Char* end, const Char* src_name, int x, int y, U64 flags, void* callback, int keyword_list_num, const void* keyword_list)
{
	SKeywordTypeList* item = (SKeywordTypeList*)AllocMem(sizeof(SKeywordTypeList));
	item->Next = NULL;
	item->Type = (Char*)AllocMem(sizeof(Char));
	item->Type[0] = L'\0';
	KeywordTypeListTop = item;
	KeywordTypeListBottom = item;
	KeywordHintFunc = NULL;
	GetKeywordsRootImpl(str, end, src_name, x, y, flags, callback, keyword_list_num, keyword_list);
	SKeywordTypeList* ptr = KeywordTypeListTop;
	while (ptr != NULL)
	{
		SKeywordTypeList* ptr2 = ptr;
		ptr = ptr->Next;
		FreeMem(ptr2->Type);
		FreeMem(ptr2);
	}
	if (KeywordHintFunc != NULL)
	{
		Char hint[AUXILIARY_BUF_SIZE + 1];
		size_t len = 0;
		len += swprintf(hint + len, AUXILIARY_BUF_SIZE, L"func %s@%s(", ((SAst*)KeywordHintFunc)->Pos->SrcName, ((SAst*)KeywordHintFunc)->Name);
		SListNode* ptr2 = KeywordHintFunc->Args->Top;
		if ((KeywordHintFunc->FuncAttr & FuncAttr_MakeInstance) != 0 && ptr2 != NULL)
			ptr2 = ptr2->Next;
		Bool first = True;
		while (ptr2 != NULL)
		{
			const SAstArg* arg = (const SAstArg*)ptr2->Data;
			if (first)
				first = False;
			else if (len < AUXILIARY_BUF_SIZE)
				len += swprintf(hint + len, AUXILIARY_BUF_SIZE - len, L", ");
			if (len < AUXILIARY_BUF_SIZE)
				len += swprintf(hint + len, AUXILIARY_BUF_SIZE - len, L"%s: ", ((SAst*)arg)->Name);
			GetKeywordHintTypeNameRecursion(&len, hint, arg->Type);
			ptr2 = ptr2->Next;
		}
		if (len < AUXILIARY_BUF_SIZE)
			len += swprintf(hint + len, AUXILIARY_BUF_SIZE - len, L")");
		if (KeywordHintFunc->Ret != NULL)
		{
			if (len < AUXILIARY_BUF_SIZE)
				len += swprintf(hint + len, AUXILIARY_BUF_SIZE - len, L": ");
			GetKeywordHintTypeNameRecursion(&len, hint, KeywordHintFunc->Ret);
		}
		KeywordHintFunc = NULL;
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)result)[0] = 1;
		((S64*)result)[1] = len;
		memcpy(result + 0x10, hint, sizeof(Char) * (len + 1));
		return result;
	}
	return NULL;
}

void GetDbgVars(int keyword_list_num, const void* keyword_list, const Char* pos_name, int pos_row, HANDLE process_handle, U64 start_addr, const CONTEXT* context, void* callback)
{
	const SKeywordListItem** keyword_list2 = (const SKeywordListItem**)keyword_list;
	int i;
	Char buf1[0x08 + 256];
	Char buf2[0x08 + 1024];
	Char* str1 = buf1 + 0x08;
	Char* str2 = buf2 + 0x08;
	U64 addr;
	for (i = 0; i < keyword_list_num; i++)
	{
		const SKeywordListItem* item = keyword_list2[i];
		if (item->Name[0] == L'%' || item->Name[0] == L'.' || item->Ast->TypeId != AstTypeId_Arg)
			continue;
		if (item->Name[0] == L'@')
		{
			if (wcscmp(pos_name, item->Ast->Pos->SrcName) == 0)
				wcscpy(str1, item->Name);
			else
			{
				if (!item->Ast->Public)
					continue;
				wcscpy(str1, item->Ast->Pos->SrcName);
				wcscat(str1, item->Name);
			}
		}
		else if (wcscmp(pos_name, item->Ast->Pos->SrcName) == 0 && *item->First <= pos_row && pos_row <= *item->Last)
			wcscpy(str1, item->Name);
		else
			continue;
		const SAstArg* arg = (const SAstArg*)item->Ast;
		switch (arg->Kind)
		{
			case AstArgKind_Global:
				addr = start_addr + *arg->Addr;
				break;
			case AstArgKind_LocalArg:
			case AstArgKind_LocalVar:
				addr = context->Rsp + *arg->Addr;
				break;
			default:
				continue;
		}
		switch (((SAst*)arg->Type)->TypeId)
		{
			case AstTypeId_TypePrim:
				switch (((SAstTypePrim*)arg->Type)->Kind)
				{
					case AstTypePrimKind_Int:
						{
							S64 value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%I64d (0x%016I64X)", value, (U64)value);
						}
						break;
					case AstTypePrimKind_Float:
						{
							double value = 0.0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%g", value);
						}
						break;
					case AstTypePrimKind_Char:
						{
							Char value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"'%c' %d (0x%04X)", value, value, value);
						}
						break;
					case AstTypePrimKind_Bool:
						{
							Bool value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%s", value ? L"true" : L"false");
						}
						break;
					default:
						ASSERT(False);
						break;
				}
				break;
			case AstTypeId_TypeBit:
				switch (((SAstTypeBit*)arg->Type)->Size)
				{
					case 1:
						{
							U8 value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%u (0x%02X)", value, value);
						}
						break;
					case 2:
						{
							U16 value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%u (0x%04X)", value, value);
						}
						break;
					case 4:
						{
							U32 value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%u (0x%08X)", value, value);
						}
						break;
					case 8:
						{
							U64 value = 0;
							ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
							swprintf(str2, 1024, L"%I64u (0x%016I64X)", value, value);
						}
						break;
					default:
						ASSERT(False);
						break;
				}
				break;
			default:
				if (IsEnum(arg->Type))
				{
					U64 value = 0;
					ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
					swprintf(str2, 1024, L"%I64d (0x%016I64X)", value, value);
				}
				else
				{
					U64 value = 0;
					ReadProcessMemory(process_handle, (LPVOID)addr, &value, sizeof(value), NULL);
					swprintf(str2, 1024, L"0x%016I64X", value);
				}
				break;
		}
		((S64*)buf1)[0] = 2;
		((S64*)buf1)[1] = (S64)wcslen(str1);
		((S64*)buf2)[0] = 2;
		((S64*)buf2)[1] = (S64)wcslen(str2);
		Call3Asm((void*)1, buf1, buf2, callback);
	}
}

static void GetKeywordsRootImpl(const Char** str, const Char* end, const Char* src_name, int x, int y, U64 flags, void* callback, int keyword_list_num, const void* keyword_list)
{
	GetKeywordsEnd = end;
	GetKeywordsSrcName = src_name;
	GetKeywordsCursorX = x;
	GetKeywordsCursorY = y;
	GetKeywordsFlags = flags;
	GetKeywordsCallback = callback;
	GetKeywordsKeywordListNum = keyword_list_num;
	GetKeywordsKeywordList = (const SKeywordListItem**)keyword_list;

	Char buf[129];
	if (GetKeywordsReadIdentifier(buf, str, True, False))
	{
		GetKeywordsAdd(L"alias");
		GetKeywordsAdd(L"assert");
		GetKeywordsAdd(L"block");
		GetKeywordsAdd(L"break");
		GetKeywordsAdd(L"case");
		GetKeywordsAdd(L"catch");
		GetKeywordsAdd(L"class");
		GetKeywordsAdd(L"const");
		GetKeywordsAdd(L"default");
		GetKeywordsAdd(L"do");
		GetKeywordsAdd(L"elif");
		GetKeywordsAdd(L"else");
		GetKeywordsAdd(L"end");
		GetKeywordsAdd(L"enum");
		GetKeywordsAdd(L"finally");
		GetKeywordsAdd(L"for");
		GetKeywordsAdd(L"func");
		GetKeywordsAdd(L"if");
		GetKeywordsAdd(L"include");
		GetKeywordsAdd(L"ret");
		GetKeywordsAdd(L"skip");
		GetKeywordsAdd(L"switch");
		GetKeywordsAdd(L"throw");
		GetKeywordsAdd(L"try");
		GetKeywordsAdd(L"var");
		GetKeywordsAdd(L"while");
	}
	else if (wcscmp(buf, L"func") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L'('))
			return;
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return;
		if (c != L')')
		{
			for (; ; )
			{
				if (GetKeywordsReadIdentifier(buf, str, True, False))
					return;
				if (GetKeywordsReadUntil(str, L':'))
					return;
				{
					Char* tmp_type = L"";
					if (GetKeywordsReadType(str, &tmp_type))
						return;
				}
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return;
				if (c == L')')
					break;
				if (c != L',')
					return;
			}
		}
		c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return;
		if (c == L':')
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadType(str, &tmp_type))
				return;
		}
	}
	else if (wcscmp(buf, L"end") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
		{
			GetKeywordsAdd(L"block");
			GetKeywordsAdd(L"class");
			GetKeywordsAdd(L"enum");
			GetKeywordsAdd(L"for");
			GetKeywordsAdd(L"func");
			GetKeywordsAdd(L"if");
			GetKeywordsAdd(L"switch");
			GetKeywordsAdd(L"try");
			GetKeywordsAdd(L"while");
		}
	}
	else if (wcscmp(buf, L"break") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
		{
			GetKeywordsAdd(L"block");
			GetKeywordsAdd(L"for");
			GetKeywordsAdd(L"if");
			GetKeywordsAdd(L"switch");
			GetKeywordsAdd(L"try");
			GetKeywordsAdd(L"while");
		}
	}
	else if (wcscmp(buf, L"skip") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
		{
			GetKeywordsAdd(L"for");
			GetKeywordsAdd(L"while");
		}
	}
	else if (wcscmp(buf, L"var") == 0 || wcscmp(buf, L"const") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L':'))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadType(str, &tmp_type))
				return;
		}
		if (GetKeywordsReadUntil(str, L':'))
			return;
		if (GetKeywordsReadUntil(str, L':'))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadExpr(str, &tmp_type))
				return;
		}
	}
	else if (wcscmp(buf, L"alias") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L':'))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadType(str, &tmp_type))
				return;
		}
	}
	else if (wcscmp(buf, L"class") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L'('))
			return;
		if (GetKeywordsReadIdentifier(buf, str, True, True))
		{
			GetKeywordsAddKeywords(L'c');
			return;
		}
	}
	else if (wcscmp(buf, L"do") == 0 || wcscmp(buf, L"assert") == 0 || wcscmp(buf, L"elif") == 0 || wcscmp(buf, L"throw") == 0 || wcscmp(buf, L"ret") == 0)
	{
		Char* tmp_type = L"";
		if (GetKeywordsReadExpr(str, &tmp_type))
			return;
	}
	else if (wcscmp(buf, L"case") == 0 || wcscmp(buf, L"catch") == 0)
	{
		Char c;
		for (; ; )
		{
			{
				Char* tmp_type = L"";
				if (GetKeywordsReadExpr(str, &tmp_type))
					return;
			}
			c = GetKeywordsReadChar(str);
			if (c == L'\0')
				return;
			if (c == L',')
				continue;
			(*str)--;
			if (GetKeywordsReadIdentifier(buf, str, True, False))
			{
				GetKeywordsAdd(L"to");
				return;
			}
			if (wcscmp(buf, L"to") != 0)
				return;
		}
	}
	else if (wcscmp(buf, L"for") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L'('))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadExpr(str, &tmp_type))
				return;
		}
		if (GetKeywordsReadUntil(str, L','))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadExpr(str, &tmp_type))
				return;
		}
		if (GetKeywordsReadUntil(str, L','))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadExpr(str, &tmp_type))
				return;
		}
	}
	else if (wcscmp(buf, L"while") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L'('))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadExpr(str, &tmp_type))
				return;
		}
		if (GetKeywordsReadUntil(str, L','))
			return;
		if (GetKeywordsReadIdentifier(buf, str, True, False))
		{
			GetKeywordsAdd(L"skip");
			return;
		}
	}
	else if (wcscmp(buf, L"if") == 0 || wcscmp(buf, L"switch") == 0)
	{
		if (GetKeywordsReadIdentifier(buf, str, True, False))
			return;
		if (GetKeywordsReadUntil(str, L'('))
			return;
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadExpr(str, &tmp_type))
				return;
		}
	}
}

static const void* ParseSrc(const Char* src_name, const void* ast, void* param)
{
	Bool* end_flag = (Bool*)param;
	if (ast != NULL)
	{
		Srces2 = DictAdd(Srces2, src_name, ast);
		return ast;
	}

	if (!IsCorrectSrcName(src_name, True))
	{
		Err(L"EP0060", DictSearch(SrcRefPos, src_name), src_name);
		return NULL;
	}
	*end_flag = False;

	{
		const Char* true_path;
		if (src_name[0] == L'\\')
			true_path = NewStr(NULL, L"%s%s.kn", Option->SrcDir, src_name + 1);
		else
			true_path = NewStr(NULL, L"%s%s.kn", Option->SysDir, src_name);
		for (; ; )
		{
			FilePtr = FuncWfopen(true_path, L"r, ccs=UTF-8");
			if (FilePtr == NULL)
			{
				Err(L"EP0061", DictSearch(SrcRefPos, src_name), true_path);
				return NULL;
			}
			{
				const Char* reload = NULL;
				const Char* env = L"unknown";
				size_t file_size = FuncSize(FilePtr);
				switch (Option->Env)
				{
					case Env_Wnd: env = L"wnd"; break;
					case Env_Cui: env = L"cui"; break;
				}
				if (file_size == 0)
					reload = NewStr(NULL, L"%spreset00_%s.knp", Option->SysDir, env);
				else if (file_size == 1)
				{
					switch (FuncFgetwc(FilePtr))
					{
						case L'q': reload = NewStr(NULL, L"%spreset01_%s.knp", Option->SysDir, env); break;
						case L'f': reload = NewStr(NULL, L"%spreset02_%s.knp", Option->SysDir, env); break;
						case L'9': reload = NewStr(NULL, L"%spreset03_%s.knp", Option->SysDir, env); break;
					}
				}
				if (reload == NULL)
					break;
				FuncFclose(FilePtr);
				true_path = reload;
			}
		}
	}

	{
		SAstRoot* ast2 = NULL;
		SrcName = src_name;
		Row = 1;
		Col = 0;
		SubSrc = False;
		{
			const Char* sub_src_name = wcschr(src_name, L'.');
			if (sub_src_name != NULL)
			{
				Char parent_src_name[KUIN_MAX_PATH + 1];
				size_t len = sub_src_name - src_name;
				memcpy(parent_src_name, src_name, sizeof(Char) * len);
				parent_src_name[len] = L'\0';
				Srces2 = DictAdd(Srces2, src_name, DummyPtr);
				src_name = parent_src_name;
				ast2 = (SAstRoot*)DictSearch(Srces2, src_name);
				SubSrc = True;
			}
		}
		FileBuf = L'\0';
		FileBufTmp = L'\0';
		IsLast = False;
		Scope = NULL;
		UniqueCnt = 0;
		ast2 = ParseRoot(ast2);
		FuncFclose(FilePtr);
		Srces2 = DictAdd(Srces2, src_name, ast2);
		return ast2;
	}
}

static Char ReadBuf(void)
{
	Char result;
	if (FileBuf == L'\0')
	{
		if (FileBufTmp == L'\0')
		{
			wint_t c = FuncFgetwc(FilePtr);
			result = c == WEOF ? L'\0' : (Char)c;
			if (result == L'\n')
			{
				Row++;
				Col = 0;
			}
			else if (result == L'\0')
			{
				if (!IsLast)
				{
					IsLast = True;
					result = L'\n';
					Row++;
					Col = 0;
				}
			}
			else
				Col++;
		}
		else
		{
			result = FileBufTmp;
			FileBufTmp = L'\0';
		}
	}
	else
	{
		result = FileBuf;
		FileBuf = L'\0';
	}
	return result;
}

static Char Read(void)
{
	for (; ; )
	{
		Char c = ReadBuf();
		switch (c)
		{
			case L'{':
				ReadComment();
				return L' ';
			case L'\r':
				continue;
			case L'\t':
				return L' ';
		}
		return c;
	}
}

static void ReadComment(void)
{
	int row = Row;
	int col = Col;
	Char c;
	do
	{
		c = Read();
		if (c == L'\0')
		{
			Err(L"EP0009", NewPos(SrcName, row, col));
			return;
		}
		if (c == L'"')
		{
			Bool esc = False;
			for (; ; )
			{
				c = ReadStrict();
				if (c == L'\0')
				{
					Err(L"EP0009", NewPos(SrcName, row, col));
					return;
				}
				if (esc)
				{
					if (c == L'{')
						ReadComment();
					esc = False;
					continue;
				}
				if (c == L'"')
					break;
				if (c == L'\\')
					esc = True;
			}
		}
		else if (c == L'\'')
		{
			Bool esc = False;
			for (; ; )
			{
				c = ReadStrict();
				if (c == L'\0')
				{
					Err(L"EP0009", NewPos(SrcName, row, col));
					return;
				}
				if (esc)
				{
					esc = False;
					continue;
				}
				if (c == L'\'')
					break;
				if (c == L'\\')
					esc = True;
			}
		}
		else if (c == L';')
		{
			for (; ; )
			{
				c = ReadBuf();
				if (c == L'\0')
				{
					Err(L"EP0009", NewPos(SrcName, row, col));
					return;
				}
				if (c == L'\n')
					break;
			}
		}
	} while (c != L'}');
}

static Char ReadChar(void)
{
	for (; ; )
	{
		Char c = Read();
		if (c == L'\n')
		{
			c = ReadChar();
			switch (c)
			{
				case L'\n':
					return L'\n';
				case L';':
					do
					{
						c = ReadBuf();
						if (c == L'\0')
							return L'\0';
					} while (c != L'\n');
					FileBuf = c;
					continue;
				case L'|':
					return ReadChar();
			}
			FileBufTmp = c;
			return L'\n';
		}
		if (c != L' ')
			return c;
	}
}

static Char ReadStrict(void)
{
	for (; ; )
	{
		Char c = ReadBuf();
		switch (c)
		{
			case L'\t':
				Err(L"EP0010", NewPos(SrcName, Row, Col));
				continue;
			case L'\r':
				continue;
			case L'\n':
				Err(L"EP0011", NewPos(SrcName, Row, Col));
				break;
		}
		return c;
	}
}

static const Char* ReadIdentifier(Bool skip_spaces, Bool ref)
{
	Char c = skip_spaces ? ReadChar() : Read();
	if (!(L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || ref && (c == L'@' || c == L'\\')))
	{
		FileBuf = c;
		Err(L"EP0000", NewPos(SrcName, Row, Col), CharToStr(c));
		return L"";
	}
	{
		Char buf[128];
		int pos = 0;
		Bool at = False;
		do
		{
			switch (c)
			{
				case L'@':
					if (at)
					{
						FileBuf = c;
						Err(L"EP0001", NewPos(SrcName, Row, Col));
						return L"";
					}
					if (pos != 0)
					{
						const Char* src_name = SubStr(buf, 0, pos);
						{
							const Char* ptr = src_name;
							while (*ptr != L'\0')
							{
								if (L'A' <= *ptr && *ptr <= L'Z')
								{
									FileBuf = c;
									Err(L"EP0002", NewPos(SrcName, Row, Col), src_name);
									return L"";
								}
								ptr++;
							}
						}
						AddSrc(src_name);
					}
					at = True;
					break;
				case L'\\':
					if (at)
					{
						FileBuf = c;
						Err(L"EP0055", NewPos(SrcName, Row, Col));
						return L"";
					}
					break;
			}
			if (pos == 128)
			{
				FileBuf = c;
				buf[127] = L'\0';
				Err(L"EP0003", NewPos(SrcName, Row, Col), buf);
				return L"";
			}
			buf[pos] = c;
			pos++;
			c = Read();
		} while (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || L'0' <= c && c <= L'9' || ref && (c == L'@' || c == L'\\'));
		FileBuf = c;
		return SubStr(buf, 0, pos);
	}
}

static const Char* ReadFuncAttr(void)
{
	Char c = ReadChar();
	if (!(L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_' || c == L'.'))
	{
		Err(L"EP0023", NewPos(SrcName, Row, Col), CharToStr(c));
		return L"";
	}
	{
		Char buf[128];
		int pos = 0;
		do
		{
			if (pos == 128)
			{
				buf[127] = L'\0';
				Err(L"EP0024", NewPos(SrcName, Row, Col), buf);
				return L"";
			}
			buf[pos] = c;
			pos++;
			c = Read();
		} while (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_' || c == L'.');
		FileBuf = c;
		return SubStr(buf, 0, pos);
	}
}

static void ReadUntilRet(Char c)
{
	while (c != L'\n' && c != L'\0')
		c = Read();
	FileBuf = c;
}

static void InitAst(SAst* ast, EAstTypeId type_id, const SPos* pos, const Char* name, Bool set_parent, Bool init_refeds)
{
	ast->TypeId = type_id;
	ast->Pos = pos;
	ast->Name = name;
	ast->ScopeParent = NULL;
	ast->ScopeChildren = NULL;
	ast->ScopeRefedItems = NULL;
	ast->RefName = NULL;
	ast->RefItem = NULL;
	ast->AnalyzedCache = NULL;
	ast->Public = False;

	if (ast->Name != NULL)
		AddScopeName((SAst*)ast, True);
	if (set_parent)
		ast->ScopeParent = (const SAst*)StackPeek(Scope);
	if (init_refeds)
		((SAst*)ast)->ScopeRefedItems = ListNew();
}

static void InitAstExpr(SAstExpr* ast, EAstTypeId type_id, const SPos* pos)
{
	InitAst((SAst*)ast, type_id, pos, NULL, False, False);
	ast->VarKind = AstExprVarKind_Unknown;
	ast->Type = NULL;
}

static void AddScopeName(SAst* ast, Bool refuse_reserved)
{
	ASSERT(ast->Name != NULL);
	if (refuse_reserved && IsReserved(ast->Name))
	{
		Err(L"EP0004", NewPos(SrcName, Row, Col), ast->Name);
		return;
	}
	{
		SAst* scope = ((SAst*)StackPeek(Scope));
		if (DictSearch(scope->ScopeChildren, ast->Name) != NULL)
		{
			Err(L"EP0005", NewPos(SrcName, Row, Col), ast->Name);
			return;
		}
		{
			const SAst* parent = scope;
			Bool over_func = False;
			for (; ; )
			{
				if (parent->ScopeParent == NULL)
					break;
				if (parent->Name != NULL && wcscmp(parent->Name, ast->Name) == 0)
				{
					if (!((parent->TypeId & AstTypeId_Func) == AstTypeId_Func && ((SAst*)parent)->RefName != NULL))
					{
						Err(L"EP0058", NewPos(SrcName, Row, Col), ast->Name);
						return;
					}
				}
				const SAst* child = (const SAst*)DictSearch(parent->ScopeChildren, ast->Name);
				if (child != NULL)
				{
					if (!(over_func && (child->TypeId == AstTypeId_Arg && (((SAstArg*)child)->Kind == AstArgKind_Member || ((SAstArg*)child)->Kind == AstArgKind_LocalVar || ((SAstArg*)child)->Kind == AstArgKind_LocalArg) || (child->TypeId & AstTypeId_StatBreakable) != 0) || (child->TypeId & AstTypeId_Func) == AstTypeId_Func && ((SAst*)child)->RefName != NULL))
					{
						Err(L"EP0058", NewPos(SrcName, Row, Col), ast->Name);
						return;
					}
				}
				if ((parent->TypeId & AstTypeId_Func) == AstTypeId_Func)
					over_func = True;
				parent = parent->ScopeParent;
			}
		}
		scope->ScopeChildren = DictAdd(scope->ScopeChildren, ast->Name, ast);
	}
}

static void AssertNextChar(Char c, Bool skip_spaces)
{
	Char c2 = skip_spaces ? ReadChar() : Read();
	if (c2 != c)
		NextCharErr(c, c2);
}

static void NextCharErr(Char c, Char c2)
{
	if (c == L'\0')
		c = L' ';
	if (c2 == L'\0')
		c2 = L' ';
	Err(L"EP0006", NewPos(SrcName, Row, Col), CharToStr(c), CharToStr(c2));
	ReadUntilRet(c2);
}

static void AddScopeRefeds(SAst* ast)
{
	// 'RefName' added here will be resolved later.
	ListAdd(((SAst*)StackPeek(Scope))->ScopeRefedItems, ast);
}

static void PushDummyScope(SAst* ast)
{
	SAst* dummy = (SAst*)Alloc(sizeof(SAst));
	InitAst(dummy, AstTypeId_Ast, NULL, NULL, True, True);
	((SAst*)StackPeek(Scope))->ScopeChildren = DictAdd(((SAst*)StackPeek(Scope))->ScopeChildren, NewStr(NULL, L"$%u", UniqueCnt), dummy);
	UniqueCnt++;
	Scope = StackPush(Scope, dummy);
	ast->ScopeParent = dummy;
}

static void AddEndPosScope(void)
{
	// This is used for hints on the editor.
	SAst* dummy = (SAst*)Alloc(sizeof(SAst));
	InitAst(dummy, AstTypeId_Ast, NewPos(SrcName, Row - 1, 0), NULL, True, True);
	((SAst*)StackPeek(Scope))->ScopeChildren = DictAdd(((SAst*)StackPeek(Scope))->ScopeChildren, NewStr(NULL, L"$%u", UniqueCnt), dummy);
	UniqueCnt++;
	Scope = StackPush(Scope, dummy);
	Scope = StackPop(Scope);
}

static SAstArg* MakeBlockVar(int row, int col)
{
	SAstArg* result = (SAstArg*)Alloc(sizeof(SAstArg));
	InitAst((SAst*)result, AstTypeId_Arg, NewPos(SrcName, row, col), NULL, False, False);
	result->Addr = NewAddr();
	result->Kind = AstArgKind_LocalVar;
	result->RefVar = False;
	result->Type = NULL;
	result->Expr = NULL;
	return result;
}

static void ObtainBlockName(SAst* ast)
{
	Char c = ReadChar();
	if (c != L'(')
	{
		FileBuf = c;
		((SAst*)ast)->Name = ReadIdentifier(True, False);
		AddScopeName((SAst*)ast, True);
		AssertNextChar(L'(', True);
	}
	else
		((SAst*)ast)->Name = L"$";
}

static SAstExprValue* ObtainPrimValue(const SPos* pos, EAstTypePrimKind kind, const void* value)
{
	SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
	InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, pos);
	{
		SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
		InitAst((SAst*)type, AstTypeId_TypePrim, pos, NULL, False, False);
		type->Kind = kind;
		((SAstExpr*)expr)->Type = (SAstType*)type;
	}
	memcpy(expr->Value, value, 8);
	return expr;
}

static SAstExprValue* ObtainStrValue(const SPos* pos, const Char* value)
{
	SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
	InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, pos);
	{
		SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
		InitAst((SAst*)type, AstTypeId_TypeArray, pos, NULL, False, False);
		{
			SAstTypePrim* type2 = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
			InitAst((SAst*)type2, AstTypeId_TypePrim, pos, NULL, False, False);
			type2->Kind = AstTypePrimKind_Char;
			type->ItemType = (SAstType*)type2;
		}
		((SAstExpr*)expr)->Type = (SAstType*)type;
	}
	*(const Char**)expr->Value = value;
	return expr;
}

static Char EscChar(Char c)
{
	switch (c)
	{
		case L'"': return L'"';
		case L'\'': return L'\'';
		case L'\\': return L'\\';
		case L'0': return L'\0';
		case L'n': return L'\n';
		case L't': return L'\t';
		case L'u':
			{
				Char buf[5];
				int i;
				for (i = 0; i < 4; i++)
				{
					c = ReadStrict();
					if (!(L'0' <= c && c <= L'9' || L'A' <= c && c <= L'F'))
					{
						Err(L"EP0057", NewPos(SrcName, Row, Col));
						return 'u';
					}
					buf[i] = c;
				}
				buf[4] = L'\0';
				{
					Char* end_ptr;
					U32 value;
					errno = 0;
					value = wcstoul(buf, &end_ptr, 16);
					ASSERT(*end_ptr == L'\0' && errno != ERANGE);
					return (Char)value;
				}
			}
		default:
			Err(L"EP0007", NewPos(SrcName, Row, Col), CharToStr(c));
			return c;
	}
}

static Bool IsCorrectSrcName(const Char* name, Bool skip_dot)
{
	if (*name == L'\\')
		name++;
	for (; ; )
	{
		if (!(L'a' <= name[0] && name[0] <= L'z' || name[0] == L'_'))
			return False;
		for (; ; )
		{
			name++;
			if (L'a' <= name[0] && name[0] <= L'z' || name[0] == L'_' || L'0' <= name[0] && name[0] <= L'9' || skip_dot && name[0] == L'.')
				continue;
			if (*name == L'\\')
				break;
			if (*name == L'\0')
				return True;
			return False;
		}
		name++;
	}
}

static void AddSrc(const Char* name)
{
	if (DictSearch(Srces2, name) == NULL)
		Srces2 = DictAdd(Srces2, name, NULL);
	if (DictSearch(SrcRefPos, name) == NULL)
		SrcRefPos = DictAdd(SrcRefPos, name, NewPos(SrcName, Row, Col));
}

static Bool IsReserved(const Char* word)
{
	return BinSearch(Reserved, (int)(sizeof(Reserved) / sizeof(Char*)), word) != -1;
}

static SAstStatBlock* ParseDummyBlock(SAstStat** out_stat, EAstTypeId* out_type_id, EAstTypeId type_id, SAst* block)
{
	SAstStatBlock* ast = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
	InitAst((SAst*)ast, AstTypeId_StatBlock, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = NULL;
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	((SAst*)ast)->Name = L"$";
	for (; ; )
	{
		SAstStat* stat = ParseStat(block);
		if (stat == NULL)
		{
			if (IsLast)
			{
				*out_stat = NULL;
				*out_type_id = AstTypeId_StatEnd;
				break;
			}
			else
				continue;
		}
		Bool end_flag = False;
		switch (type_id)
		{
			case AstTypeId_StatIf:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatElIf:
					case AstTypeId_StatElse:
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatElIf:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatElIf:
					case AstTypeId_StatElse:
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatElse:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatElIf:
					case AstTypeId_StatElse:
						Err(L"EP0041", NewPos(SrcName, Row, Col));
						continue;
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatCase:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatCase:
					case AstTypeId_StatDefault:
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatDefault:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatCase:
					case AstTypeId_StatDefault:
						Err(L"EP0045", NewPos(SrcName, Row, Col));
						continue;
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatTry:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatCatch:
					case AstTypeId_StatFinally:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatCatch:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatCatch:
					case AstTypeId_StatFinally:
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			case AstTypeId_StatFinally:
				switch (((SAst*)stat)->TypeId)
				{
					case AstTypeId_StatCatch:
					case AstTypeId_StatFinally:
						Err(L"EP0050", NewPos(SrcName, Row, Col));
						continue;
					case AstTypeId_StatEnd:
						end_flag = True;
						break;
				}
				break;
			default:
				ASSERT(False);
				break;
		}
		if (end_flag)
		{
			*out_stat = stat;
			*out_type_id = ((SAst*)stat)->TypeId;
			break;
		}
		ListAdd(ast->Stats, stat);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return ast;
}

static SAstRoot* ParseRoot(SAstRoot* ast)
{
	if (ast == NULL)
	{
		ast = (SAstRoot*)Alloc(sizeof(SAstRoot));
		InitAst((SAst*)ast, AstTypeId_Root, NewPos(SrcName, 1, 1), NULL, False, True);
		ast->Items = ListNew();
	}
	Scope = StackPush(Scope, ast);
	{
		// For the case where there is a single line comment at the beginning of source codes.
		FileBuf = L'\n';
		FileBuf = ReadChar();
	}
	for (; ; )
	{
		Char c = ReadChar();
		if (c == L'\0')
			break;
		if (c == L'\n')
			continue;
		{
			SAst* child;
			Bool child_public = False;
			if (c == L'+')
				child_public = True;
			else
				FileBuf = c;
			{
				int row = Row;
				int col = Col;
				const Char* id = ReadIdentifier(True, False);
				if (wcscmp(id, L"func") == 0)
					child = (SAst*)ParseFunc(NULL, False);
				else if (wcscmp(id, L"var") == 0)
					child = (SAst*)ParseVar(AstArgKind_Global, NULL);
				else if (wcscmp(id, L"const") == 0)
					child = (SAst*)ParseConst();
				else if (wcscmp(id, L"alias") == 0)
					child = (SAst*)ParseAlias();
				else if (wcscmp(id, L"include") == 0)
				{
					ParseInclude();
					continue;
				}
				else if (wcscmp(id, L"class") == 0)
					child = (SAst*)ParseClass();
				else if (wcscmp(id, L"enum") == 0)
					child = (SAst*)ParseEnum();
				else
				{
					Err(L"EP0025", NewPos(SrcName, row, col), id);
					ReadUntilRet(Read());
					continue;
				}
			}
			if (child == (SAst*)DummyPtr)
				break;
			if (((SAst*)child)->TypeId == AstTypeId_Var)
				((SAst*)((SAstVar*)child)->Var)->Public = child_public;
			else if (((SAst*)child)->TypeId == AstTypeId_Const)
				((SAst*)((SAstConst*)child)->Var)->Public = child_public;
			else
				child->Public = child_public;
			ListAdd(ast->Items, child);
		}
	}
	AddEndPosScope();
	Scope = StackPop(Scope);
	return ast;
}

static SAstFunc* ParseFunc(const Char* parent_class, Bool overridden)
{
	SAstFunc* ast = (SAstFunc*)Alloc(sizeof(SAstFunc));
	ast->DllName = NULL;
	ast->DllFuncName = NULL;
	ast->FuncAttr = FuncAttr_None;
	{
		Char c = ReadChar();
		if (c == L'[')
		{
			for (; ; )
			{
				int row = Row;
				int col = Col;
				const Char* func_attr = ReadFuncAttr();
				if (wcscmp(func_attr, L"callback") == 0 && (ast->FuncAttr & FuncAttr_Callback) == 0)
					ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_Callback);
				else if (func_attr[0] == L'_')
				{
					const Char* func_attr2 = func_attr + 1;
					if (wcscmp(func_attr2, L"any_type") == 0 && (ast->FuncAttr & FuncAttr_AnyType) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_AnyType);
					else if (wcscmp(func_attr2, L"init") == 0 && (ast->FuncAttr & FuncAttr_Init) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_Init);
					else if (wcscmp(func_attr2, L"take_me") == 0 && (ast->FuncAttr & FuncAttr_TakeMe) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeMe);
					else if (wcscmp(func_attr2, L"ret_me") == 0 && (ast->FuncAttr & FuncAttr_RetMe) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetMe);
					else if (wcscmp(func_attr2, L"take_child") == 0 && (ast->FuncAttr & FuncAttr_TakeChild) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeChild);
					else if (wcscmp(func_attr2, L"ret_child") == 0 && (ast->FuncAttr & FuncAttr_RetChild) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetChild);
					else if (wcscmp(func_attr2, L"take_key_value") == 0 && (ast->FuncAttr & FuncAttr_TakeKeyValue) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeKeyValue);
					else if (wcscmp(func_attr2, L"ret_array_of_list_child") == 0 && (ast->FuncAttr & FuncAttr_RetArrayOfListChild) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetArrayOfListChild);
					else if (wcscmp(func_attr2, L"make_instance") == 0 && (ast->FuncAttr & FuncAttr_MakeInstance) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_MakeInstance);
					else if (wcscmp(func_attr2, L"force") == 0 && (ast->FuncAttr & FuncAttr_Force) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_Force);
					else if (wcscmp(func_attr2, L"exit_code") == 0 && (ast->FuncAttr & FuncAttr_ExitCode) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_ExitCode);
					else if (wcscmp(func_attr2, L"take_key_value_func") == 0 && (ast->FuncAttr & FuncAttr_TakeKeyValueFunc) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeKeyValueFunc);
					else if (wcscmp(func_attr2, L"ret_array_of_dict_key") == 0 && (ast->FuncAttr & FuncAttr_RetArrayOfDictKey) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetArrayOfDictKey);
					else if (wcscmp(func_attr2, L"ret_array_of_dict_value") == 0 && (ast->FuncAttr & FuncAttr_RetArrayOfDictValue) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetArrayOfDictValue);
					else if (L'0' <= func_attr2[0] && func_attr2[0] <= L'9')
					{
						Char* ptr;
						S64 value = (S64)wcstoll(func_attr2, &ptr, 10);
						S64 value2 = (value - 1) / 8;
						if (*ptr == L'\0' && 1 <= value && value2 < USE_RES_FLAGS_LEN)
							MakeUseResFlags[value2] |= 1 << ((value - 1) % 8);
					}
					else if (ast->DllName == NULL)
						ast->DllName = func_attr;
					else if (ast->DllFuncName == NULL)
						ast->DllFuncName = func_attr;
					else
						Err(L"EP0026", NewPos(SrcName, row, col), func_attr);
				}
				else if (ast->DllName == NULL)
					ast->DllName = func_attr;
				else if (ast->DllFuncName == NULL)
					ast->DllFuncName = func_attr;
				else
					Err(L"EP0026", NewPos(SrcName, row, col), func_attr);
				c = ReadChar();
				if (c == L'\0')
					break;
				if (c == L']')
					break;
				if (c != L',')
				{
					NextCharErr(L',', c);
					break;
				}
			}
		}
		else
			FileBuf = c;
	}
	InitAst((SAst*)ast, AstTypeId_Func, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, True);
	ast->AddrTop = NewAddr();
	ast->AddrBottom = -1;
	ast->Args = ListNew();
	ast->Ret = NULL;
	ast->Stats = ListNew();
	ast->RetPoint = NULL;
	Scope = StackPush(Scope, ast);
	if (parent_class != NULL)
	{
		((SAst*)ast)->RefName = parent_class;
		AddScopeRefeds((SAst*)ast);
	}
	AssertNextChar(L'(', True);
	if (parent_class != NULL)
	{
		SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
		InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos, NULL, False, False);
		((SAst*)arg)->Name = L"me";
		arg->Addr = NewAddr();
		arg->Expr = NULL;
		arg->Kind = AstArgKind_LocalArg;
		arg->RefVar = False;
		AddScopeName((SAst*)arg, False);
		{
			SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
			InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos, NULL, False, False);
			((SAst*)type)->RefName = parent_class;
			AddScopeRefeds((SAst*)type);
			arg->Type = (SAstType*)type;
		}
		ListAdd(ast->Args, arg);
	}
	{
		Char c = ReadChar();
		if (c != L')')
		{
			FileBuf = c;
			for (; ; )
			{
				ListAdd(ast->Args, ParseArg(AstArgKind_LocalArg, NULL));
				c = ReadChar();
				if (c == L'\0')
					break;
				if (c == L')')
					break;
				if (c != L',')
				{
					NextCharErr(L',', c);
					break;
				}
			}
		}
	}
	{
		Char c = ReadChar();
		if (c == L':')
		{
			ast->Ret = ParseType();
			c = ReadChar();
		}
		if (c != L'\n')
			NextCharErr(L'\n', c);
	}
	if (overridden)
	{
		SAstStatVar* stat_var = (SAstStatVar*)Alloc(sizeof(SAstStatVar));
		{
			InitAst((SAst*)stat_var, AstTypeId_StatVar, ((SAst*)ast)->Pos, NULL, False, False);
			((SAstStat*)stat_var)->AsmTop = NULL;
			((SAstStat*)stat_var)->AsmBottom = NULL;
			((SAstStat*)stat_var)->PosRowBottom = -1;
			SAstVar* var = (SAstVar*)Alloc(sizeof(SAstVar));
			{
				InitAst((SAst*)var, AstTypeId_Var, ((SAst*)ast)->Pos, NULL, False, False);
				SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
				{
					InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos, NULL, False, False);
					((SAst*)arg)->Name = L"super";
					arg->Addr = NewAddr();
					arg->Expr = NULL;
					arg->Kind = AstArgKind_LocalArg;
					arg->RefVar = False;
					AddScopeName((SAst*)arg, False);
					{
						SAstTypeFunc* type = (SAstTypeFunc*)Alloc(sizeof(SAstTypeFunc));
						InitAst((SAst*)type, AstTypeId_TypeFunc, ((SAst*)ast)->Pos, NULL, False, False);
						type->FuncAttr = ast->FuncAttr;
						type->Args = ListNew();
						{
							SListNode* ptr = ast->Args->Top;
							while (ptr != NULL)
							{
								SAstTypeFuncArg* arg2 = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
								arg2->RefVar = ((SAstArg*)ptr->Data)->RefVar;
								arg2->Arg = ((SAstArg*)ptr->Data)->Type;
								ListAdd(type->Args, arg2);
								ptr = ptr->Next;
							}
						}
						type->Ret = ast->Ret;
						arg->Type = (SAstType*)type;
					}
				}
				var->Var = arg;
			}
			stat_var->Def = var;
		}
		ListAdd(ast->Stats, stat_var);
	}
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == NULL)
		{
			if (IsLast)
				break;
			else
				continue;
		}
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	ast->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return ast;
}

static SAstVar* ParseVar(EAstArgKind kind, const Char* parent_class)
{
	SAstVar* ast = (SAstVar*)Alloc(sizeof(SAstVar));
	InitAst((SAst*)ast, AstTypeId_Var, NewPos(SrcName, Row, Col), NULL, False, False);
	ast->Var = ParseArg(kind, parent_class);
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstConst* ParseConst(void)
{
	SAstConst* ast = (SAstConst*)Alloc(sizeof(SAstConst));
	InitAst((SAst*)ast, AstTypeId_Const, NewPos(SrcName, Row, Col), NULL, False, False);
	ast->Var = ParseArg(AstArgKind_Const, NULL);
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstAlias* ParseAlias(void)
{
	SAstAlias* ast = (SAstAlias*)Alloc(sizeof(SAstAlias));
	InitAst((SAst*)ast, AstTypeId_Alias, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, False);
	AssertNextChar(L':', True);
	ast->Type = ParseType();
	AssertNextChar(L'\n', True);
	return ast;
}

static void ParseInclude(void)
{
	const Char* sub_src_name = ReadIdentifier(True, False);
	if (SubSrc)
	{
		Err(L"EP0059", NewPos(SrcName, Row, Col));
		return;
	}
	const Char* new_src_name = NewStr(NULL, L"%s.%s", SrcName, sub_src_name);
	AssertNextChar(L'\n', True);
	AddSrc(new_src_name);
}

static SAstClass* ParseClass(void)
{
	SAstClass* ast = (SAstClass*)Alloc(sizeof(SAstClass));
	InitAst((SAst*)ast, AstTypeId_Class, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, True);
	ast->Addr = NewAddr();
	ast->VarSize = 0;
	ast->FuncSize = 0;
	ast->Items = ListNew();
	ast->IndirectCreation = False;
	AssertNextChar(L'(', True);
	{
		Char c = ReadChar();
		if (c != L')')
		{
			FileBuf = c;
			((SAst*)ast)->RefName = ReadIdentifier(True, True);
			AssertNextChar(L')', True);
		}
		else
		{
			Bool kuin_src = wcscmp(SrcName, L"kuin") == 0;
			if (!(kuin_src && wcscmp(((SAst*)ast)->Name, L"Class") == 0))
				((SAst*)ast)->RefName = kuin_src ? L"@Class" : L"kuin@Class";
		}
		if (((SAst*)ast)->RefName != NULL)
			AddScopeRefeds((SAst*)ast);
	}
	AssertNextChar(L'\n', True);
	Scope = StackPush(Scope, ast);
	for (; ; )
	{
		Char c = ReadChar();
		if (c == L'\0')
		{
			Err(L"EP0027", NewPos(SrcName, Row, Col));
			break;
		}
		if (c == L'\n')
			continue;
		{
			int row = Row;
			int col = Col;
			SAstClassItem* item = (SAstClassItem*)Alloc(sizeof(SAstClassItem));
			item->Public = False;
			item->Override = False;
			item->ParentItem = NULL;
			item->Addr = -1;
			if (c == L'+')
				item->Public = True;
			else
				FileBuf = c;
			c = ReadChar();
			if (c == L'*')
				item->Override = True;
			else
				FileBuf = c;
			{
				const Char* s = ReadIdentifier(True, False);
				const Char* class_name = ((SAst*)ast)->ScopeParent->TypeId == AstTypeId_Root ? NewStr(NULL, L"@%s", ((SAst*)ast)->Name) : ((SAst*)ast)->Name;
				if (wcscmp(s, L"func") == 0)
					item->Def = (SAst*)ParseFunc(class_name, item->Override);
				else if (wcscmp(s, L"var") == 0)
				{
					if (item->Override)
						Err(L"EP0028", NewPos(SrcName, row, col), s);
					item->Def = (SAst*)ParseVar(AstArgKind_Member, class_name);
				}
				else
				{
					if (item->Public)
						Err(L"EP0029", NewPos(SrcName, row, col), s);
					if (item->Override)
						Err(L"EP0028", NewPos(SrcName, row, col), s);
					if (wcscmp(s, L"end") == 0)
					{
						const Char* s2 = ReadIdentifier(True, False);
						if (wcscmp(s2, L"class") != 0)
							Err(L"EP0030", NewPos(SrcName, row, col), s2);
						{
							SAst* ast2 = (SAst*)Alloc(sizeof(SAst)); // 'end class'.
							InitAst((SAst*)ast2, AstTypeId_Ast, NewPos(SrcName, row, col), NULL, False, False);
						}
						AssertNextChar(L'\n', True);
						break;
					}
					else if (wcscmp(s, L"const") == 0)
						item->Def = (SAst*)ParseConst();
					else if (wcscmp(s, L"alias") == 0)
						item->Def = (SAst*)ParseAlias();
					else if (wcscmp(s, L"class") == 0)
						item->Def = (SAst*)ParseClass();
					else if (wcscmp(s, L"enum") == 0)
						item->Def = (SAst*)ParseEnum();
					else
					{
						Err(L"EP0031", NewPos(SrcName, row, col), s);
						ReadUntilRet(Read());
						continue;
					}
				}
			}
			ListAdd(ast->Items, item);
		}
	}
	AddEndPosScope();
	Scope = StackPop(Scope);
	return ast;
}

static SAstEnum* ParseEnum(void)
{
	SAstEnum* ast = (SAstEnum*)Alloc(sizeof(SAstEnum));
	InitAst((SAst*)ast, AstTypeId_Enum, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, True);
	ast->Items = ListNew();
	AssertNextChar(L'\n', True);
	Scope = StackPush(Scope, ast);
	for (; ; )
	{
		Char c = ReadChar();
		if (c == L'\0')
		{
			Err(L"EP0032", NewPos(SrcName, Row, Col));
			break;
		}
		if (c == L'\n')
			continue;
		FileBuf = c;
		{
			SAstExpr* item;
			const Char* s = ReadIdentifier(True, False);
			if (wcscmp(s, L"end") == 0)
			{
				const Char* s2 = ReadIdentifier(True, False);
				if (wcscmp(s2, L"enum") != 0)
					Err(L"EP0033", NewPos(SrcName, Row, Col), s2);
				AssertNextChar(L'\n', True);
				break;
			}
			c = ReadChar();
			if (c == L':')
			{
				AssertNextChar(L':', False);
				item = ParseExpr();
				if (item == NULL)
				{
					ReadUntilRet(Read());
					continue;
				}
				c = ReadChar();
			}
			else
			{
				SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
				InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
				((SAstExpr*)expr)->Type = NULL; // Set the type to 'NULL' when no value is specified.
				memset(expr->Value, 0x00, 8);
				item = (SAstExpr*)expr;
			}
			((SAst*)item)->Name = s;
			if (c != L'\n')
				NextCharErr(L'\n', c);
			ListAdd(ast->Items, item);
			AddScopeName((SAst*)item, True);
		}
	}
	AddEndPosScope();
	Scope = StackPop(Scope);
	return ast;
}

static SAstArg* ParseArg(EAstArgKind kind, const Char* parent_class)
{
	SAstArg* ast = (SAstArg*)Alloc(sizeof(SAstArg));
	InitAst((SAst*)ast, AstTypeId_Arg, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), False, False);
	ast->Addr = NewAddr();
	ast->Kind = kind;
	ast->RefVar = False;
	if (parent_class != NULL)
	{
		((SAst*)ast)->RefName = parent_class;
		AddScopeRefeds((SAst*)ast);
	}
	AssertNextChar(L':', True);
	{
		Char c = ReadChar();
		if (c == L'&')
		{
			if (kind != AstArgKind_LocalArg)
				Err(L"EP0034", NewPos(SrcName, Row, Col));
			else
				ast->RefVar = True;
		}
		else
			FileBuf = c;
	}
	ast->Type = ParseType();
	{
		Char c = ReadChar();
		if (c == L':')
		{
			AssertNextChar(L':', False);
			ASSERT(kind != AstArgKind_Unknown);
			if (kind == AstArgKind_LocalArg)
			{
				Err(L"EP0035", NewPos(SrcName, Row, Col));
				ast->Expr = NULL;
			}
			else if (kind == AstArgKind_Member)
			{
				Err(L"EP0036", NewPos(SrcName, Row, Col));
				ast->Expr = NULL;
			}
			else
				ast->Expr = ParseExpr();
		}
		else
		{
			if (kind == AstArgKind_Const)
				Err(L"EP0037", NewPos(SrcName, Row, Col));
			ast->Expr = NULL;
			FileBuf = c;
		}
	}
	return ast;
}

static SAstStat* ParseStat(SAst* block)
{
	SAstStat* ast;
	{
		Char c;
		do
		{
			c = ReadChar();
			if (c == L'\0')
			{
				Err(L"EP0038", NewPos(SrcName, Row, Col));
				return NULL;
			}
		} while (c == L'\n');
		FileBuf = c;
	}
	{
		int row = Row;
		int col = Col;
		const Char* s = ReadIdentifier(True, False);
		if (wcscmp(s, L"end") == 0)
			ast = ParseStatEnd(row, col, block);
		else if (wcscmp(s, L"func") == 0)
			ast = ParseStatFunc();
		else if (wcscmp(s, L"var") == 0)
			ast = ParseStatVar();
		else if (wcscmp(s, L"const") == 0)
			ast = ParseStatConst();
		else if (wcscmp(s, L"alias") == 0)
			ast = ParseStatAlias();
		else if (wcscmp(s, L"class") == 0)
			ast = ParseStatClass();
		else if (wcscmp(s, L"enum") == 0)
			ast = ParseStatEnum();
		else if (wcscmp(s, L"if") == 0)
			ast = ParseStatIf();
		else if (wcscmp(s, L"elif") == 0)
			ast = ParseStatElIf(row, col, block);
		else if (wcscmp(s, L"else") == 0)
			ast = ParseStatElse(row, col, block);
		else if (wcscmp(s, L"switch") == 0)
			ast = ParseStatSwitch(row, col);
		else if (wcscmp(s, L"case") == 0)
			ast = ParseStatCase(row, col, block);
		else if (wcscmp(s, L"default") == 0)
			ast = ParseStatDefault(row, col, block);
		else if (wcscmp(s, L"while") == 0)
			ast = ParseStatWhile();
		else if (wcscmp(s, L"for") == 0)
			ast = ParseStatFor(row, col);
		else if (wcscmp(s, L"try") == 0)
			ast = ParseStatTry(row, col);
		else if (wcscmp(s, L"catch") == 0)
			ast = ParseStatCatch(row, col, block);
		else if (wcscmp(s, L"finally") == 0)
			ast = ParseStatFinally(row, col, block);
		else if (wcscmp(s, L"throw") == 0)
			ast = ParseStatThrow();
		else if (wcscmp(s, L"block") == 0)
			ast = ParseStatBlock();
		else if (wcscmp(s, L"ret") == 0)
			ast = ParseStatRet();
		else if (wcscmp(s, L"do") == 0)
			ast = ParseStatDo();
		else if (wcscmp(s, L"break") == 0)
			ast = ParseStatBreak();
		else if (wcscmp(s, L"skip") == 0)
			ast = ParseStatSkip();
		else if (wcscmp(s, L"assert") == 0)
			ast = ParseStatAssert();
		else
		{
			Err(L"EP0025", NewPos(SrcName, row, col), s);
			ReadUntilRet(Read());
			return NULL;
		}
		if (ast == NULL)
			return NULL;
		((SAst*)ast)->Pos = NewPos(SrcName, row, col);
	}
	return ast;
}

static SAstStat* ParseStatEnd(int row, int col, SAst* block)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatEnd, NewPos(SrcName, row, col), NULL, False, False);
	ast->AsmTop = NULL;
	ast->AsmBottom = NULL;
	ast->PosRowBottom = row;
	{
		const Char* s = ReadIdentifier(True, False);
		Bool err = False;
		AssertNextChar(L'\n', True);
		if (wcscmp(s, L"func") == 0)
		{
			if (block->TypeId != AstTypeId_Func)
				err = True;
		}
		else if (wcscmp(s, L"if") == 0)
		{
			if (block->TypeId != AstTypeId_StatIf)
				err = True;
		}
		else if (wcscmp(s, L"switch") == 0)
		{
			if (block->TypeId != AstTypeId_StatSwitch)
				err = True;
		}
		else if (wcscmp(s, L"while") == 0)
		{
			if (block->TypeId != AstTypeId_StatWhile)
				err = True;
		}
		else if (wcscmp(s, L"for") == 0)
		{
			if (block->TypeId != AstTypeId_StatFor)
				err = True;
		}
		else if (wcscmp(s, L"try") == 0)
		{
			if (block->TypeId != AstTypeId_StatTry)
				err = True;
		}
		else if (wcscmp(s, L"block") == 0)
		{
			if (block->TypeId != AstTypeId_StatBlock)
				err = True;
		}
		else
			Err(L"EP0039", NewPos(SrcName, row, col), s);
		if (err)
			Err(L"EP0040", NewPos(SrcName, row, col), s);
	}
	return ast;
}

static SAstStat* ParseStatFunc(void)
{
	SAstStatFunc* ast = (SAstStatFunc*)Alloc(sizeof(SAstStatFunc));
	InitAst((SAst*)ast, AstTypeId_StatFunc, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Def = ParseFunc(NULL, False);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatVar(void)
{
	SAstStatVar* ast = (SAstStatVar*)Alloc(sizeof(SAstStatVar));
	InitAst((SAst*)ast, AstTypeId_StatVar, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Def = ParseVar(AstArgKind_LocalVar, NULL);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatConst(void)
{
	SAstStatConst* ast = (SAstStatConst*)Alloc(sizeof(SAstStatConst));
	InitAst((SAst*)ast, AstTypeId_StatConst, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Def = ParseConst();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatAlias(void)
{
	SAstStatAlias* ast = (SAstStatAlias*)Alloc(sizeof(SAstStatAlias));
	InitAst((SAst*)ast, AstTypeId_StatAlias, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Def = ParseAlias();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatClass(void)
{
	SAstStatClass* ast = (SAstStatClass*)Alloc(sizeof(SAstStatClass));
	InitAst((SAst*)ast, AstTypeId_StatClass, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Def = ParseClass();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatEnum(void)
{
	SAstStatEnum* ast = (SAstStatEnum*)Alloc(sizeof(SAstStatEnum));
	InitAst((SAst*)ast, AstTypeId_StatEnum, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Def = ParseEnum();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatIf(void)
{
	SAstStatIf* ast = (SAstStatIf*)Alloc(sizeof(SAstStatIf));
	InitAst((SAst*)ast, AstTypeId_StatIf, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->StatBlock = NULL;
	ast->ElIfs = ListNew();
	ast->ElseStatBlock = NULL;
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	AssertNextChar(L')', True);
	AssertNextChar(L'\n', True);
	{
		SAstStat* stat;
		EAstTypeId type_id;
		ast->StatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatIf, (SAst*)ast);
		while (type_id == AstTypeId_StatElIf)
		{
			SAstStatElIf* elif = (SAstStatElIf*)stat;
			elif->StatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatElIf, (SAst*)ast);
			ListAdd(ast->ElIfs, elif);
		}
		while (type_id == AstTypeId_StatElse)
			ast->ElseStatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatElse, (SAst*)ast);
		ASSERT(type_id == AstTypeId_StatEnd);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatElIf(int row, int col, const SAst* block)
{
	SAstStatElIf* ast = (SAstStatElIf*)Alloc(sizeof(SAstStatElIf));
	InitAst((SAst*)ast, AstTypeId_StatElIf, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->StatBlock = NULL;
	if (block->TypeId != AstTypeId_StatIf)
	{
		Err(L"EP0042", NewPos(SrcName, row, col));
		ReadUntilRet(Read());
		return NULL;
	}
	AssertNextChar(L'(', True);
	ast->Cond = ParseExpr();
	AssertNextChar(L')', True);
	AssertNextChar(L'\n', True);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatElse(int row, int col, const SAst* block)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatElse, NULL, NULL, False, False);
	ast->AsmTop = NULL;
	ast->AsmBottom = NULL;
	ast->PosRowBottom = -1;
	if (block->TypeId != AstTypeId_StatIf)
	{
		Err(L"EP0043", NewPos(SrcName, row, col));
		ReadUntilRet(Read());
		return NULL;
	}
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatSwitch(int row, int col)
{
	SAstStatSwitch* ast = (SAstStatSwitch*)Alloc(sizeof(SAstStatSwitch));
	InitAst((SAst*)ast, AstTypeId_StatSwitch, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->Cases = ListNew();
	ast->DefaultStatBlock = NULL;
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	AssertNextChar(L')', True);
	AssertNextChar(L'\n', True);
	{
		SAstStat* stat;
		EAstTypeId type_id;
		for (; ; )
		{
			stat = ParseStat((SAst*)ast);
			if (stat == NULL)
			{
				if (IsLast)
					return NULL;
				else
					continue;
			}
			break;
		}
		type_id = ((SAst*)stat)->TypeId;
		if (!(type_id == AstTypeId_StatCase || type_id == AstTypeId_StatDefault || type_id == AstTypeId_StatEnd))
			Err(L"EP0044", NewPos(SrcName, Row, Col));
		while (type_id == AstTypeId_StatCase)
		{
			SAstStatCase* case_ = (SAstStatCase*)stat;
			case_->StatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatCase, (SAst*)ast);
			ListAdd(ast->Cases, case_);
		}
		while (type_id == AstTypeId_StatDefault)
			ast->DefaultStatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatDefault, (SAst*)ast);
		ASSERT(type_id == AstTypeId_StatEnd);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatCase(int row, int col, const SAst* block)
{
	SAstStatCase* ast = (SAstStatCase*)Alloc(sizeof(SAstStatCase));
	InitAst((SAst*)ast, AstTypeId_StatCase, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Conds = ListNew();
	ast->StatBlock = NULL;
	if (block->TypeId != AstTypeId_StatSwitch)
	{
		Err(L"EP0046", NewPos(SrcName, row, col));
		ReadUntilRet(Read());
		return NULL;
	}
	for (; ; )
	{
		Char c;
		SAstExpr** exprs = (SAstExpr**)Alloc(sizeof(SAstExpr*) * 2);
		exprs[0] = ParseExpr();
		exprs[1] = NULL;
		c = ReadChar();
		if (c == L'\0')
			break;
		if (c == L'\n')
		{
			ListAdd(ast->Conds, exprs);
			break;
		}
		if (c == L',')
		{
			ListAdd(ast->Conds, exprs);
			continue;
		}
		FileBuf = c;
		{
			const Char* s = ReadIdentifier(True, False);
			if (wcscmp(s, L"to") != 0)
			{
				Err(L"EP0047", NewPos(SrcName, Row, Col), s);
				ReadUntilRet(Read());
				return NULL;
			}
		}
		exprs[1] = ParseExpr();
		c = ReadChar();
		if (c == L'\0')
			break;
		ListAdd(ast->Conds, exprs);
		if (c == L'\n')
			break;
		if (c != L',')
			NextCharErr(L',', c);
	}
	return (SAstStat*)ast;
}

static SAstStat* ParseStatDefault(int row, int col, const SAst* block)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatDefault, NULL, NULL, False, False);
	ast->AsmTop = NULL;
	ast->AsmBottom = NULL;
	ast->PosRowBottom = -1;
	if (block->TypeId != AstTypeId_StatSwitch)
	{
		Err(L"EP0048", NewPos(SrcName, row, col));
		ReadUntilRet(Read());
		return NULL;
	}
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatWhile(void)
{
	SAstStatWhile* ast = (SAstStatWhile*)Alloc(sizeof(SAstStatWhile));
	InitAst((SAst*)ast, AstTypeId_StatWhile, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	((SAstStatSkipable*)ast)->SkipPoint = AsmLabel();
	ast->Skip = False;
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	{
		Char c = ReadChar();
		if (c == L',')
		{
			const Char* s = ReadIdentifier(True, False);
			if (wcscmp(s, L"skip") != 0)
				Err(L"EP0049", NewPos(SrcName, Row, Col), s);
			ast->Skip = True;
		}
		else
			FileBuf = c;
		AssertNextChar(L')', True);
		AssertNextChar(L'\n', True);
	}
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == NULL)
		{
			if (IsLast)
				return NULL;
			else
				continue;
		}
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatFor(int row, int col)
{
	SAstStatFor* ast = (SAstStatFor*)Alloc(sizeof(SAstStatFor));
	InitAst((SAst*)ast, AstTypeId_StatFor, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	((SAstStatSkipable*)ast)->SkipPoint = AsmLabel();
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Start = ParseExpr();
	{
		AssertNextChar(L',', True);
		ast->Cond = ParseExpr();
		{
			Char c = ReadChar();
			if (c == L',')
			{
				ast->Step = ParseExpr();
				AssertNextChar(L')', True);
				AssertNextChar(L'\n', True);
			}
			else
			{
				U64 value = 1;
				ast->Step = (SAstExpr*)ObtainPrimValue(NewPos(SrcName, Row, Col), AstTypePrimKind_Int, &value);
				FileBuf = c;
				AssertNextChar(L')', True);
				AssertNextChar(L'\n', True);
			}
		}
	}
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == NULL)
		{
			if (IsLast)
				return NULL;
			else
				continue;
		}
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatTry(int row, int col)
{
	SAstStatTry* ast = (SAstStatTry*)Alloc(sizeof(SAstStatTry));
	InitAst((SAst*)ast, AstTypeId_StatTry, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->StatBlock = NULL;
	ast->Catches = ListNew();
	ast->FinallyStatBlock = NULL;
	PushDummyScope((SAst*)ast);
	{
		Char c = ReadChar();
		if (c != L'\n')
		{
			FileBuf = c;
			((SAst*)ast)->Name = ReadIdentifier(True, False);
			AddScopeName((SAst*)ast, True);
			AssertNextChar(L'\n', True);
		}
		else
			((SAst*)ast)->Name = L"$";
	}
	{
		SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
		InitAst((SAst*)type, AstTypeId_TypePrim, NewPos(SrcName, row, col), NULL, False, False);
		type->Kind = AstTypePrimKind_Int;
		((SAstStatBreakable*)ast)->BlockVar->Type = (SAstType*)type;
	}
	{
		SAstStat* stat;
		EAstTypeId type_id;
		ast->StatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatTry, (SAst*)ast);
		while (type_id == AstTypeId_StatCatch)
		{
			SAstStatCatch* catch_ = (SAstStatCatch*)stat;
			catch_->StatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatCatch, (SAst*)ast);
			ListAdd(ast->Catches, catch_);
		}
		if (type_id == AstTypeId_StatFinally)
			ast->FinallyStatBlock = ParseDummyBlock(&stat, &type_id, AstTypeId_StatFinally, (SAst*)ast);
		ASSERT(type_id == AstTypeId_StatEnd);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatCatch(int row, int col, const SAst* block)
{
	SAstStatCatch* ast = (SAstStatCatch*)Alloc(sizeof(SAstStatCatch));
	InitAst((SAst*)ast, AstTypeId_StatCatch, NULL, NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = -1;
	ast->Conds = ListNew();
	ast->StatBlock = NULL;
	if (block->TypeId != AstTypeId_StatTry)
	{
		Err(L"EP0051", NewPos(SrcName, row, col));
		ReadUntilRet(Read());
		return NULL;
	}
	{
		Char c = ReadChar();
		if (c == L'\n')
		{
			SAstExpr** exprs = (SAstExpr**)Alloc(sizeof(SAstExpr*) * 2);
			const SPos* pos = NewPos(SrcName, Row, Col);
			U64 value_begin = 0;
			U64 value_end = _UI32_MAX;
			exprs[0] = (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Int, &value_begin);
			exprs[1] = (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Int, &value_end);
			ListAdd(ast->Conds, exprs);
		}
		else
		{
			FileBuf = c;
			for (; ; )
			{
				SAstExpr** exprs = (SAstExpr**)Alloc(sizeof(SAstExpr*) * 2);
				exprs[0] = ParseExpr();
				exprs[1] = NULL;
				c = ReadChar();
				if (c == L'\0')
					break;
				if (c == L'\n')
				{
					ListAdd(ast->Conds, exprs);
					break;
				}
				if (c == L',')
				{
					ListAdd(ast->Conds, exprs);
					continue;
				}
				FileBuf = c;
				{
					const Char* s = ReadIdentifier(True, False);
					if (wcscmp(s, L"to") != 0)
						Err(L"EP0047", NewPos(SrcName, Row, Col), s);
				}
				exprs[1] = ParseExpr();
				c = ReadChar();
				if (c == L'\0')
					break;
				ListAdd(ast->Conds, exprs);
				if (c == L'\n')
					break;
				if (c != L',')
					NextCharErr(L',', c);
			}
		}
	}
	return (SAstStat*)ast;
}

static SAstStat* ParseStatFinally(int row, int col, const SAst* block)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatFinally, NULL, NULL, False, False);
	ast->AsmTop = NULL;
	ast->AsmBottom = NULL;
	ast->PosRowBottom = -1;
	if (block->TypeId != AstTypeId_StatTry)
	{
		Err(L"EP0052", NewPos(SrcName, row, col));
		ReadUntilRet(Read());
		return NULL;
	}
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatThrow(void)
{
	SAstStatThrow* ast = (SAstStatThrow*)Alloc(sizeof(SAstStatThrow));
	InitAst((SAst*)ast, AstTypeId_StatThrow, NewPos(SrcName, Row, Col), NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = Row;
	ast->Code = ParseExpr();
	AssertNextChar(L'\n', True);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatBlock(void)
{
	SAstStatBlock* ast = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
	InitAst((SAst*)ast, AstTypeId_StatBlock, NewPos(SrcName, Row, Col), NULL, False, True);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	{
		Char c = ReadChar();
		if (c != L'\n')
		{
			FileBuf = c;
			((SAst*)ast)->Name = ReadIdentifier(True, False);
			AddScopeName((SAst*)ast, True);
			AssertNextChar(L'\n', True);
		}
		else
			((SAst*)ast)->Name = L"$";
	}
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == NULL)
		{
			if (IsLast)
				return NULL;
			else
				continue;
		}
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	((SAstStat*)ast)->PosRowBottom = Row - 1;
	AddEndPosScope();
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatRet(void)
{
	SAstStatRet* ast = (SAstStatRet*)Alloc(sizeof(SAstStatRet));
	InitAst((SAst*)ast, AstTypeId_StatRet, NewPos(SrcName, Row, Col), NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = Row;
	{
		Char c = ReadChar();
		if (c != L'\n')
		{
			FileBuf = c;
			ast->Value = ParseExpr();
			AssertNextChar(L'\n', True);
		}
		else
			ast->Value = NULL;
	}
	return (SAstStat*)ast;
}

static SAstStat* ParseStatDo(void)
{
	SAstStatDo* ast = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
	InitAst((SAst*)ast, AstTypeId_StatDo, NewPos(SrcName, Row, Col), NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = Row;
	ast->Expr = ParseExpr();
	AssertNextChar(L'\n', True);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatBreak(void)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatBreak, NewPos(SrcName, Row, Col), NULL, False, False);
	ast->AsmTop = NULL;
	ast->AsmBottom = NULL;
	ast->PosRowBottom = Row;
	((SAst*)ast)->RefName = ReadIdentifier(True, False);
	AddScopeRefeds((SAst*)ast);
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatSkip(void)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatSkip, NewPos(SrcName, Row, Col), NULL, False, False);
	ast->AsmTop = NULL;
	ast->AsmBottom = NULL;
	ast->PosRowBottom = Row;
	((SAst*)ast)->RefName = ReadIdentifier(True, False);
	AddScopeRefeds((SAst*)ast);
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatAssert(void)
{
	SAstStatAssert* ast = (SAstStatAssert*)Alloc(sizeof(SAstStatAssert));
	InitAst((SAst*)ast, AstTypeId_StatAssert, NewPos(SrcName, Row, Col), NULL, False, False);
	((SAstStat*)ast)->AsmTop = NULL;
	((SAstStat*)ast)->AsmBottom = NULL;
	((SAstStat*)ast)->PosRowBottom = Row;
	ast->Cond = ParseExpr();
	AssertNextChar(L'\n', True);
	return (SAstStat*)ast;
}

static SAstType* ParseType(void)
{
	const SPos* pos = NewPos(SrcName, Row, Col);
	SAstType* ast = NULL;
	Char c = ReadChar();
	if (c == L'[')
	{
		AssertNextChar(L']', True);
		{
			SAstTypeArray* ast2 = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
			InitAst((SAst*)ast2, AstTypeId_TypeArray, pos, NULL, False, False);
			ast2->ItemType = ParseType();
			ast = (SAstType*)ast2;
		}
	}
	else
	{
		const Char* s;
		FileBuf = c;
		s = ReadIdentifier(True, True);
		if (wcslen(s) >= 4 && s[0] == L'b' && s[1] == L'i' && s[2] == L't')
		{
			const Char* size = SubStr(s, 3, (int)wcslen(s) - 3);
			if (wcscmp(size, L"8") == 0 || wcscmp(size, L"16") == 0 || wcscmp(size, L"32") == 0 || wcscmp(size, L"64") == 0)
			{
				SAstTypeBit* ast2 = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
				InitAst((SAst*)ast2, AstTypeId_TypeBit, pos, NULL, False, False);
				ast2->Size = _wtoi(size) / 8;
				ast = (SAstType*)ast2;
			}
		}
		if (ast == NULL)
		{
			if (wcscmp(s, L"func") == 0)
			{
				AssertNextChar(L'<', True);
				AssertNextChar(L'(', True);
				{
					SAstTypeFunc* ast2 = (SAstTypeFunc*)Alloc(sizeof(SAstTypeFunc));
					InitAst((SAst*)ast2, AstTypeId_TypeFunc, pos, NULL, False, False);
					ast2->FuncAttr = FuncAttr_None;
					ast2->Args = ListNew();
					ast2->Ret = NULL;
					c = ReadChar();
					if (c != L')')
					{
						FileBuf = c;
						for (; ; )
						{
							SAstTypeFuncArg* arg = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
							c = ReadChar();
							if (c == L'\0')
								break;
							if (c == L'&')
								arg->RefVar = True;
							else
							{
								arg->RefVar = False;
								FileBuf = c;
							}
							arg->Arg = ParseType();
							ListAdd(ast2->Args, arg);
							c = ReadChar();
							if (c == L')')
								break;
							if (c != L',')
							{
								NextCharErr(L',', c);
								break;
							}
						}
					}
					c = ReadChar();
					if (c == L':')
					{
						ast2->Ret = ParseType();
						c = ReadChar();
					}
					if (c != L'>')
						NextCharErr(L'>', c);
					ast = (SAstType*)ast2;
				}
			}
			else if (wcscmp(s, L"list") == 0)
			{
				AssertNextChar(L'<', True);
				{
					SAstTypeGen* ast2 = (SAstTypeGen*)Alloc(sizeof(SAstTypeGen));
					InitAst((SAst*)ast2, AstTypeId_TypeGen, pos, NULL, False, False);
					ast2->Kind = AstTypeGenKind_List;
					ast2->ItemType = ParseType();
					AssertNextChar(L'>', True);
					ast = (SAstType*)ast2;
				}
			}
			else if (wcscmp(s, L"stack") == 0)
			{
				AssertNextChar(L'<', True);
				{
					SAstTypeGen* ast2 = (SAstTypeGen*)Alloc(sizeof(SAstTypeGen));
					InitAst((SAst*)ast2, AstTypeId_TypeGen, pos, NULL, False, False);
					ast2->Kind = AstTypeGenKind_Stack;
					ast2->ItemType = ParseType();
					AssertNextChar(L'>', True);
					ast = (SAstType*)ast2;
				}
			}
			else if (wcscmp(s, L"queue") == 0)
			{
				AssertNextChar(L'<', True);
				{
					SAstTypeGen* ast2 = (SAstTypeGen*)Alloc(sizeof(SAstTypeGen));
					InitAst((SAst*)ast2, AstTypeId_TypeGen, pos, NULL, False, False);
					ast2->Kind = AstTypeGenKind_Queue;
					ast2->ItemType = ParseType();
					AssertNextChar(L'>', True);
					ast = (SAstType*)ast2;
				}
			}
			else if (wcscmp(s, L"dict") == 0)
			{
				AssertNextChar(L'<', True);
				{
					SAstTypeDict* ast2 = (SAstTypeDict*)Alloc(sizeof(SAstTypeDict));
					InitAst((SAst*)ast2, AstTypeId_TypeDict, pos, NULL, False, False);
					ast2->ItemTypeKey = ParseType();
					AssertNextChar(L',', True);
					ast2->ItemTypeValue = ParseType();
					AssertNextChar(L'>', True);
					ast = (SAstType*)ast2;
				}
			}
			else if (wcscmp(s, L"int") == 0)
			{
				SAstTypePrim* ast2 = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)ast2, AstTypeId_TypePrim, pos, NULL, False, False);
				ast2->Kind = AstTypePrimKind_Int;
				ast = (SAstType*)ast2;
			}
			else if (wcscmp(s, L"float") == 0)
			{
				SAstTypePrim* ast2 = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)ast2, AstTypeId_TypePrim, pos, NULL, False, False);
				ast2->Kind = AstTypePrimKind_Float;
				ast = (SAstType*)ast2;
			}
			else if (wcscmp(s, L"char") == 0)
			{
				SAstTypePrim* ast2 = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)ast2, AstTypeId_TypePrim, pos, NULL, False, False);
				ast2->Kind = AstTypePrimKind_Char;
				ast = (SAstType*)ast2;
			}
			else if (wcscmp(s, L"bool") == 0)
			{
				SAstTypePrim* ast2 = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)ast2, AstTypeId_TypePrim, pos, NULL, False, False);
				ast2->Kind = AstTypePrimKind_Bool;
				ast = (SAstType*)ast2;
			}
			else
			{
				SAstTypeUser* ast2 = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
				InitAst((SAst*)ast2, AstTypeId_TypeUser, pos, NULL, False, False);
				((SAst*)ast2)->RefName = s;
				AddScopeRefeds((SAst*)ast2);
				ast = (SAstType*)ast2;
			}
		}
	}
	return ast;
}

static SAstExpr* ParseExpr(void)
{
	SAstExpr* ast = ParseExprThree();
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L':')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			((SAst*)ast2)->Pos = NewPos(SrcName, row, col);
			{
				Char c2 = Read();
				switch (c2)
				{
					case L':': ast2->Kind = AstExpr2Kind_Assign; break;
					case L'+': ast2->Kind = AstExpr2Kind_AssignAdd; break;
					case L'-': ast2->Kind = AstExpr2Kind_AssignSub; break;
					case L'*': ast2->Kind = AstExpr2Kind_AssignMul; break;
					case L'/': ast2->Kind = AstExpr2Kind_AssignDiv; break;
					case L'%': ast2->Kind = AstExpr2Kind_AssignMod; break;
					case L'^': ast2->Kind = AstExpr2Kind_AssignPow; break;
					case L'~': ast2->Kind = AstExpr2Kind_AssignCat; break;
					case L'$': ast2->Kind = AstExpr2Kind_Swap; break;
					default:
						Err(L"EP0054", NewPos(SrcName, row, col), CharToStr(c2));
						ReadUntilRet(c2);
						return NULL;
				}
			}
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExpr();
			ast = (SAstExpr*)ast2;
		}
		else
			FileBuf = c;
	}
	return ast;
}

static SAstExpr* ParseExprThree(void)
{
	SAstExpr* ast = ParseExprOr();
	for (; ; )
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'?')
		{
			AssertNextChar(L'(', False);
			{
				SAstExpr3* ast2 = (SAstExpr3*)Alloc(sizeof(SAstExpr3));
				InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr3, NewPos(SrcName, row, col));
				ast2->Children[0] = ast;
				ast2->Children[1] = ParseExpr();
				AssertNextChar(L',', True);
				ast2->Children[2] = ParseExpr();
				ast = (SAstExpr*)ast2;
			}
			AssertNextChar(L')', True);
		}
		else
		{
			FileBuf = c;
			break;
		}
	}
	return ast;
}

static SAstExpr* ParseExprOr(void)
{
	SAstExpr* ast = ParseExprAnd();
	for (; ; )
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'|')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_Or;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprAnd();
			ast = (SAstExpr*)ast2;
		}
		else
		{
			FileBuf = c;
			break;
		}
	}
	return ast;
}

static SAstExpr* ParseExprAnd(void)
{
	SAstExpr* ast = ParseExprCmp();
	for (; ; )
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'&')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_And;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprCmp();
			ast = (SAstExpr*)ast2;
		}
		else
		{
			FileBuf = c;
			break;
		}
	}
	return ast;
}

static SAstExpr* ParseExprCmp(void)
{
	SAstExpr* ast = ParseExprCat();
	{
		Bool end_flag = False;
		do
		{
			int row = Row;
			int col = Col;
			Char c = ReadChar();
			switch (c)
			{
				case L'<':
					{
						c = Read();
						if (c == L'=')
						{
							SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
							InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
							ast2->Kind = AstExpr2Kind_LE;
							ast2->Children[0] = ast;
							ast2->Children[1] = ParseExprCat();
							ast = (SAstExpr*)ast2;
						}
						else if (c == L'>')
						{
							c = Read();
							if (c == L'&')
							{
								SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
								InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
								ast2->Kind = AstExpr2Kind_NEqRef;
								ast2->Children[0] = ast;
								ast2->Children[1] = ParseExprCat();
								ast = (SAstExpr*)ast2;
							}
							else if (c == L'$')
							{
								SAstExprAs* ast2 = (SAstExprAs*)Alloc(sizeof(SAstExprAs));
								InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprAs, NewPos(SrcName, row, col));
								ast2->Kind = AstExprAsKind_NIs;
								ast2->Child = ast;
								ast2->ChildType = ParseType();
								ast = (SAstExpr*)ast2;
							}
							else
							{
								FileBuf = c;
								{
									SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
									InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
									ast2->Kind = AstExpr2Kind_NEq;
									ast2->Children[0] = ast;
									ast2->Children[1] = ParseExprCat();
									ast = (SAstExpr*)ast2;
								}
							}
						}
						else
						{
							FileBuf = c;
							{
								SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
								InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
								ast2->Kind = AstExpr2Kind_LT;
								ast2->Children[0] = ast;
								ast2->Children[1] = ParseExprCat();
								ast = (SAstExpr*)ast2;
							}
						}
					}
					break;
				case L'>':
					{
						SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
						InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
						c = Read();
						if (c == L'=')
							ast2->Kind = AstExpr2Kind_GE;
						else
						{
							FileBuf = c;
							ast2->Kind = AstExpr2Kind_GT;
						}
						ast2->Children[0] = ast;
						ast2->Children[1] = ParseExprCat();
						ast = (SAstExpr*)ast2;
					}
					break;
				case L'=':
					{
						c = Read();
						if (c == L'&')
						{
							SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
							InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
							ast2->Kind = AstExpr2Kind_EqRef;
							ast2->Children[0] = ast;
							ast2->Children[1] = ParseExprCat();
							ast = (SAstExpr*)ast2;
						}
						else if (c == L'$')
						{
							SAstExprAs* ast2 = (SAstExprAs*)Alloc(sizeof(SAstExprAs));
							InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprAs, NewPos(SrcName, row, col));
							ast2->Kind = AstExprAsKind_Is;
							ast2->Child = ast;
							ast2->ChildType = ParseType();
							ast = (SAstExpr*)ast2;
						}
						else
						{
							FileBuf = c;
							{
								SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
								InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
								ast2->Kind = AstExpr2Kind_Eq;
								ast2->Children[0] = ast;
								ast2->Children[1] = ParseExprCat();
								ast = (SAstExpr*)ast2;
							}
						}
					}
					break;
				default:
					FileBuf = c;
					end_flag = True;
					break;
			}
		} while (!end_flag);
	}
	return ast;
}

static SAstExpr* ParseExprCat(void)
{
	SAstExpr* ast = ParseExprAdd();
	for (; ; )
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'~')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_Cat;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprAdd();
			ast = (SAstExpr*)ast2;
		}
		else
		{
			FileBuf = c;
			break;
		}
	}
	return ast;
}

static SAstExpr* ParseExprAdd(void)
{
	SAstExpr* ast = ParseExprMul();
	for (; ; )
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'+')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_Add;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprMul();
			ast = (SAstExpr*)ast2;
		}
		else if (c == L'-')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_Sub;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprMul();
			ast = (SAstExpr*)ast2;
		}
		else
		{
			FileBuf = c;
			break;
		}
	}
	return ast;
}

static SAstExpr* ParseExprMul(void)
{
	SAstExpr* ast = ParseExprPlus();
	{
		Bool end_flag = False;
		do
		{
			int row = Row;
			int col = Col;
			Char c = ReadChar();
			switch (c)
			{
				case L'*':
					{
						SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
						InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
						ast2->Kind = AstExpr2Kind_Mul;
						ast2->Children[0] = ast;
						ast2->Children[1] = ParseExprPlus();
						ast = (SAstExpr*)ast2;
					}
					break;
				case L'/':
					{
						SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
						InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
						ast2->Kind = AstExpr2Kind_Div;
						ast2->Children[0] = ast;
						ast2->Children[1] = ParseExprPlus();
						ast = (SAstExpr*)ast2;
					}
					break;
				case L'%':
					{
						SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
						InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
						ast2->Kind = AstExpr2Kind_Mod;
						ast2->Children[0] = ast;
						ast2->Children[1] = ParseExprPlus();
						ast = (SAstExpr*)ast2;
					}
					break;
				default:
					FileBuf = c;
					end_flag = True;
					break;
			}
		} while (!end_flag);
	}
	return ast;
}

static SAstExpr* ParseExprPlus(void)
{
	SAstExpr* ast = ParseExprPow();
	if (ast != NULL)
		return ast;
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'#')
		{
			c = Read();
			if (c == L'[')
			{
				SAstExprNewArray* ast2 = (SAstExprNewArray*)Alloc(sizeof(SAstExprNewArray));
				InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprNewArray, NewPos(SrcName, row, col));
				ast2->Idces = ListNew();
				for (; ; )
				{
					ListAdd(ast2->Idces, ParseExpr());
					c = ReadChar();
					if (c == L'\0')
						break;
					if (c == L']')
						break;
					if (c != L',')
					{
						NextCharErr(L',', c);
						return NULL;
					}
				}
				ast2->ItemType = ParseType();
				ast = (SAstExpr*)ast2;
			}
			else if (c == L'#')
			{
				SAstExpr1* ast2 = (SAstExpr1*)Alloc(sizeof(SAstExpr1));
				InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr1, NewPos(SrcName, row, col));
				ast2->Kind = AstExpr1Kind_Copy;
				ast2->Child = ParseExprPlus();
				ast = (SAstExpr*)ast2;
			}
			else
			{
				FileBuf = c;
				{
					SAstExprNew* ast2 = (SAstExprNew*)Alloc(sizeof(SAstExprNew));
					InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprNew, NewPos(SrcName, row, col));
					ast2->ItemType = ParseType();
					ast2->AutoCreated = False;
					ast = (SAstExpr*)ast2;
				}
			}
		}
		else
		{
			SAstExpr1* ast2 = (SAstExpr1*)Alloc(sizeof(SAstExpr1));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr1, NewPos(SrcName, row, col));
			switch (c)
			{
				case L'+': ast2->Kind = AstExpr1Kind_Plus; break;
				case L'-': ast2->Kind = AstExpr1Kind_Minus; break;
				case L'!': ast2->Kind = AstExpr1Kind_Not; break;
				case L'^': ast2->Kind = AstExpr1Kind_Len; break;
				default:
					Err(L"EP0054", NewPos(SrcName, row, col), CharToStr(c));
					ReadUntilRet(c);
					return NULL;
			}
			ast2->Child = ParseExprPlus();
			ast = (SAstExpr*)ast2;
		}
	}
	return ast;
}

static SAstExpr* ParseExprPow(void)
{
	SAstExpr* ast = ParseExprCall();
	if (ast == NULL)
		return ast; // Interpret as a unary operator.
	{
		int row = Row;
		int col = Col;
		Char c = ReadChar();
		if (c == L'^')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_Pow;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprPlus();
			ast = (SAstExpr*)ast2;
		}
		else
			FileBuf = c;
	}
	return ast;
}

static SAstExpr* ParseExprCall(void)
{
	SAstExpr* ast = ParseExprValue();
	if (ast == NULL)
		return ast;
	{
		Bool end_flag = False;
		do
		{
			int row = Row;
			int col = Col;
			Char c = ReadChar();
			switch (c)
			{
				case L'(':
					{
						SAstExprCall* ast2 = (SAstExprCall*)Alloc(sizeof(SAstExprCall));
						ASSERT(ast != NULL);
						InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprCall, NewPos(SrcName, row, col));
						ast2->Func = ast;
						ast2->Args = ListNew();
						c = ReadChar();
						if (c != L')')
						{
							FileBuf = c;
							for (; ; )
							{
								SAstExprCallArg* arg = (SAstExprCallArg*)Alloc(sizeof(SAstExprCallArg));
								Bool skip_var = False;
								c = ReadChar();
								if (c == L'&')
								{
									arg->RefVar = True;
									c = ReadChar();
									if (c == L',' || c == L')')
										skip_var = True;
								}
								else
									arg->RefVar = False;
								FileBuf = c;
								arg->SkipVar = skip_var;
								if (skip_var)
								{
									SAstExpr* ast3 = (SAstExpr*)Alloc(sizeof(SAstExpr));
									SAstArg* tmp_var = MakeBlockVar(row, col);
									InitAstExpr(ast3, AstTypeId_ExprRef, ((SAst*)ast2)->Pos);
									((SAst*)ast3)->RefName = L"$";
									((SAst*)ast3)->RefItem = (SAst*)tmp_var;
									arg->Arg = ast3;
								}
								else
								{
									arg->Arg = ParseExpr();
								}
								ListAdd(ast2->Args, arg);
								c = ReadChar();
								if (c == L'\0')
									break;
								if (c == L')')
									break;
								if (c != L',')
								{
									NextCharErr(L',', c);
									return NULL;
								}
							}
						}
						ast = (SAstExpr*)ast2;
					}
					break;
				case L'[':
					{
						SAstExprArray* ast2 = (SAstExprArray*)Alloc(sizeof(SAstExprArray));
						ASSERT(ast != NULL);
						InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprArray, NewPos(SrcName, row, col));
						ast2->Var = ast;
						ast2->Idx = ParseExpr();
						AssertNextChar(L']', True);
						ast = (SAstExpr*)ast2;
					}
					break;
				case L'.':
					{
						SAstExprDot* ast2 = (SAstExprDot*)Alloc(sizeof(SAstExprDot));
						ASSERT(ast != NULL);
						InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprDot, NewPos(SrcName, row, col));
						ast2->Var = ast;
						ast2->Member = ReadIdentifier(True, False);
						ast2->ClassItem = NULL;
						ast = (SAstExpr*)ast2;
					}
					break;
				case L'$':
					{
						ASSERT(ast != NULL);
						c = Read();
						if (c == L'>')
						{
							SAstExprToBin* ast2 = (SAstExprToBin*)Alloc(sizeof(SAstExprToBin));
							InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprToBin, NewPos(SrcName, row, col));
							ast2->Child = ast;
							ast2->ChildType = ParseType();
							ast = (SAstExpr*)ast2;
						}
						else if (c == L'<')
						{
							SAstExprFromBin* ast2 = (SAstExprFromBin*)Alloc(sizeof(SAstExprFromBin));
							InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprFromBin, NewPos(SrcName, row, col));
							ast2->Child = ast;
							ast2->ChildType = ParseType();
							{
								U64 value = 0;
								ast2->Offset = (SAstExpr*)ObtainPrimValue(((SAst*)ast)->Pos, AstTypePrimKind_Int, &value);
							}
							ast = (SAstExpr*)ast2;
						}
						else
						{
							FileBuf = c;
							{
								SAstExprAs* ast2 = (SAstExprAs*)Alloc(sizeof(SAstExprAs));
								InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprAs, NewPos(SrcName, row, col));
								ast2->Kind = AstExprAsKind_As;
								ast2->Child = ast;
								ast2->ChildType = ParseType();
								ast = (SAstExpr*)ast2;
							}
						}
					}
					break;
				default:
					FileBuf = c;
					end_flag = True;
					break;
			}
		} while (!end_flag);
	}
	return ast;
}

static SAstExpr* ParseExprValue(void)
{
	int row = Row;
	int col = Col;
	Char c = ReadChar();
	const SPos* pos = NewPos(SrcName, row, col);
	switch (c)
	{
		case L'"':
			{
				Char buf[1025];
				int len = 0;
				Bool escape = False;
				for (; ; )
				{
					c = ReadStrict();
					if (c == L'\0')
						return NULL;
					if (escape)
					{
						if (c == L'{')
						{
							SAstExpr2* cat = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
							InitAstExpr((SAstExpr*)cat, AstTypeId_Expr2, pos);
							cat->Kind = AstExpr2Kind_Cat;
							{
								SAstExpr2* cat2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
								InitAstExpr((SAstExpr*)cat2, AstTypeId_Expr2, pos);
								cat2->Kind = AstExpr2Kind_Cat;
								buf[len] = L'\0';
								{
									Char* buf2 = (Char*)Alloc(sizeof(Char) * (size_t)(len + 1));
									memcpy(buf2, buf, sizeof(Char) * (size_t)(len + 1));
									cat2->Children[0] = (SAstExpr*)ObtainStrValue(pos, buf2);
								}
								{
									SAstExprCall* call = (SAstExprCall*)Alloc(sizeof(SAstExprCall));
									InitAstExpr((SAstExpr*)call, AstTypeId_ExprCall, pos);
									call->Args = ListNew();
									{
										SAstExprDot* dot = (SAstExprDot*)Alloc(sizeof(SAstExprDot));
										InitAstExpr((SAstExpr*)dot, AstTypeId_ExprDot, pos);
										dot->Var = ParseExpr();
										dot->Member = L"toStr";
										dot->ClassItem = NULL;
										call->Func = (SAstExpr*)dot;
									}
									cat2->Children[1] = (SAstExpr*)call;
								}
								AssertNextChar(L'}', False);
								cat->Children[0] = (SAstExpr*)cat2;
								FileBuf = L'"';
								cat->Children[1] = ParseExprValue();
							}
							return (SAstExpr*)cat;
						}
						if (len == 1024)
						{
							buf[1024] = L'\0';
							Err(L"EP0012", NewPos(SrcName, row, col), buf);
							ReadUntilRet(c);
							return NULL;
						}
						buf[len] = EscChar(c);
						len++;
						escape = False;
						continue;
					}
					if (c == L'"')
						break;
					if (c == L'\\')
					{
						escape = True;
						continue;
					}
					if (len == 1024)
					{
						buf[1024] = L'\0';
						Err(L"EP0012", NewPos(SrcName, row, col), buf);
						ReadUntilRet(c);
						return NULL;
					}
					buf[len] = c;
					len++;
				}
				buf[len] = L'\0';
				{
					Char* buf2 = (Char*)Alloc(sizeof(Char) * (size_t)(len + 1));
					memcpy(buf2, buf, sizeof(Char) * (size_t)(len + 1));
					return (SAstExpr*)ObtainStrValue(pos, buf2);
				}
			}
			break;
		case L'\'':
			{
				Char buf = L'\0';
				Bool set = False;
				Bool escape = False;
				for (; ; )
				{
					c = ReadStrict();
					ASSERT(c != L'\0');
					if (escape)
					{
						if (set)
						{
							Err(L"EP0013", NewPos(SrcName, row, col), CharToStr(buf));
							ReadUntilRet(c);
							return NULL;
						}
						buf = EscChar(c);
						set = True;
						escape = False;
						continue;
					}
					if (c == L'\'')
						break;
					if (c == L'\\')
					{
						escape = True;
						continue;
					}
					if (set)
					{
						Err(L"EP0013", NewPos(SrcName, row, col), CharToStr(buf));
						ReadUntilRet(c);
						return NULL;
					}
					buf = c;
					set = True;
				}
				if (!set)
				{
					Err(L"EP0014", NewPos(SrcName, row, col));
					ReadUntilRet(c);
					return NULL;
				}
				{
					U64 value = (U64)buf;
					return (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Char, &value);
				}
			}
			break;
		case L'(':
			{
				SAstExpr* ast = ParseExpr();
				c = ReadChar();
				if (c != L')')
				{
					Err(L"EP0015", NewPos(SrcName, Row, Col));
					ReadUntilRet(c);
					return NULL;
				}
				return ast;
			}
			break;
		case L'[':
			{
				SAstExprValueArray* ast = (SAstExprValueArray*)Alloc(sizeof(SAstExprValueArray));
				InitAstExpr((SAstExpr*)ast, AstTypeId_ExprValueArray, pos);
				ast->Values = ListNew();
				c = ReadChar();
				if (c != L']')
				{
					FileBuf = c;
					for (; ; )
					{
						SAstExpr* expr = ParseExpr();
						if (expr != NULL)
							ListAdd(ast->Values, expr);
						c = ReadChar();
						if (c == L'\0')
							break;
						if (c == L']')
							break;
						if (c != L',')
						{
							NextCharErr(L',', c);
							return NULL;
						}
					}
				}
				return (SAstExpr*)ast;
			}
			break;
		case L'%':
			{
				const Char* s = ReadIdentifier(False, False);
				SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
				InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, pos);
				{
					SAstTypeEnumElement* type = (SAstTypeEnumElement*)Alloc(sizeof(SAstTypeEnumElement));
					InitAst((SAst*)type, AstTypeId_TypeEnumElement, pos, NULL, False, False);
					((SAstExpr*)expr)->Type = (SAstType*)type;
				}
				*(const Char**)expr->Value = s;
				return (SAstExpr*)expr;
			}
			break;
		default:
			if (L'0' <= c && c <= L'9')
				return ParseExprNumber(row, col, c);
			else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\')
			{
				FileBuf = c;
				{
					const Char* s = ReadIdentifier(True, True);
					if (wcscmp(s, L"false") == 0)
					{
						U64 value = 0;
						return (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Bool, &value);
					}
					if (wcscmp(s, L"true") == 0)
					{
						U64 value = 1;
						return (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Bool, &value);
					}
					if (wcscmp(s, L"inf") == 0)
					{
						U64 value = 0x7ff0000000000000ULL;
						return (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Float, &value);
					}
					if (wcscmp(s, L"null") == 0)
					{
						SAstExprValue* ast = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
						InitAstExpr((SAstExpr*)ast, AstTypeId_ExprValue, pos);
						{
							SAstTypeNull* type = (SAstTypeNull*)Alloc(sizeof(SAstTypeNull));
							InitAst((SAst*)type, AstTypeId_TypeNull, pos, NULL, False, False);
							((SAstExpr*)ast)->Type = (SAstType*)type;
						}
						*(U64*)ast->Value = 0;
						return (SAstExpr*)ast;
					}
					if (wcscmp(s, L"dbg") == 0)
					{
						U64 value = Option->Rls ? 0 : 1;
						return (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Bool, &value);
					}
					if (wcscmp(s, L"env") == 0)
					{
						U64 value = 0;
						return (SAstExpr*)ObtainPrimValue(pos, AstTypePrimKind_Int, &value);
					}
					{
						SAstExpr* ast = (SAstExpr*)Alloc(sizeof(SAstExpr));
						InitAstExpr(ast, AstTypeId_ExprRef, pos);
						((SAst*)ast)->RefName = s;
						AddScopeRefeds((SAst*)ast);
						return ast;
					}
				}
			}
			break;
	}
	FileBuf = c;
	return NULL;
}

static SAstExpr* ParseExprNumber(int row, int col, Char c)
{
	SAstExprValue* ast = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
	InitAstExpr((SAstExpr*)ast, AstTypeId_ExprValue, NewPos(SrcName, row, col));
	{
		Char buf[1025];
		int len = 0;
		int base = 10;
		Bool change_base = False;
		Bool float_type = False;
		int bit_size = 0; // The size for bit types.
		for (; ; )
		{
			if (c == L'x')
			{
				if (change_base || float_type)
				{
					Err(L"EP0016", NewPos(SrcName, row, col));
					ReadUntilRet(c);
					return NULL;
				}
				if (len != 1 || buf[0] != L'0')
				{
					Err(L"EP0019", NewPos(SrcName, row, col));
					ReadUntilRet(c);
					return NULL;
				}
				len = 0;
				base = 16;
				change_base = True;
			}
			else if (c == L'.')
			{
				if (change_base || float_type)
				{
					Err(L"EP0017", NewPos(SrcName, row, col));
					ReadUntilRet(c);
					return NULL;
				}
				if (len == 1024)
				{
					buf[1024] = L'\0';
					Err(L"EP0018", NewPos(SrcName, row, col), buf);
					ReadUntilRet(c);
					return NULL;
				}
				buf[len] = c;
				len++;
				float_type = True;
			}
			else if (L'0' <= c && c <= L'9' || L'A' <= c && c <= L'F')
			{
				if (len == 1024)
				{
					buf[1024] = L'\0';
					Err(L"EP0018", NewPos(SrcName, row, col), buf);
					ReadUntilRet(c);
					return NULL;
				}
				buf[len] = c;
				len++;
			}
			else
			{
				FileBuf = c;
				break;
			}
			c = Read();
		}
		if (len == 0 || buf[len - 1] == L'.')
		{
			Err(L"EP0017", NewPos(SrcName, row, col));
			ReadUntilRet(Read());
			return NULL;
		}
		if (float_type)
		{
			c = Read();
			if (c == L'e')
			{
				if (len == 1024)
				{
					buf[1024] = L'\0';
					Err(L"EP0018", NewPos(SrcName, row, col), buf);
					ReadUntilRet(c);
					return NULL;
				}
				buf[len] = c;
				len++;
				c = Read();
				if (c != L'+' && c != L'-')
				{
					Err(L"EP0056", NewPos(SrcName, row, col));
					ReadUntilRet(c);
					return NULL;
				}
				if (len == 1024)
				{
					buf[1024] = L'\0';
					Err(L"EP0018", NewPos(SrcName, row, col), buf);
					ReadUntilRet(c);
					return NULL;
				}
				buf[len] = c;
				len++;
				c = Read();
				if (!(L'0' <= c && c <= L'9'))
				{
					Err(L"EP0056", NewPos(SrcName, row, col));
					ReadUntilRet(c);
					return NULL;
				}
				do
				{
					if (len == 1024)
					{
						buf[1024] = L'\0';
						Err(L"EP0018", NewPos(SrcName, row, col), buf);
						ReadUntilRet(c);
						return NULL;
					}
					buf[len] = c;
					len++;
					c = Read();
				} while (L'0' <= c && c <= L'9');
			}
			FileBuf = c;
			{
				Char* end_ptr;
				double value;
				errno = 0;
				buf[len] = L'\0';
				value = wcstod(buf, &end_ptr);
				if (*end_ptr != L'\0' || errno == ERANGE)
				{
					Err(L"EP0020", NewPos(SrcName, row, col), buf);
					ReadUntilRet(Read());
					return NULL;
				}
				*(double*)ast->Value = value;
			}
		}
		else
		{
			c = Read();
			if (c == L'b')
			{
				c = Read();
				switch (c)
				{
					case L'8':
						bit_size = 1;
						break;
					case L'1':
						AssertNextChar(L'6', False);
						bit_size = 2;
						break;
					case L'3':
						AssertNextChar(L'2', False);
						bit_size = 4;
						break;
					case L'6':
						AssertNextChar(L'4', False);
						bit_size = 8;
						break;
					default:
						buf[len] = L'\0';
						Err(L"EP0022", NewPos(SrcName, row, col), buf);
						ReadUntilRet(c);
						return NULL;
				}
			}
			else
				FileBuf = c;
			{
				Char* end_ptr;
				U64 value;
				errno = 0;
				buf[len] = L'\0';
				value = _wcstoui64(buf, &end_ptr, base);
				if (*end_ptr != L'\0' || errno == ERANGE)
				{
					Err(L"EP0021", NewPos(SrcName, row, col), buf);
					ReadUntilRet(Read());
					return NULL;
				}
				if (bit_size == 1 && value > _UI8_MAX || bit_size == 2 && value > _UI16_MAX || bit_size == 4 && value > _UI32_MAX || bit_size == 0 && value > _I64_MAX)
				{
					Err(L"EP0021", NewPos(SrcName, row, col), buf);
					ReadUntilRet(Read());
					return NULL;
				}
				*(U64*)ast->Value = value;
			}
		}
		if (bit_size == 0)
		{
			SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
			InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos, NULL, False, False);
			if (float_type)
				type->Kind = AstTypePrimKind_Float;
			else
				type->Kind = AstTypePrimKind_Int;
			((SAstExpr*)ast)->Type = (SAstType*)type;
		}
		else
		{
			SAstTypeBit* type = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
			InitAst((SAst*)type, AstTypeId_TypeBit, ((SAst*)ast)->Pos, NULL, False, False);
			type->Size = bit_size;
			((SAstExpr*)ast)->Type = (SAstType*)type;
		}
	}
	return (SAstExpr*)ast;
}

static void InterpretImpl1Color(int* ptr, int str_level, const Char* str, U8* color, S64 comment_level, U64 flags)
{
	while (str[*ptr] != L'\0')
	{
		const Char c = str[*ptr];
		if (comment_level <= 0)
		{
			if (str_level > 0 && c == L'}')
			{
				break;
			}
			else if (c == L' ' || c == L'\t')
			{
				color[*ptr] = CharColor_None;
				(*ptr)++;
			}
			else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\' || c == L'%' && (L'a' <= str[*ptr + 1] && str[*ptr + 1] <= L'z' || L'A' <= str[*ptr + 1] && str[*ptr + 1] <= L'Z' || str[*ptr + 1] == L'_'))
			{
				Bool at = False;
				int begin = *ptr;
				int end;
				do
				{
					if (str[*ptr] == L'@')
						at = True;
					(*ptr)++;
				} while (L'a' <= str[*ptr] && str[*ptr] <= L'z' || L'A' <= str[*ptr] && str[*ptr] <= L'Z' || str[*ptr] == L'_' || L'0' <= str[*ptr] && str[*ptr] <= L'9' || str[*ptr] == L'@' || str[*ptr] == L'\\');
				end = *ptr;
				{
					U8 new_color = (U8)(at ? CharColor_Global : CharColor_Identifier);
					int word_len = end - begin;
					if (!at && word_len <= 16)
					{
						Char word[17];
						wcsncpy(word, str + begin, word_len);
						word[word_len] = L'\0';
						if (IsReserved(word))
							new_color = (U8)CharColor_Reserved;
					}
					int i;
					for (i = begin; i < end; i++)
						color[i] = new_color;
				}
			}
			else if (L'0' <= c && c <= L'9')
			{
				U8 new_color = (U8)CharColor_Number;
				do
				{
					color[*ptr] = new_color;
					(*ptr)++;
				} while (L'0' <= str[*ptr] && str[*ptr] <= L'9' || L'A' <= str[*ptr] && str[*ptr] <= L'F' || str[*ptr] == L'x' || str[*ptr] == L'.');
				if (str[*ptr] == L'e')
				{
					color[*ptr] = new_color;
					(*ptr)++;
					if (str[*ptr] == L'+' || str[*ptr] == L'-')
					{
						color[*ptr] = new_color;
						(*ptr)++;
						while (L'0' <= str[*ptr] && str[*ptr] <= L'9')
						{
							color[*ptr] = new_color;
							(*ptr)++;
						}
					}
				}
				else if (str[*ptr] == L'b')
				{
					color[*ptr] = new_color;
					(*ptr)++;
					while (L'0' <= str[*ptr] && str[*ptr] <= L'9')
					{
						color[*ptr] = new_color;
						(*ptr)++;
					}
				}
			}
			else if (c == L'"')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Str;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
						if (str[*ptr] == L'{')
						{
							color[*ptr] = CharColor_Str;
							(*ptr)++;
							InterpretImpl1Color(ptr, str_level + 1, str, color, comment_level, flags);
							continue;
						}
					}
					color[*ptr] = CharColor_Str;
					(*ptr)++;
					if (str[*ptr] == L'"')
					{
						color[*ptr] = CharColor_Str;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L'\'')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Char;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
					}
					color[*ptr] = CharColor_Char;
					(*ptr)++;
					if (str[*ptr] == L'\'')
					{
						color[*ptr] = CharColor_Char;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L'{')
			{
				color[*ptr] = CharColor_Comment;
				(*ptr)++;
				comment_level++;
			}
			else if (c == L';')
			{
				do
				{
					color[*ptr] = CharColor_LineComment;
					(*ptr)++;
				} while (str[*ptr] != L'\0');
			}
			else
			{
				color[*ptr] = CharColor_Symbol;
				(*ptr)++;
			}
		}
		else
		{
			if (c == L'"')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
					}
					color[*ptr] = CharColor_Comment;
					(*ptr)++;
					if (str[*ptr] == L'"')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L'\'')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
					}
					color[*ptr] = CharColor_Comment;
					(*ptr)++;
					if (str[*ptr] == L'\'')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L';')
			{
				do
				{
					color[*ptr] = CharColor_Comment;
					(*ptr)++;
				} while (str[*ptr] != L'\0');
			}
			else
			{
				if (c == L'{')
					comment_level++;
				else if (c == L'}')
					comment_level--;
				color[*ptr] = CharColor_Comment;
				(*ptr)++;
			}
		}
	}
}

static void InterpretImpl1Align(int* ptr_buf, int* ptr_str, Char* buf, const Char* str, S64* comment_level, U64* flags, S64* tab_context, S64 cursor_x, S64* new_cursor_x, Char* add_end)
{
	EAlignmentToken prev = AlignmentToken_None;
	if (*comment_level <= 0)
	{
		int access_public = -1;
		int access_override = -1;
		Bool access_override2 = False;
		S64 enum_depth = -1;
		while (str[*ptr_str] != L'\0')
		{
			const Char c = str[*ptr_str];
			if (c == L' ' || c == L'\t')
			{
				(*ptr_str)++;
				continue;
			}
			if (c == L'+')
			{
				access_public = *ptr_str;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				(*ptr_str)++;
				continue;
			}
			if (c == L'*')
			{
				if (access_override != -1)
					access_override2 = True;
				access_override = *ptr_str;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				(*ptr_str)++;
				continue;
			}
			if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\')
			{
				prev = AlignmentToken_Value;
				int begin = *ptr_str;
				int end;
				do
				{
					(*ptr_str)++;
				} while (L'a' <= str[*ptr_str] && str[*ptr_str] <= L'z' || L'A' <= str[*ptr_str] && str[*ptr_str] <= L'Z' || str[*ptr_str] == L'_' || L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9' || str[*ptr_str] == L'@' || str[*ptr_str] == L'\\');
				end = *ptr_str;
				if (str[begin] == L'e' && str[begin + 1] == L'n' && str[begin + 2] == L'd' && *ptr_str == begin + 3)
				{
					if (enum_depth == *tab_context)
						enum_depth = -1;
					(*tab_context)--;
					if (*tab_context < 0)
						*tab_context = 0;
				}
				else if (str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L'i' && str[begin + 3] == L'f' && *ptr_str == begin + 4 ||
					str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'd' && str[begin + 1] == L'e' && str[begin + 2] == L'f' && str[begin + 3] == L'a' && str[begin + 4] == L'u' && str[begin + 5] == L'l' && str[begin + 6] == L't' && *ptr_str == begin + 7 ||
					str[begin] == L'f' && str[begin + 1] == L'i' && str[begin + 2] == L'n' && str[begin + 3] == L'a' && str[begin + 4] == L'l' && str[begin + 5] == L'l' && str[begin + 6] == L'y' && *ptr_str == begin + 7)
				{
					(*tab_context)--;
					if (*tab_context < 0)
						*tab_context = 0;
				}
				else if (str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L't' && str[begin + 3] == L'c' && str[begin + 4] == L'h' && *ptr_str == begin + 5)
				{
					prev = AlignmentToken_Comma;
					(*tab_context)--;
					if (*tab_context < 0)
						*tab_context = 0;
				}
				if (enum_depth != -1)
					(*flags) |= 2;
				else
					(*flags) &= ~2;
				{
					S64 i;
					for (i = 0; i < *tab_context; i++)
						InterpretImpl1Write(ptr_buf, buf, L'\t');
				}
				{
					if (access_public != -1)
					{
						if (new_cursor_x != NULL && cursor_x == (S64)access_public)
							*new_cursor_x = (S64)*ptr_buf;
						InterpretImpl1Write(ptr_buf, buf, L'+');
					}
					if (access_override != -1)
					{
						if (new_cursor_x != NULL && cursor_x == (S64)access_override)
							*new_cursor_x = (S64)*ptr_buf;
						InterpretImpl1Write(ptr_buf, buf, L'*');
						if (access_override2)
							InterpretImpl1Write(ptr_buf, buf, L'*');
					}
					int i;
					for (i = begin; i < end; i++)
					{
						if (new_cursor_x != NULL && cursor_x == (S64)i)
							*new_cursor_x = (S64)*ptr_buf;
						InterpretImpl1Write(ptr_buf, buf, str[i]);
					}
				}
				Bool is_enum = False;
				if (str[begin] == L'e' && str[begin + 1] == L'n' && str[begin + 2] == L'u' && str[begin + 3] == L'm' && *ptr_str == begin + 4)
				{
					enum_depth = *tab_context;
					is_enum = True;
				}
				if (is_enum ||
					str[begin] == L'f' && str[begin + 1] == L'u' && str[begin + 2] == L'n' && str[begin + 3] == L'c' && *ptr_str == begin + 4 ||
					str[begin] == L'c' && str[begin + 1] == L'l' && str[begin + 2] == L'a' && str[begin + 3] == L's' && str[begin + 4] == L's' && *ptr_str == begin + 5 ||
					str[begin] == L'i' && str[begin + 1] == L'f' && *ptr_str == begin + 2 ||
					str[begin] == L's' && str[begin + 1] == L'w' && str[begin + 2] == L'i' && str[begin + 3] == L't' && str[begin + 4] == L'c' && str[begin + 5] == L'h' && *ptr_str == begin + 6 ||
					str[begin] == L'w' && str[begin + 1] == L'h' && str[begin + 2] == L'i' && str[begin + 3] == L'l' && str[begin + 4] == L'e' && *ptr_str == begin + 5 ||
					str[begin] == L'f' && str[begin + 1] == L'o' && str[begin + 2] == L'r' && *ptr_str == begin + 3 ||
					str[begin] == L't' && str[begin + 1] == L'r' && str[begin + 2] == L'y' && *ptr_str == begin + 3 ||
					str[begin] == L'b' && str[begin + 1] == L'l' && str[begin + 2] == L'o' && str[begin + 3] == L'c' && str[begin + 4] == L'k' && *ptr_str == begin + 5)
				{
					(*tab_context)++;
					if (add_end != NULL)
					{
						((S64*)add_end)[0] = 2;
						((S64*)add_end)[1] = (S64)(0x05 + end - begin);
						add_end[0x08] = L'\n';
						add_end[0x09] = L'e';
						add_end[0x0a] = L'n';
						add_end[0x0b] = L'd';
						add_end[0x0c] = L' ';
						int i;
						for (i = begin; i < end; i++)
							add_end[0x0d + i - begin] = str[i];
						add_end[0x0d + end - begin] = L'\0';
					}
				}
				else if (str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L'i' && str[begin + 3] == L'f' && *ptr_str == begin + 4 ||
					str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'd' && str[begin + 1] == L'e' && str[begin + 2] == L'f' && str[begin + 3] == L'a' && str[begin + 4] == L'u' && str[begin + 5] == L'l' && str[begin + 6] == L't' && *ptr_str == begin + 7 ||
					str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L't' && str[begin + 3] == L'c' && str[begin + 4] == L'h' && *ptr_str == begin + 5 ||
					str[begin] == L'f' && str[begin + 1] == L'i' && str[begin + 2] == L'n' && str[begin + 3] == L'a' && str[begin + 4] == L'l' && str[begin + 5] == L'l' && str[begin + 6] == L'y' && *ptr_str == begin + 7)
				{
					(*tab_context)++;
				}
			}
			break;
		}
		if (prev == AlignmentToken_None)
		{
			S64 i;
			for (i = 0; i < *tab_context; i++)
				InterpretImpl1Write(ptr_buf, buf, L'\t');
			if (new_cursor_x != NULL)
				*new_cursor_x = (S64)*ptr_buf;
		}
	}
	else
	{
		if (new_cursor_x != NULL)
			*new_cursor_x = (S64)*ptr_buf;
	}
	InterpretImpl1AlignRecursion(ptr_buf, ptr_str, 0, 0, buf, str, comment_level, flags, tab_context, &prev, cursor_x, new_cursor_x);
	buf[*ptr_buf] = L'\0';
	if (new_cursor_x != NULL && cursor_x >= (S64)*ptr_str)
		*new_cursor_x = (S64)*ptr_buf;
}

static void InterpretImpl1AlignRecursion(int* ptr_buf, int* ptr_str, int str_level, int type_level, Char* buf, const Char* str, S64* comment_level, U64* flags, S64* tab_context, EAlignmentToken* prev, S64 cursor_x, S64* new_cursor_x)
{
	while (str[*ptr_str] != L'\0')
	{
		const Char c = str[*ptr_str];
		if (*comment_level <= 0)
		{
			if (str_level > 0 && c == L'}')
			{
				*prev = AlignmentToken_None;
				break;
			}
			else if (type_level > 0 && c == L'>')
			{
				*prev = AlignmentToken_Pr;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
				(*ptr_str)++;
				break;
			}
			else if (c == L' ' || c == L'\t')
			{
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				(*ptr_str)++;
			}
			else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\' || c == L'%' && (L'a' <= str[*ptr_str + 1] && str[*ptr_str + 1] <= L'z' || L'A' <= str[*ptr_str + 1] && str[*ptr_str + 1] <= L'Z' || str[*ptr_str + 1] == L'_'))
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_Value;
				int begin = *ptr_str;
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (L'a' <= str[*ptr_str] && str[*ptr_str] <= L'z' || L'A' <= str[*ptr_str] && str[*ptr_str] <= L'Z' || str[*ptr_str] == L'_' || L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9' || str[*ptr_str] == L'@' || str[*ptr_str] == L'\\');
				if (str[begin] == L'f' && str[begin + 1] == L'u' && str[begin + 2] == L'n' && str[begin + 3] == L'c' && *ptr_str == begin + 4 ||
					str[begin] == L'l' && str[begin + 1] == L'i' && str[begin + 2] == L's' && str[begin + 3] == L't' && *ptr_str == begin + 4 ||
					str[begin] == L's' && str[begin + 1] == L't' && str[begin + 2] == L'a' && str[begin + 3] == L'c' && str[begin + 4] == L'k' && *ptr_str == begin + 5 ||
					str[begin] == L'q' && str[begin + 1] == L'u' && str[begin + 2] == L'e' && str[begin + 3] == L'u' && str[begin + 4] == L'e' && *ptr_str == begin + 5 ||
					str[begin] == L'd' && str[begin + 1] == L'i' && str[begin + 2] == L'c' && str[begin + 3] == L't' && *ptr_str == begin + 4)
				{
					while (str[*ptr_str] != L'\0')
					{
						if (str[*ptr_str] == L' ' || str[*ptr_str] == L'\t')
						{
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							(*ptr_str)++;
							continue;
						}
						if (str[*ptr_str] == L'<')
						{
							*prev = AlignmentToken_None;
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
							(*ptr_str)++;
							InterpretImpl1AlignRecursion(ptr_buf, ptr_str, str_level, type_level + 1, buf, str, comment_level, flags, tab_context, prev, cursor_x, new_cursor_x);
						}
						break;
					}
				}
			}
			else if (L'0' <= c && c <= L'9')
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_Value;
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9' || L'A' <= str[*ptr_str] && str[*ptr_str] <= L'F' || str[*ptr_str] == L'x' || str[*ptr_str] == L'.');
				if (str[*ptr_str] == L'e')
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'+' || str[*ptr_str] == L'-')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						while (L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9')
						{
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
							(*ptr_str)++;
						}
					}
				}
				else if (str[*ptr_str] == L'b')
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					while (L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
					}
				}
			}
			else if (c == L'"')
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_None;
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
						if (str[*ptr_str] == L'{')
						{
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
							(*ptr_str)++;
							InterpretImpl1AlignRecursion(ptr_buf, ptr_str, str_level + 1, type_level, buf, str, comment_level, flags, tab_context, prev, cursor_x, new_cursor_x);
							continue;
						}
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'"')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
				*prev = AlignmentToken_Value;
			}
			else if (c == L'\'')
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_None;
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'\'')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
				*prev = AlignmentToken_Value;
			}
			else if (c == L'{')
			{
				if (*prev != AlignmentToken_None)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_None;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
				(*ptr_str)++;
				(*comment_level)++;
			}
			else if (c == L';')
			{
				*prev = AlignmentToken_None;
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (str[*ptr_str] != L'\0');
			}
			else
			{
				Bool is_ope2 = False;
				int len = 1;
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
				{
					switch (c)
					{
						case L'^':
						case L'*':
						case L'/':
						case L'%':
						case L'+':
						case L'-':
						case L'~':
						case L'&':
						case L'|':
							is_ope2 = True;
							len = 1;
							break;
						case L'$':
							is_ope2 = True;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'>':
								case L'<':
									len = 2;
									break;
							}
							break;
						case L'=':
							is_ope2 = True;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'&':
								case L'$':
									len = 2;
									break;
							}
							break;
						case L'<':
							is_ope2 = True;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'>':
									len = 2;
									switch (str[*ptr_str + 2])
									{
										case L'&':
										case L'$':
											len = 3;
											break;
									}
									break;
								case L'=':
									len = 2;
									break;
							}
							break;
						case L'>':
							is_ope2 = True;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'=':
									len = 2;
									break;
							}
							break;
						case L':':
							switch (str[*ptr_str + 1])
							{
								case L'$':
								case L':':
								case L'+':
								case L'-':
								case L'*':
								case L'/':
								case L'%':
								case L'^':
								case L'~':
									is_ope2 = True;
									len = 2;
									break;
							}
							break;
					}
				}
				if (is_ope2)
				{
					if (*prev != AlignmentToken_None)
						InterpretImpl1Write(ptr_buf, buf, L' ');
					*prev = AlignmentToken_Ope2;
				}
				else
				{
					if ((*prev & (AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
						InterpretImpl1Write(ptr_buf, buf, L' ');
					switch (c)
					{
						case L'(':
						case L'[':
						case L'.':
							len = 1;
							*prev = AlignmentToken_None;
							break;
						case L')':
						case L']':
							len = 1;
							*prev = AlignmentToken_Pr;
							break;
						case L',':
							len = 1;
							*prev = AlignmentToken_Comma;
							break;
						case L':':
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L':':
									if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
										InterpretImpl1Write(ptr_buf, buf, L' ');
									*prev = AlignmentToken_Ope2;
									len = 2;
									break;
								default:
									*prev = AlignmentToken_Comma;
									break;
							}
							break;
						case L'+':
						case L'-':
						case L'!':
						case L'^':
						case L'*':
							if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
								InterpretImpl1Write(ptr_buf, buf, L' ');
							len = 1;
							*prev = AlignmentToken_None;
							break;
						case L'#':
							if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
								InterpretImpl1Write(ptr_buf, buf, L' ');
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'#':
									len = 2;
									break;
							}
							*prev = AlignmentToken_None;
							break;
						case L'?':
							switch (str[*ptr_str + 1])
							{
								case L'(':
									if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
										InterpretImpl1Write(ptr_buf, buf, L' ');
									len = 2;
									break;
							}
							*prev = AlignmentToken_None;
							break;
						default:
							*prev = AlignmentToken_None;
							break;
					}
				}
				int i;
				for (i = 0; i < len; i++)
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				}
			}
		}
		else
		{
			*prev = AlignmentToken_None;
			if (c == L'"')
			{
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'"')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
			}
			else if (c == L'\'')
			{
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'\'')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
			}
			else if (c == L';')
			{
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (str[*ptr_str] != L'\0');
			}
			else
			{
				if (c == L'{')
					(*comment_level)++;
				else if (c == L'}')
					(*comment_level)--;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
				(*ptr_str)++;
			}
		}
	}
}

static void InterpretImpl1Write(int* ptr, Char* buf, Char c)
{
	if (*ptr < AUXILIARY_BUF_SIZE)
	{
		buf[*ptr] = c;
		(*ptr)++;
	}
}

static void Interpret1Impl1UpdateCursor(S64 cursor_x, S64* new_cursor_x, int* ptr_str, int* ptr_buf)
{
	if (new_cursor_x != NULL && cursor_x == (S64)*ptr_str)
		*new_cursor_x = (S64)*ptr_buf;
}

static Char GetKeywordsRead(const Char** str)
{
	for (; ; )
	{
		if (*str == GetKeywordsEnd)
			return L'\0';
		Char c = **str;
		(*str)++;
		switch (c)
		{
		case L'{':
			GetKeywordsReadComment(str);
			return L' ';
		case L'\r':
			continue;
		case L'\t':
			return L' ';
		}
		return c;
	}
}

static void GetKeywordsReadComment(const Char** str)
{
	Char c;
	do
	{
		c = GetKeywordsRead(str);
		if (c == L'\0')
			return;
		if (c == L'"')
		{
			Bool esc = False;
			for (; ; )
			{
				c = GetKeywordsReadStrict(str);
				if (c == L'\0')
					return;
				if (esc)
				{
					if (c == L'{')
						GetKeywordsReadComment(str);
					esc = False;
					continue;
				}
				if (c == L'"')
					break;
				if (c == L'\\')
					esc = True;
			}
		}
		else if (c == L'\'')
		{
			Bool esc = False;
			for (; ; )
			{
				c = GetKeywordsReadStrict(str);
				if (c == L'\0')
					return;
				if (esc)
				{
					esc = False;
					continue;
				}
				if (c == L'\'')
					break;
				if (c == L'\\')
					esc = True;
			}
		}
		else if (c == L';')
		{
			for (; ; )
			{
				if (*str == GetKeywordsEnd)
					return;
				(*str)++;
			}
		}
	} while (c != L'}');
}

static Char GetKeywordsReadChar(const Char** str)
{
	for (; ; )
	{
		Char c = GetKeywordsRead(str);
		if (c != L' ')
			return c;
	}
}

static Char GetKeywordsReadStrict(const Char** str)
{
	for (; ; )
	{
		if (*str == GetKeywordsEnd)
			return L'\0';
		Char c = **str;
		(*str)++;
		switch (c)
		{
		case L'\t':
			continue;
		case L'\r':
			continue;
		}
		return c;
	}
}

static Bool GetKeywordsReadIdentifier(Char* buf, const Char** str, Bool skip_spaces, Bool ref)
{
	int ptr = 0;
	Char c = skip_spaces ? GetKeywordsReadChar(str) : GetKeywordsRead(str);
	if (c == L'\0')
	{
		buf[ptr] = L'\0';
		return True;
	}
	if (!(L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || ref && (c == L'@' || c == L'\\')))
	{
		(*str)--;
		buf[ptr] = L'\0';
		return False;
	}
	{
		Bool at = False;
		do
		{
			switch (c)
			{
				case L'@':
					if (at)
					{
						(*str)--;
						buf[ptr] = L'\0';
						return False;
					}
					at = True;
					break;
				case L'\\':
					if (at)
					{
						(*str)--;
						buf[ptr] = L'\0';
						return False;
					}
					break;
			}
			if (ptr == 128)
			{
				(*str)--;
				buf[ptr] = L'\0';
				return False;
			}
			buf[ptr] = c;
			ptr++;
			c = GetKeywordsRead(str);
			if (c == L'\0')
			{
				buf[ptr] = L'\0';
				return True;
			}
		} while (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || L'0' <= c && c <= L'9' || ref && (c == L'@' || c == L'\\'));
		(*str)--;
		buf[ptr] = L'\0';
		return False;
	}
}

static Bool GetKeywordsReadUntil(const Char** str, Char c)
{
	for (; ; )
	{
		Char c2 = GetKeywordsRead(str);
		if (c2 == L'\0')
			return True;
		if (c2 == c)
			return False;
	}
}

static void GetKeywordsAdd(const Char* str)
{
	Char buf[256];
	size_t len = wcslen(str);
	((S64*)buf)[0] = 2;
	((S64*)buf)[1] = (S64)len;
	wcscpy(buf + 0x08, str);
	Call1Asm(buf, GetKeywordsCallback);
}

static Bool GetKeywordsReadType(const Char** str, Char** type)
{
	Char c = GetKeywordsReadChar(str);
	if (c == L'\0')
		return True;
	if (c == L'[')
	{
		Char* tmp_type = L"";
		if (GetKeywordsReadUntil(str, L']'))
			return True;
		if (GetKeywordsReadType(str, &tmp_type))
			return True;
		*type = CatKeywordType(L"&a", tmp_type);
	}
	else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'\\' || c == L'@')
	{
		(*str)--;
		Char buf[129];
		if (GetKeywordsReadIdentifier(buf, str, True, True))
		{
			GetKeywordsAdd(L"bit16");
			GetKeywordsAdd(L"bit32");
			GetKeywordsAdd(L"bit64");
			GetKeywordsAdd(L"bit8");
			GetKeywordsAdd(L"bool");
			GetKeywordsAdd(L"char");
			GetKeywordsAdd(L"dict");
			GetKeywordsAdd(L"float");
			GetKeywordsAdd(L"func");
			GetKeywordsAdd(L"int");
			GetKeywordsAdd(L"list");
			GetKeywordsAdd(L"queue");
			GetKeywordsAdd(L"stack");
			GetKeywordsAddKeywords(L't');
		}
		else if (wcscmp(buf, L"int") == 0)
		{
			*type = L"int";
			return False;
		}
		else if (wcscmp(buf, L"float") == 0)
		{
			*type = L"float";
			return False;
		}
		else if (wcscmp(buf, L"char") == 0)
		{
			*type = L"char";
			return False;
		}
		else if (wcscmp(buf, L"bool") == 0)
		{
			*type = L"bool";
			return False;
		}
		else if (wcscmp(buf, L"bit8") == 0)
		{
			*type = L"bit8";
			return False;
		}
		else if (wcscmp(buf, L"bit16") == 0)
		{
			*type = L"bit16";
			return False;
		}
		else if (wcscmp(buf, L"bit32") == 0)
		{
			*type = L"bit32";
			return False;
		}
		else if (wcscmp(buf, L"bit64") == 0)
		{
			*type = L"bit64";
			return False;
		}
		else if (wcscmp(buf, L"list") == 0 || wcscmp(buf, L"stack") == 0 || wcscmp(buf, L"queue") == 0)
		{
			Char* tmp_type = L"";
			if (GetKeywordsReadUntil(str, L'<'))
				return True;
			if (GetKeywordsReadType(str, &tmp_type))
				return True;
			if (GetKeywordsReadUntil(str, L'>'))
				return True;
			Char prefix[3];
			prefix[0] = L'&';
			prefix[1] = buf[0];
			prefix[2] = L'\0';
			*type = CatKeywordType(prefix, tmp_type);
		}
		else if (wcscmp(buf, L"dict") == 0)
		{
			Char* tmp_type1 = L"";
			Char* tmp_type2 = L"";
			if (GetKeywordsReadUntil(str, L'<'))
				return True;
			if (GetKeywordsReadType(str, &tmp_type1))
				return True;
			if (GetKeywordsReadUntil(str, L','))
				return True;
			if (GetKeywordsReadType(str, &tmp_type2))
				return True;
			if (GetKeywordsReadUntil(str, L'>'))
				return True;
			Char new_type[1024] = L"&d";
			wcscat(new_type, tmp_type1);
			wcscat(new_type, L"|");
			wcscat(new_type, tmp_type2);
			*type = NewKeywordType(new_type);
		}
		else if (wcscmp(buf, L"func") == 0)
		{
			if (GetKeywordsReadUntil(str, L'<'))
				return True;
			if (GetKeywordsReadUntil(str, L'('))
				return True;
			c = GetKeywordsReadChar(str);
			if (c != L')')
			{
				for (; ; )
				{
					Char* tmp_type = L"";
					if (GetKeywordsReadType(str, &tmp_type))
						return True;
					c = GetKeywordsReadChar(str);
					if (c == L'\0')
						return True;
					if (c == L')')
						break;
					if (c != L',')
					{
						(*str)--;
						return False;
					}
				}
			}
			c = GetKeywordsReadChar(str);
			if (c == L':')
			{
				Char* tmp_type = L"";
				if (GetKeywordsReadType(str, &tmp_type))
					return True;
			}
			*type = L"";
		}
		else
			return False;
	}
	return False;
}

static Bool GetKeywordsReadExpr(const Char** str, Char** type)
{
	if (GetKeywordsReadExprThree(str, type))
		return True;
	Char c = GetKeywordsReadChar(str);
	if (c == L'\0')
		return True;
	if (c == L':')
	{
		c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		switch (c)
		{
			case L':':
			case L'+':
			case L'-':
			case L'*':
			case L'/':
			case L'%':
			case L'^':
			case L'~':
			case L'$':
				if (GetKeywordsReadExpr(str, type))
					return True;
				break;
			default:
				(*str)--;
				break;
		}
		return False;
	}
	(*str)--;
	return False;
}

static Bool GetKeywordsReadExprThree(const Char** str, Char** type)
{
	if (GetKeywordsReadExprOr(str, type))
		return True;
	for (; ; )
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		if (c == L'?')
		{
			if (GetKeywordsReadUntil(str, L'('))
				return True;
			if (GetKeywordsReadExpr(str, type))
				return True;
			if (GetKeywordsReadUntil(str, L','))
				return True;
			if (GetKeywordsReadExpr(str, type))
				return True;
			if (GetKeywordsReadUntil(str, L')'))
				return True;
		}
		else
		{
			(*str)--;
			break;
		}
	}
	return False;
}

static Bool GetKeywordsReadExprOr(const Char** str, Char** type)
{
	if (GetKeywordsReadExprAnd(str, type))
		return True;
	for (; ; )
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		if (c == L'|')
		{
			if (GetKeywordsReadExprAnd(str, type))
				return True;
		}
		else
		{
			(*str)--;
			break;
		}
	}
	return False;
}

static Bool GetKeywordsReadExprAnd(const Char** str, Char** type)
{
	if (GetKeywordsReadExprCmp(str, type))
		return True;
	for (; ; )
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		if (c == L'&')
		{
			if (GetKeywordsReadExprCmp(str, type))
				return True;
		}
		else
		{
			(*str)--;
			break;
		}
	}
	return False;
}

static Bool GetKeywordsReadExprCmp(const Char** str, Char** type)
{
	if (GetKeywordsReadExprCat(str, type))
		return True;
	Bool end_flag = False;
	do
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		switch (c)
		{
			case L'<':
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return True;
				if (c == L'=')
				{
					if (GetKeywordsReadExprCat(str, type))
						return True;
				}
				else if (c == L'>')
				{
					c = GetKeywordsReadChar(str);
					if (c == L'\0')
						return True;
					if (c == L'&')
					{
						if (GetKeywordsReadExprCat(str, type))
							return True;
					}
					else if (c == L'$')
					{
						Char* tmp_type = L"";
						if (GetKeywordsReadType(str, &tmp_type))
							return True;
					}
					else
					{
						(*str)--;
						if (GetKeywordsReadExprCat(str, type))
							return True;
					}
				}
				else
				{
					(*str)--;
					if (GetKeywordsReadExprCat(str, type))
						return True;
				}
				*type = L"bool";
				break;
			case L'>':
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return True;
				if (c != L'=')
					(*str)--;
				if (GetKeywordsReadExprCat(str, type))
					return True;
				*type = L"bool";
				break;
			case L'=':
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return True;
				if (c == L'&')
				{
					if (GetKeywordsReadExprCat(str, type))
						return True;
				}
				else if (c == L'$')
				{
					Char* tmp_type = L"";
					if (GetKeywordsReadType(str, &tmp_type))
						return True;
				}
				else
				{
					(*str)--;
					if (GetKeywordsReadExprCat(str, type))
						return True;
				}
				*type = L"bool";
				break;
			default:
				(*str)--;
				end_flag = True;
				break;
		}
	} while (!end_flag);
	return False;
}

static Bool GetKeywordsReadExprCat(const Char** str, Char** type)
{
	if (GetKeywordsReadExprAdd(str, type))
		return True;
	for (; ; )
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		if (c == L'~')
		{
			if (GetKeywordsReadExprAdd(str, type))
				return True;
		}
		else
		{
			(*str)--;
			break;
		}
	}
	return False;
}

static Bool GetKeywordsReadExprAdd(const Char** str, Char** type)
{
	if (GetKeywordsReadExprMul(str, type))
		return True;
	for (; ; )
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		if (c == L'+' || c == L'-')
		{
			if (GetKeywordsReadExprMul(str, type))
				return True;
		}
		else
		{
			(*str)--;
			break;
		}
	}
	return False;
}

static Bool GetKeywordsReadExprMul(const Char** str, Char** type)
{
	if (GetKeywordsReadExprPlus(str, type))
		return True;
	Bool end_flag = False;
	do
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		switch (c)
		{
			case L'*':
			case L'/':
			case L'%':
				if (GetKeywordsReadExprPlus(str, type))
					return True;
			default:
				(*str)--;
				end_flag = True;
				break;
		}
	} while (!end_flag);
	return False;
}

static Bool GetKeywordsReadExprPlus(const Char** str, Char** type)
{
	const Char* old = *str;
	if (GetKeywordsReadExprPow(str, type))
		return True;
	if (old != *str)
		return False;
	Char c = GetKeywordsReadChar(str);
	if (c == L'\0')
		return True;
	switch (c)
	{
		case L'#':
			{
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return True;
				if (c == L'[')
				{
					for (; ; )
					{
						c = GetKeywordsReadChar(str);
						if (c == L'\0')
							return True;
						if (c == L']')
							break;
						if (c != L',')
						{
							(*str)--;
							return False;
						}
					}
					{
						Char* tmp_type = L"";
						if (GetKeywordsReadType(str, &tmp_type))
							return True;
						*type = CatKeywordType(L"&a", tmp_type);
					}
				}
				else if (c == L'#')
				{
					if (GetKeywordsReadExprPlus(str, type))
						return True;
				}
				else
				{
					(*str)--;
					Char* tmp_type = L"";
					if (GetKeywordsReadType(str, &tmp_type))
						return True;
					*type = tmp_type;
				}
			}
			break;
		case L'+':
		case L'-':
		case L'!':
		case L'^':
			if (GetKeywordsReadExprPlus(str, type))
				return True;
			break;
		default:
			(*str)--;
			return False;
	}
	return False;
}

static Bool GetKeywordsReadExprPow(const Char** str, Char** type)
{
	const Char* old = *str;
	if (GetKeywordsReadExprCall(str, type))
		return True;
	if (old == *str)
		return False; // Interpret as a unary operator.
	Char c = GetKeywordsReadChar(str);
	if (c == L'\0')
		return True;
	if (c == L'^')
	{
		if (GetKeywordsReadExprPlus(str, type))
			return True;
	}
	else
		(*str)--;
	return False;
}

static Bool GetKeywordsReadExprCall(const Char** str, Char** type)
{
	const Char* old = *str;
	if (GetKeywordsReadExprValue(str, type))
		return True;
	if (old == *str)
		return False;
	Bool end_flag = False;
	do
	{
		Char c = GetKeywordsReadChar(str);
		if (c == L'\0')
			return True;
		switch (c)
		{
			case L'(':
				if ((*type)[0] == L'&' && (*type)[1] == L'F')
				{
					void* type_ptr;
					StrToPtr(&type_ptr, (*type) + 2);
					KeywordHintFunc = (SAstFunc*)type_ptr;
				}
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return True;
				if (c != L')')
				{
					(*str)--;
					for (; ; )
					{
						Bool skip_var = False;
						c = GetKeywordsReadChar(str);
						if (c == L'\0')
							return True;
						if (c == L'&')
						{
							c = GetKeywordsReadChar(str);
							if (c == L'\0')
								return True;
							if (c == L',' || c == L')')
								skip_var = True;
						}
						(*str)--;
						if (!skip_var)
						{
							Char* tmp_type = L"";
							if (GetKeywordsReadExpr(str, &tmp_type))
								return True;
						}
						c = GetKeywordsReadChar(str);
						if (c == L'\0')
							return True;
						if (c == L')')
							break;
						if (c != L',')
						{
							(*str)--;
							*type = L"";
							return False;
						}
					}
				}
				if ((*type)[0] == L'&' && (*type)[1] == L'F')
				{
					void* type_ptr;
					StrToPtr(&type_ptr, (*type) + 2);
					SAstType* ret_type = ((SAstFunc*)type_ptr)->Ret;
					if (ret_type == NULL)
						*type = L"";
					else
						*type = GetKeywordTypeRecursion(ret_type);
				}
				else if ((*type)[0] == L'&' && (*type)[1] == L'f')
				{
					void* type_ptr;
					StrToPtr(&type_ptr, (*type) + 2);
					SAstType* ret_type = ((SAstTypeFunc*)type_ptr)->Ret;
					if (ret_type == NULL)
						*type = L"";
					else
						*type = GetKeywordTypeRecursion(ret_type);
				}
				else
					*type = L"";
				break;
			case L'[':
				{
					Char* tmp_type = L"";
					if (GetKeywordsReadExpr(str, &tmp_type))
						return True;
					if (GetKeywordsReadUntil(str, L']'))
						return True;
					if ((*type)[0] == L'&' && (*type)[1] == L'a')
						*type = NewKeywordType((*type) + 2);
					break;
				}
			case L'.':
				{
					Char buf[129];
					if (GetKeywordsReadIdentifier(buf, str, True, False))
					{
						GetKeywordsAddMember(*type);
						return True;
					}
					*type = L"";
				}
				break;
			case L'$':
				c = GetKeywordsReadChar(str);
				if (c == L'\0')
					return True;
				if (c == L'>' || c == L'<')
				{
					Char* tmp_type = L"";
					if (GetKeywordsReadType(str, &tmp_type))
						return True;
					*type = tmp_type;
				}
				else
				{
					(*str)--;
					Char* tmp_type = L"";
					if (GetKeywordsReadType(str, &tmp_type))
						return True;
					*type = tmp_type;
				}
				break;
			default:
				(*str)--;
				end_flag = True;
				break;
		}
	} while (!end_flag);
	return False;
}

static Bool GetKeywordsReadExprValue(const Char** str, Char** type)
{
	const Char* old = *str;
	Char c = GetKeywordsReadChar(str);
	if (c == L'\0')
		return True;
	switch (c)
	{
		case L'"':
			{
				Bool escape = False;
				for (; ; )
				{
					c = GetKeywordsReadStrict(str);
					if (c == L'\0')
						return True;
					if (escape)
					{
						if (c == L'{')
						{
							Char* tmp_type = L"";
							if (GetKeywordsReadExpr(str, &tmp_type))
								return True;
							if (GetKeywordsReadUntil(str, L'}'))
								return True;
						}
						escape = False;
						continue;
					}
					if (c == L'"')
						break;
					if (c == L'\\')
						escape = True;
				}
				*type = L"&achar";
			}
			return False;
		case L'\'':
			{
				Bool escape = False;
				for (; ; )
				{
					c = GetKeywordsReadStrict(str);
					if (c == L'\0')
						return True;
					if (escape)
					{
						escape = False;
						continue;
					}
					if (c == L'\'')
						break;
					if (c == L'\\')
						escape = True;
				}
				*type = L"char";
			}
			return False;
		case L'(':
			if (GetKeywordsReadExpr(str, type))
				return True;
			if (GetKeywordsReadUntil(str, L')'))
				return True;
			return False;
		case L'[':
			c = GetKeywordsReadStrict(str);
			if (c == L'\0')
				return True;
			if (c != L']')
			{
				(*str)--;
				for (; ; )
				{
					Char* tmp_type = L"";
					if (GetKeywordsReadExpr(str, &tmp_type))
						return True;
					if ((*type)[0] == L'\0' && tmp_type[0] != L'\0')
						*type = CatKeywordType(L"&a", tmp_type);
					c = GetKeywordsReadStrict(str);
					if (c == L'\0')
						return True;
					if (c == L']')
						break;
					if (c != L',')
					{
						(*str)--;
						return False;
					}
				}
			}
			return False;
		case L'%':
			{
				Char buf[129];
				if (GetKeywordsReadIdentifier(buf, str, False, False))
				{
					GetKeywordsAddEnum();
					return True;
				}
				*type = L"&e";
			}
			return False;
		default:
			if (L'0' <= c && c <= L'9')
				return GetKeywordsReadExprNumber(str, type, c);
			else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\')
			{
				(*str)--;
				Char buf[129];
				if (GetKeywordsReadIdentifier(buf, str, True, True))
				{
					GetKeywordsAdd(L"dbg");
					GetKeywordsAdd(L"env");
					GetKeywordsAdd(L"false");
					GetKeywordsAdd(L"inf");
					GetKeywordsAdd(L"null");
					GetKeywordsAdd(L"super");
					GetKeywordsAdd(L"true");
					GetKeywordsAddKeywords(L'e');
					return True;
				}
				if (buf[0] == L'd' && buf[1] == L'b' && buf[2] == L'g')
					*type = L"bool";
				else if (buf[0] == L'f' && buf[1] == L'a' && buf[2] == L'l' && buf[3] == L's' && buf[4] == L'e')
					*type = L"bool";
				else if (buf[0] == L'i' && buf[1] == L'n' && buf[2] == L'f')
					*type = L"float";
				else if (buf[0] == L'n' && buf[1] == L'u' && buf[2] == L'l' && buf[3] == L'l')
					*type = L"";
				else if (buf[0] == L's' && buf[1] == L'u' && buf[2] == L'p' && buf[3] == L'e' && buf[4] == L'r')
					*type = L"";
				else if (buf[0] == L't' && buf[1] == L'r' && buf[2] == L'u' && buf[3] == L'e')
					*type = L"bool";
				else
					*type = GetKeywordType(buf);
				return False;
			}
			break;
	}
	*str = old;
	return False;
}

static Bool GetKeywordsReadExprNumber(const Char** str, Char** type, Char c)
{
	*type = L"int";
	for (; ; )
	{
		if (!(c == L'x' || c == L'.' || L'0' <= c && c <= L'9' || L'A' <= c && c <= L'F'))
			break;
		if (c == L'.')
			*type = L"float";
		c = GetKeywordsRead(str);
		if (c == L'\0')
			return True;
	}
	if (c == L'e')
	{
		c = GetKeywordsRead(str);
		if (c == L'\0')
			return True;
		if (c != L'+' && c != L'-')
		{
			(*str)--;
			return False;
		}
		do
		{
			c = GetKeywordsRead(str);
			if (c == L'\0')
				return True;
		} while (L'0' <= c && c <= L'9');
		(*str)--;
	}
	else if (c == L'b')
	{
		c = GetKeywordsRead(str);
		switch (c)
		{
			case L'8':
				*type = L"bit8";
				break;
			case L'1':
				if (GetKeywordsReadUntil(str, L'6'))
					return True;
				*type = L"bit16";
				break;
			case L'3':
				if (GetKeywordsReadUntil(str, L'2'))
					return True;
				*type = L"bit32";
				break;
			case L'6':
				if (GetKeywordsReadUntil(str, L'4'))
					return True;
				*type = L"bit64";
				break;
			default:
				(*str)--;
				return False;
		}
	}
	else
		(*str)--;
	return False;
}

static void GetKeywordsAddEnum(void)
{
	if (GetKeywordsKeywordList == NULL)
		return;
	int i;
	for (i = 0; i < GetKeywordsKeywordListNum; i++)
	{
		const SKeywordListItem* item = GetKeywordsKeywordList[i];
		if (item->Name[0] != L'%')
			continue;
		GetKeywordsAdd(GetKeywordsKeywordList[i]->Name);
	}
}

static void GetKeywordsAddMember(const Char* type)
{
	if (type[0] == L'\0')
		return;
	if (type[0] == L'b' && type[1] == L'i' && type[2] == L't')
	{
		if (type[3] == L'1' && type[4] == L'6' || type[3] == L'3' && type[4] == L'2' || type[3] == L'6' && type[4] == L'4' || type[3] == L'8')
		{
			GetKeywordsAdd(L"and");
			GetKeywordsAdd(L"endian");
			GetKeywordsAdd(L"not");
			GetKeywordsAdd(L"or");
			GetKeywordsAdd(L"sar");
			GetKeywordsAdd(L"shl");
			GetKeywordsAdd(L"shr");
			GetKeywordsAdd(L"toStr");
			GetKeywordsAdd(L"xor");
		}
	}
	else if (type[0] == L'b' && type[1] == L'o' && type[2] == L'o' && type[3] == L'l')
		GetKeywordsAdd(L"toStr");
	else if (type[0] == L'c' && type[1] == L'h' && type[2] == L'a' && type[3] == L'r')
	{
		GetKeywordsAdd(L"offset");
		GetKeywordsAdd(L"toStr");
	}
	else if (type[0] == L'f' && type[1] == L'l' && type[2] == L'o' && type[3] == L'a' && type[4] == L't')
	{
		GetKeywordsAdd(L"abs");
		GetKeywordsAdd(L"clamp");
		GetKeywordsAdd(L"clampMax");
		GetKeywordsAdd(L"clampMin");
		GetKeywordsAdd(L"sign");
		GetKeywordsAdd(L"toStr");
		GetKeywordsAdd(L"toStrFmt");
	}
	else if (type[0] == L'i' && type[1] == L'n' && type[2] == L't')
	{
		GetKeywordsAdd(L"abs");
		GetKeywordsAdd(L"clamp");
		GetKeywordsAdd(L"clampMax");
		GetKeywordsAdd(L"clampMin");
		GetKeywordsAdd(L"sign");
		GetKeywordsAdd(L"toStr");
		GetKeywordsAdd(L"toStrFmt");
	}
	else if (type[0] == L'&')
	{
		if (type[1] == L'a')
		{
			if (type[2] == L'c' && type[3] == L'h' && type[4] == L'a' && type[5] == L'r')
			{
				GetKeywordsAdd(L"findStr");
				GetKeywordsAdd(L"findStrEx");
				GetKeywordsAdd(L"findStrLast");
				GetKeywordsAdd(L"lower");
				GetKeywordsAdd(L"replace");
				GetKeywordsAdd(L"split");
				GetKeywordsAdd(L"toBit64");
				GetKeywordsAdd(L"toFloat");
				GetKeywordsAdd(L"toInt");
				GetKeywordsAdd(L"toStr");
				GetKeywordsAdd(L"trim");
				GetKeywordsAdd(L"trimLeft");
				GetKeywordsAdd(L"trimRight");
				GetKeywordsAdd(L"upper");
			}
			else if (type[2] == L'&' && type[3] == L'a' && type[4] == L'c' && type[5] == L'h' && type[6] == L'a' && type[7] == L'r')
				GetKeywordsAdd(L"join");
			GetKeywordsAdd(L"fill");
			GetKeywordsAdd(L"find");
			GetKeywordsAdd(L"findBin");
			GetKeywordsAdd(L"findLast");
			GetKeywordsAdd(L"max");
			GetKeywordsAdd(L"min");
			GetKeywordsAdd(L"repeat");
			GetKeywordsAdd(L"reverse");
			GetKeywordsAdd(L"shuffle");
			GetKeywordsAdd(L"sort");
			GetKeywordsAdd(L"sortDesc");
			GetKeywordsAdd(L"sub");
		}
		else if (type[1] == L'c')
		{
			void* ast_ptr;
			StrToPtr(&ast_ptr, type + 2);
			SAstClass* ast = (SAstClass*)ast_ptr;
			SListNode* ptr = ast->Items->Top;
			while (ptr != NULL)
			{
				const SAstClassItem* item = (const SAstClassItem*)ptr->Data;
				if (item->Public)
				{
					const Char* name;
					if (item->Def->TypeId == AstTypeId_Var)
						name = ((SAst*)((SAstVar*)item->Def)->Var)->Name;
					else if (item->Def->TypeId == AstTypeId_Const)
						name = ((SAst*)((SAstConst*)item->Def)->Var)->Name;
					else
						name = item->Def->Name;
					if (name != NULL)
						GetKeywordsAdd(name);
				}
				ptr = ptr->Next;
			}
		}
		else if (type[1] == L'd')
		{
			GetKeywordsAdd(L"add");
			GetKeywordsAdd(L"del");
			GetKeywordsAdd(L"exist");
			GetKeywordsAdd(L"forEach");
			GetKeywordsAdd(L"get");
			GetKeywordsAdd(L"toArrayKey");
			GetKeywordsAdd(L"toArrayValue");
		}
		else if (type[1] == L'e')
		{
			GetKeywordsAdd(L"and");
			GetKeywordsAdd(L"not");
			GetKeywordsAdd(L"or");
			GetKeywordsAdd(L"xor");
		}
		else if (type[1] == L'l')
		{
			GetKeywordsAdd(L"add");
			GetKeywordsAdd(L"del");
			GetKeywordsAdd(L"delNext");
			GetKeywordsAdd(L"find");
			GetKeywordsAdd(L"findLast");
			GetKeywordsAdd(L"get");
			GetKeywordsAdd(L"getOffset");
			GetKeywordsAdd(L"head");
			GetKeywordsAdd(L"ins");
			GetKeywordsAdd(L"moveOffset");
			GetKeywordsAdd(L"next");
			GetKeywordsAdd(L"prev");
			GetKeywordsAdd(L"sort");
			GetKeywordsAdd(L"sortDesc");
			GetKeywordsAdd(L"tail");
			GetKeywordsAdd(L"term");
			GetKeywordsAdd(L"termOffset");
			GetKeywordsAdd(L"toArray");
		}
		else if (type[1] == L'q' || type[1] == L's')
		{
			GetKeywordsAdd(L"add");
			GetKeywordsAdd(L"get");
			GetKeywordsAdd(L"peek");
		}
	}
}

static void GetKeywordsAddKeywords(Char kind)
{
	if (GetKeywordsKeywordList == NULL)
		return;
	int i;
	Char buf[256];
	for (i = 0; i < GetKeywordsKeywordListNum; i++)
	{
		const SKeywordListItem* item = GetKeywordsKeywordList[i];
		if (item->Name[0] == L'%' || item->Name[0] == L'.')
			continue;
		switch (kind)
		{
			case L't': // Types
				if (item->Ast->TypeId != AstTypeId_Class && item->Ast->TypeId != AstTypeId_Enum && item->Ast->TypeId != AstTypeId_Alias)
					continue;
				break;
			case L'e': // Expressions
				if (item->Ast->TypeId == AstTypeId_Class || item->Ast->TypeId == AstTypeId_Enum || item->Ast->TypeId == AstTypeId_Alias)
					continue;
				break;
			case L'c': // Class Names
				if (item->Ast->TypeId != AstTypeId_Class)
					continue;
				break;
			default:
				ASSERT(False);
				break;
		}
		if (item->Name[0] == L'@')
		{
			if (wcscmp(GetKeywordsSrcName, item->Ast->Pos->SrcName) == 0)
				GetKeywordsAdd(item->Name);
			else
			{
				if (!item->Ast->Public)
					continue;
				wcscpy(buf, item->Ast->Pos->SrcName);
				wcscat(buf, item->Name);
				GetKeywordsAdd(buf);
			}
		}
		else if (wcscmp(GetKeywordsSrcName, item->Ast->Pos->SrcName) == 0 && *item->First - 1 <= GetKeywordsCursorY && GetKeywordsCursorY <= *item->Last - 1)
			GetKeywordsAdd(item->Name);
	}
}

static Char* NewKeywordType(const Char* keyword_type)
{
	size_t len = wcslen(keyword_type);
	Char* buf = (Char*)AllocMem(sizeof(Char) * (len + 1));
	memcpy(buf, keyword_type, sizeof(Char) * (len + 1));
	SKeywordTypeList* item = (SKeywordTypeList*)AllocMem(sizeof(SKeywordTypeList));
	item->Next = NULL;
	item->Type = buf;
	KeywordTypeListBottom->Next = item;
	KeywordTypeListBottom = item;
	return buf;
}

static Char* CatKeywordType(const Char* keyword_type1, const Char* keyword_type2)
{
	Char keyword_type[1024];
	wcscpy(keyword_type, keyword_type1);
	wcscat(keyword_type, keyword_type2);
	return NewKeywordType(keyword_type);
}

static Char* GetKeywordType(const Char* identifier)
{
	if (GetKeywordsKeywordList == NULL)
		return L"";
	const SAst* ast = NULL;
	int i;
	for (i = 0; i < GetKeywordsKeywordListNum; i++)
	{
		const SKeywordListItem* item = GetKeywordsKeywordList[i];
		if (item->Name[0] == L'.' || item->Name[0] == L'%')
			continue;
		if (item->Name[0] == L'@')
		{
			if (wcscmp(GetKeywordsSrcName, item->Ast->Pos->SrcName) == 0)
			{
				if (wcscmp(identifier, item->Name) == 0)
				{
					ast = item->Ast;
					break;
				}
			}
			else
			{
				if (!item->Ast->Public)
					continue;
				Char buf[1024];
				wcscpy(buf, item->Ast->Pos->SrcName);
				wcscat(buf, item->Name);
				if (wcscmp(identifier, buf) == 0)
				{
					ast = item->Ast;
					break;
				}
			}
		}
		else if (wcscmp(GetKeywordsSrcName, item->Ast->Pos->SrcName) == 0 && *item->First - 1 <= GetKeywordsCursorY && GetKeywordsCursorY <= *item->Last - 1 && wcscmp(identifier, item->Name) == 0)
		{
			ast = item->Ast;
			break;
		}
	}
	if (ast == NULL)
		return L"";
	SAstType* type = NULL;
	if (ast->TypeId == AstTypeId_Arg)
		type = ((SAstArg*)ast)->Type;
	else if (ast->TypeId == AstTypeId_Class)
	{
		Char buf[1024];
		buf[0] = L'&';
		buf[1] = L'c';
		PtrToStr(buf + 2, &ast);
		buf[10] = L'\0';
		return NewKeywordType(buf);
	}
	else if (ast->TypeId == AstTypeId_Func)
	{
		Char buf[1024];
		buf[0] = L'&';
		buf[1] = L'F';
		PtrToStr(buf + 2, &ast);
		buf[10] = L'\0';
		return NewKeywordType(buf);
	}
	if (type == NULL)
		return L"";
	return GetKeywordTypeRecursion(type);
}

static Char* GetKeywordTypeRecursion(const SAstType* type)
{
	switch (((SAst*)type)->TypeId)
	{
		case AstTypeId_TypeArray:
			return CatKeywordType(L"&a", GetKeywordTypeRecursion(((SAstTypeArray*)type)->ItemType));
		case AstTypeId_TypeBit:
			switch (((SAstTypeBit*)type)->Size)
			{
				case 1:
					return L"bit8";
				case 2:
					return L"bit16";
				case 4:
					return L"bit32";
				case 8:
					return L"bit64";
			}
			break;
		case AstTypeId_TypeFunc:
			{
				Char buf[1024];
				buf[0] = L'&';
				buf[1] = L'f';
				PtrToStr(buf + 2, type);
				buf[10] = L'\0';
				return NewKeywordType(buf);
			}
		case AstTypeId_TypeGen:
			switch (((SAstTypeGen*)type)->Kind)
			{
				case AstTypeGenKind_List:
					return CatKeywordType(L"&l", GetKeywordTypeRecursion(((SAstTypeGen*)type)->ItemType));
				case AstTypeGenKind_Stack:
					return CatKeywordType(L"&s", GetKeywordTypeRecursion(((SAstTypeGen*)type)->ItemType));
				case AstTypeGenKind_Queue:
					return CatKeywordType(L"&q", GetKeywordTypeRecursion(((SAstTypeGen*)type)->ItemType));
			}
			break;
		case AstTypeId_TypeDict:
			{
				Char buf[1024];
				wcscpy(buf, L"&d");
				wcscat(buf, GetKeywordTypeRecursion(((SAstTypeDict*)type)->ItemTypeKey));
				wcscpy(buf, L"|");
				wcscat(buf, GetKeywordTypeRecursion(((SAstTypeDict*)type)->ItemTypeValue));
				return NewKeywordType(buf);
			}
		case AstTypeId_TypePrim:
			switch (((SAstTypePrim*)type)->Kind)
			{
				case AstTypePrimKind_Int: return L"int";
				case AstTypePrimKind_Float: return L"float";
				case AstTypePrimKind_Char: return L"char";
				case AstTypePrimKind_Bool: return L"bool";
			}
			break;
		case AstTypeId_TypeUser:
			return GetKeywordType(((SAst*)type)->RefName);
	}
	return L"";
}

static void PtrToStr(Char* str, const void* ptr)
{
	const U8* src = (const U8*)ptr;
	U8* dst = (U8*)str;
	int i;
	for (i = 0; i < 8; i++)
	{
		U8 value = src[i];
		dst[i * 2 + 0] = ((value & 0x0f) << 4) | 0x0f;
		dst[i * 2 + 1] = (value & 0xf0) | 0x0f;
	}
}

static void StrToPtr(void* ptr, const Char* str)
{
	const U8* src = (const U8*)str;
	U8* dst = (U8*)ptr;
	int i;
	for (i = 0; i < 8; i++)
		dst[i] = (src[i * 2 + 0] >> 4) | (src[i * 2 + 1] & 0xf0);
}

static void GetKeywordHintTypeNameRecursion(size_t* len, Char* buf, const SAstType* type)
{
	switch (((SAst*)type)->TypeId)
	{
		case AstTypeId_TypeArray:
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"[]");
			GetKeywordHintTypeNameRecursion(len, buf, ((SAstTypeArray*)type)->ItemType);
			return;
		case AstTypeId_TypeBit:
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"bit%d", ((SAstTypeBit*)type)->Size * 8);
			return;
		case AstTypeId_TypeFunc:
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"func<(");
			{
				SListNode* ptr = ((SAstTypeFunc*)type)->Args->Top;
				while (ptr != NULL)
				{
					if (ptr != ((SAstTypeFunc*)type)->Args->Top)
					{
						if (*len < AUXILIARY_BUF_SIZE)
							*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L", ");
					}
					SAstTypeFuncArg* arg = (SAstTypeFuncArg*)ptr->Data;
					GetKeywordHintTypeNameRecursion(len, buf, arg->Arg);
					ptr = ptr->Next;
				}
			}
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L")");
			if (((SAstTypeFunc*)type)->Ret != NULL)
			{
				if (*len < AUXILIARY_BUF_SIZE)
					*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L": ");
				GetKeywordHintTypeNameRecursion(len, buf, ((SAstTypeFunc*)type)->Ret);
			}
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L">");
			return;
		case AstTypeId_TypeGen:
			switch (((SAstTypeGen*)type)->Kind)
			{
				case AstTypeGenKind_List:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"list<");
					break;
				case AstTypeGenKind_Stack:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"stack<");
					break;
				case AstTypeGenKind_Queue:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"queue<");
					break;
			}
			GetKeywordHintTypeNameRecursion(len, buf, ((SAstTypeGen*)type)->ItemType);
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L">");
			return;
		case AstTypeId_TypeDict:
			{
				if (*len < AUXILIARY_BUF_SIZE)
					*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"dict<");
				GetKeywordHintTypeNameRecursion(len, buf, ((SAstTypeDict*)type)->ItemTypeKey);
				if (*len < AUXILIARY_BUF_SIZE)
					*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L", ");
				GetKeywordHintTypeNameRecursion(len, buf, ((SAstTypeDict*)type)->ItemTypeValue);
				if (*len < AUXILIARY_BUF_SIZE)
					*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L">");
				return;
			}
		case AstTypeId_TypePrim:
			switch (((SAstTypePrim*)type)->Kind)
			{
				case AstTypePrimKind_Int:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"int");
					return;
				case AstTypePrimKind_Float:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"float");
					return;
				case AstTypePrimKind_Char:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"char");
					return;
				case AstTypePrimKind_Bool:
					if (*len < AUXILIARY_BUF_SIZE)
						*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"bool");
					return;
			}
			break;
		case AstTypeId_TypeUser:
			if (*len < AUXILIARY_BUF_SIZE)
				*len += swprintf(buf + *len, AUXILIARY_BUF_SIZE - *len, L"%s", ((SAst*)type)->RefName);
			return;
	}
}
