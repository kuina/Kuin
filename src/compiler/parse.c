#include "parse.h"

#include "ast.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "pos.h"
#include "stack.h"
#include "util.h"

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
	L"false",
	L"finally",
	L"float",
	L"for",
	L"foreach",
	L"func",
	L"if",
	L"ifdef",
	L"inf",
	L"int",
	L"list",
	L"me",
	L"null",
	L"queue",
	L"ret",
	L"rls",
	L"skip",
	L"stack",
	L"switch",
	L"throw",
	L"to",
	L"true",
	L"try",
	L"var",
	L"while",
};

static FILE*(*FuncWfopen)(const Char*, const Char*);
static int(*FuncFclose)(FILE*);
static U16(*FuncFgetwc)(FILE*);
static size_t(*FuncSize)(FILE*);
static SDict* Srces;
static SDict* Srces2;
static const SOption* Option;
static FILE* FilePtr;
static const Char* SrcName;
static int Row;
static int Col;
static Char FileBuf;
static Char FileBufTmp; // For single line comments and line breaking.
static Bool IsLast;
static SStack* Scope;
static U32 UniqueCnt;
static Bool LocalErr;

static Bool IsReserved(const Char* word);
static const void* ParseSrc(const Char* src_name, const void* ast, void* param);
static Char ReadBuf(void);
static Char Read(void);
static void ReadComment(void);
static Char ReadChar(void);
static Char ReadStrict(void);
static const Char* ReadIdentifier(Bool skip_spaces, Bool ref);
static const Char* ReadFuncAttr(void);
static void ReadUntilRet(void);
static void InitAst(SAst* ast, EAstTypeId type_id, const SPos* pos, const Char* name, Bool set_parent, Bool init_refeds);
static void InitAstExpr(SAstExpr* ast, EAstTypeId type_id, const SPos* pos);
static void AddScopeName(SAst* ast, Bool refuse_reserved);
static void AssertNextChar(Char c, Bool skip_spaces);
static void NextCharErr(Char c, Char c2);
static void AddScopeRefeds(SAst* ast);
static void PushDummyScope(SAst* ast);
static SAstArg* MakeBlockVar(int row, int col);
static void ObtainBlockName(SAst* ast);
static SAstExprValue* ObtainPrimValue(const SPos* pos, EAstTypePrimKind kind, const void* value);
static SAstExprValue* ObtainStrValue(const SPos* pos, const Char* value);
static Char EscChar(Char c);
static Bool IsCorrectSrcName(const Char* name);
static SAst* ParseSysCache(void);
static SAstRoot* ParseRoot(void);
static SAstFunc* ParseFunc(const Char* parent_class);
static SAstVar* ParseVar(EAstArgKind kind, const Char* parent_class);
static SAstConst* ParseConst(void);
static SAstAlias* ParseAlias(void);
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
static SAstStat* ParseStatForEach(int row, int col);
static SAstStat* ParseStatTry(int row, int col);
static SAstStat* ParseStatCatch(int row, int col, const SAst* block);
static SAstStat* ParseStatFinally(int row, int col, const SAst* block);
static SAstStat* ParseStatThrow(void);
static SAstStat* ParseStatIfDef(void);
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

SDict* Parse(FILE*(*func_wfopen)(const Char*, const Char*), int(*func_fclose)(FILE*), U16(*func_fgetwc)(FILE*), size_t(*func_size)(FILE*), const SOption* option)
{
	Bool end_flag = False;
	FuncWfopen = func_wfopen;
	FuncFclose = func_fclose;
	FuncFgetwc = func_fgetwc;
	FuncSize = func_size;

#ifdef _DEBUG
	{
		int len = (int)(sizeof(Reserved) / sizeof(Char*));
		int i;
		for (i = 0; i < len - 1; i++)
			ASSERT(wcscmp(Reserved[i], Reserved[i + 1]) < 0);
	}
#endif

	Srces = NULL;
	Srces = DictAdd(Srces, NewStr(NULL, L"\\%s", option->SrcName), NULL);
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
		case Env_Web:
			// TODO:
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

static Bool IsReserved(const Char* word)
{
	return BinSearch(Reserved, (int)(sizeof(Reserved) / sizeof(Char*)), word) != -1;
}

static const void* ParseSrc(const Char* src_name, const void* ast, void* param)
{
	Bool* end_flag = (Bool*)param;
	if (ast != NULL)
	{
		Srces2 = DictAdd(Srces2, src_name, ast);
		return ast;
	}

	if (src_name[0] == L'\\' && !IsCorrectSrcName(src_name + 1) || src_name[0] != L'\\' && !IsCorrectSrcName(src_name))
	{
		Err(L"EK0005", NULL, src_name);
		return DummyPtr;
	}
	*end_flag = False;

	{
		const Char* true_path;
		if (src_name[0] == L'\\')
			true_path = NewStr(NULL, L"%s%s.kn", Option->SrcDir, src_name + 1);
		else
		{
			true_path = NewStr(NULL, L"%s%s.knc", Option->SysDir, src_name);
			FilePtr = FuncWfopen(true_path, L"rb");
			if (FilePtr != NULL)
			{
				SrcName = src_name;
				Row = 1;
				Col = 1;
				return ParseSysCache();
			}
			true_path = NewStr(NULL, L"%s%s.kn", Option->SysDir, src_name);
		}
		for (; ; )
		{
			FilePtr = FuncWfopen(true_path, L"r, ccs=UTF-8");
			if (FilePtr == NULL)
			{
				Err(L"EK0006", NULL, true_path);
				return DummyPtr;
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
		SAstRoot* ast2;
		SrcName = src_name;
		Row = 1;
		Col = 0;
		FileBuf = L'\0';
		FileBufTmp = L'\0';
		IsLast = False;
		Scope = NULL;
		UniqueCnt = 0;
		ast2 = ParseRoot();
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
			case L'\u3000':
				Err(L"EP0008", NewPos(SrcName, Row, Col));
				continue;
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
				if (c == L'\u3000')
					Err(L"EP0008", NewPos(SrcName, Row, Col));
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
						if (c == L'\u3000')
						{
							Err(L"EP0008", NewPos(SrcName, Row, Col));
							continue;
						}
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
			case L'\u3000':
				Err(L"EP0008", NewPos(SrcName, Row, Col));
				continue;
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
		Err(L"EP0000", NewPos(SrcName, Row, Col), c);
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
									Err(L"EP0002", NewPos(SrcName, Row, Col), src_name);
									return L"";
								}
								ptr++;
							}
						}
						if (DictSearch(Srces2, src_name) == NULL)
							Srces2 = DictAdd(Srces2, src_name, NULL);
					}
					at = True;
					break;
				case L'\\':
					if (at)
					{
						Err(L"EP0055", NewPos(SrcName, Row, Col));
						return L"";
					}
					break;
			}
			if (pos == 128)
			{
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
		Err(L"EP0023", NewPos(SrcName, Row, Col), c);
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

static void ReadUntilRet(void)
{
	Char c;
	do
	{
		c = Read();
	} while (c != L'\n' && c != L'\0');
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
		SDict* children = ((SAst*)StackPeek(Scope))->ScopeChildren;
		if (DictSearch(children, ast->Name) != NULL)
		{
			Err(L"EP0005", NewPos(SrcName, Row, Col), ast->Name);
			return;
		}
		((SAst*)StackPeek(Scope))->ScopeChildren = DictAdd(children, ast->Name, ast);
	}
}

static void AssertNextChar(Char c, Bool skip_spaces)
{
	Char c2 = skip_spaces ? ReadChar() : Read();
	if (c2 != c)
	{
		NextCharErr(c, c2);
		FileBuf = c2;
	}
}

static void NextCharErr(Char c, Char c2)
{
	if (c == L'\0')
		c = L' ';
	if (c2 == L'\0')
		c2 = L' ';
	Err(L"EP0006", NewPos(SrcName, Row, Col), c, c2);
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
		case L'w': return L'\u3000';
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
			Err(L"EP0007", NewPos(SrcName, Row, Col), c);
			return c;
	}
}

static Bool IsCorrectSrcName(const Char* name)
{
	if (!(L'a' <= name[0] && name[0] <= L'z' || name[0] == L'_'))
		return False;
	while (*name != L'\0')
	{
		if (!(L'a' <= name[0] && name[0] <= L'z' || name[0] == L'_' || L'0' <= name[0] && name[0] <= L'9'))
			return False;
		name++;
	}
	return True;
}

static SAst* ParseSysCache(void)
{
	// TODO:
	return NULL;
}

static SAstRoot* ParseRoot(void)
{
	SAstRoot* ast = (SAstRoot*)Alloc(sizeof(SAstRoot));
	InitAst((SAst*)ast, AstTypeId_Root, NewPos(SrcName, 1, 1), NULL, False, True);
	ast->Items = ListNew();
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
					child = (SAst*)ParseFunc(NULL);
				else if (wcscmp(id, L"var") == 0)
					child = (SAst*)ParseVar(AstArgKind_Global, NULL);
				else if (wcscmp(id, L"const") == 0)
					child = (SAst*)ParseConst();
				else if (wcscmp(id, L"alias") == 0)
					child = (SAst*)ParseAlias();
				else if (wcscmp(id, L"class") == 0)
					child = (SAst*)ParseClass();
				else if (wcscmp(id, L"enum") == 0)
					child = (SAst*)ParseEnum();
				else
				{
					Err(L"EP0025", NewPos(SrcName, row, col), id);
					ReadUntilRet();
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
	Scope = StackPop(Scope);
	return ast;
}

static SAstFunc* ParseFunc(const Char* parent_class)
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
					if (wcscmp(func_attr, L"_any_type") == 0 && (ast->FuncAttr & FuncAttr_AnyType) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_AnyType);
					else if (wcscmp(func_attr, L"_init") == 0 && (ast->FuncAttr & FuncAttr_Init) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_Init);
					else if (wcscmp(func_attr, L"_take_me") == 0 && (ast->FuncAttr & FuncAttr_TakeMe) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeMe);
					else if (wcscmp(func_attr, L"_ret_me") == 0 && (ast->FuncAttr & FuncAttr_RetMe) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetMe);
					else if (wcscmp(func_attr, L"_take_child") == 0 && (ast->FuncAttr & FuncAttr_TakeChild) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeChild);
					else if (wcscmp(func_attr, L"_ret_child") == 0 && (ast->FuncAttr & FuncAttr_RetChild) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetChild);
					else if (wcscmp(func_attr, L"_take_key_value") == 0 && (ast->FuncAttr & FuncAttr_TakeKeyValue) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_TakeKeyValue);
					else if (wcscmp(func_attr, L"_ret_array_of_list_child") == 0 && (ast->FuncAttr & FuncAttr_RetArrayOfListChild) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_RetArrayOfListChild);
					else if (wcscmp(func_attr, L"_make_instance") == 0 && (ast->FuncAttr & FuncAttr_MakeInstance) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_MakeInstance);
					else if (wcscmp(func_attr, L"_force") == 0 && (ast->FuncAttr & FuncAttr_Force) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_Force);
					else if (wcscmp(func_attr, L"_exit_code") == 0 && (ast->FuncAttr & FuncAttr_ExitCode) == 0)
						ast->FuncAttr = (EFuncAttr)(ast->FuncAttr | FuncAttr_ExitCode);
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
					FileBuf = c;
					break;
				}
			}
		}
		else
			FileBuf = c;
	}
	InitAst((SAst*)ast, AstTypeId_Func, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, True);
	ast->Addr = NewAddr();
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
				if (LocalErr)
				{
					LocalErr = False;
					ReadUntilRet();
					FileBuf = L'\n';
					break;
				}
				c = ReadChar();
				if (c == L'\0')
					break;
				if (c == L')')
					break;
				if (c != L',')
				{
					NextCharErr(L',', c);
					FileBuf = c;
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
			if (LocalErr)
			{
				LocalErr = False;
				ReadUntilRet();
				c = L'\n';
			}
			else
				c = ReadChar();
		}
		if (c != L'\n')
		{
			NextCharErr(L'\n', c);
			FileBuf = c;
		}
	}
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == (SAstStat*)DummyPtr || ((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	Scope = StackPop(Scope);
	return ast;
}

static SAstVar* ParseVar(EAstArgKind kind, const Char* parent_class)
{
	SAstVar* ast = (SAstVar*)Alloc(sizeof(SAstVar));
	InitAst((SAst*)ast, AstTypeId_Var, NewPos(SrcName, Row, Col), NULL, False, False);
	ast->Var = ParseArg(kind, parent_class);
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
		return (SAstVar*)DummyPtr;
	}
	else
		AssertNextChar(L'\n', True);
	return ast;
}

static SAstConst* ParseConst(void)
{
	SAstConst* ast = (SAstConst*)Alloc(sizeof(SAstConst));
	InitAst((SAst*)ast, AstTypeId_Const, NewPos(SrcName, Row, Col), NULL, False, False);
	ast->Var = ParseArg(AstArgKind_Const, NULL);
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
		return (SAstConst*)DummyPtr;
	}
	else
		AssertNextChar(L'\n', True);
	return ast;
}

static SAstAlias* ParseAlias(void)
{
	SAstAlias* ast = (SAstAlias*)Alloc(sizeof(SAstAlias));
	InitAst((SAst*)ast, AstTypeId_Alias, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, False);
	AssertNextChar(L':', True);
	ast->Type = ParseType();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
		AssertNextChar(L'\n', True);
	return ast;
}

static SAstClass* ParseClass(void)
{
	SAstClass* ast = (SAstClass*)Alloc(sizeof(SAstClass));
	InitAst((SAst*)ast, AstTypeId_Class, NewPos(SrcName, Row, Col), ReadIdentifier(True, False), True, True);
	ast->Addr = NewAddr();
	ast->VarSize = 0;
	ast->FuncSize = 0;
	ast->Items = ListNew();
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
			return (SAstClass*)DummyPtr;
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
					item->Def = (SAst*)ParseFunc(class_name);
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
						ReadUntilRet();
						continue;
					}
				}
			}
			ListAdd(ast->Items, item);
		}
	}
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
			return (SAstEnum*)DummyPtr;
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
				if (LocalErr)
				{
					LocalErr = False;
					ReadUntilRet();
					c = L'\n';
					continue;
				}
				else
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
	if (LocalErr)
		return (SAstArg*)DummyPtr;
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
			{
				ast->Expr = ParseExpr();
				if (LocalErr)
					return (SAstArg*)DummyPtr;
			}
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
				return (SAstStat*)DummyPtr;
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
		else if (wcscmp(s, L"foreach") == 0)
			ast = ParseStatForEach(row, col);
		else if (wcscmp(s, L"try") == 0)
			ast = ParseStatTry(row, col);
		else if (wcscmp(s, L"catch") == 0)
			ast = ParseStatCatch(row, col, block);
		else if (wcscmp(s, L"finally") == 0)
			ast = ParseStatFinally(row, col, block);
		else if (wcscmp(s, L"throw") == 0)
			ast = ParseStatThrow();
		else if (wcscmp(s, L"ifdef") == 0)
			ast = ParseStatIfDef();
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
			ReadUntilRet();
			return (SAstStat*)DummyPtr;
		}
		if (ast == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		((SAst*)ast)->Pos = NewPos(SrcName, row, col);
	}
	return ast;
}

static SAstStat* ParseStatEnd(int row, int col, SAst* block)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatEnd, NULL, NULL, False, False);
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
		else if (wcscmp(s, L"foreach") == 0)
		{
			if (block->TypeId != AstTypeId_StatForEach)
				err = True;
		}
		else if (wcscmp(s, L"try") == 0)
		{
			if (block->TypeId != AstTypeId_StatTry)
				err = True;
		}
		else if (wcscmp(s, L"ifdef") == 0)
		{
			if (block->TypeId != AstTypeId_StatIfDef)
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
	ast->Def = ParseFunc(NULL);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatVar(void)
{
	SAstStatVar* ast = (SAstStatVar*)Alloc(sizeof(SAstStatVar));
	InitAst((SAst*)ast, AstTypeId_StatVar, NULL, NULL, False, False);
	ast->Def = ParseVar(AstArgKind_LocalVar, NULL);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatConst(void)
{
	SAstStatConst* ast = (SAstStatConst*)Alloc(sizeof(SAstStatConst));
	InitAst((SAst*)ast, AstTypeId_StatConst, NULL, NULL, False, False);
	ast->Def = ParseConst();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatAlias(void)
{
	SAstStatAlias* ast = (SAstStatAlias*)Alloc(sizeof(SAstStatAlias));
	InitAst((SAst*)ast, AstTypeId_StatAlias, NULL, NULL, False, False);
	ast->Def = ParseAlias();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatClass(void)
{
	SAstStatClass* ast = (SAstStatClass*)Alloc(sizeof(SAstStatClass));
	InitAst((SAst*)ast, AstTypeId_StatClass, NULL, NULL, False, False);
	ast->Def = ParseClass();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatEnum(void)
{
	SAstStatEnum* ast = (SAstStatEnum*)Alloc(sizeof(SAstStatEnum));
	InitAst((SAst*)ast, AstTypeId_StatEnum, NULL, NULL, False, False);
	ast->Def = ParseEnum();
	return (SAstStat*)ast;
}

static SAstStat* ParseStatIf(void)
{
	SAstStatIf* ast = (SAstStatIf*)Alloc(sizeof(SAstStatIf));
	InitAst((SAst*)ast, AstTypeId_StatIf, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->Stats = ListNew();
	ast->ElIfs = ListNew();
	ast->ElseStats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
	{
		AssertNextChar(L')', True);
		AssertNextChar(L'\n', True);
	}
	{
		SAstStat* stat;
		EAstTypeId type_id;
		for (; ; )
		{
			stat = ParseStat((SAst*)ast);
			if (stat == (SAstStat*)DummyPtr)
				return (SAstStat*)DummyPtr;
			type_id = ((SAst*)stat)->TypeId;
			if (type_id == AstTypeId_StatElIf || type_id == AstTypeId_StatElse || type_id == AstTypeId_StatEnd)
				break;
			ListAdd(ast->Stats, stat);
		}
		while (type_id == AstTypeId_StatElIf)
		{
			SAstStatElIf* elif = (SAstStatElIf*)stat;
			elif->Stats = ListNew();
			for (; ; )
			{
				stat = ParseStat((SAst*)ast);
				if (stat == (SAstStat*)DummyPtr)
					return (SAstStat*)DummyPtr;
				type_id = ((SAst*)stat)->TypeId;
				if (type_id == AstTypeId_StatElIf || type_id == AstTypeId_StatElse || type_id == AstTypeId_StatEnd)
					break;
				ListAdd(elif->Stats, stat);
			}
			ListAdd(ast->ElIfs, elif);
		}
		while (type_id == AstTypeId_StatElse)
		{
			for (; ; )
			{
				stat = ParseStat((SAst*)ast);
				if (stat == (SAstStat*)DummyPtr)
					return (SAstStat*)DummyPtr;
				type_id = ((SAst*)stat)->TypeId;
				if (type_id == AstTypeId_StatEnd)
					break;
				if (type_id == AstTypeId_StatElIf || type_id == AstTypeId_StatElse)
					Err(L"EP0041", NewPos(SrcName, Row, Col));
				ListAdd(ast->ElseStats, stat);
			}
		}
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatElIf(int row, int col, const SAst* block)
{
	SAstStatElIf* ast = (SAstStatElIf*)Alloc(sizeof(SAstStatElIf));
	InitAst((SAst*)ast, AstTypeId_StatElIf, NULL, NULL, False, False);
	ast->Stats = NULL;
	if (block->TypeId != AstTypeId_StatIf)
	{
		Err(L"EP0042", NewPos(SrcName, row, col));
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	AssertNextChar(L'(', True);
	ast->Cond = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
	{
		AssertNextChar(L')', True);
		AssertNextChar(L'\n', True);
	}
	return (SAstStat*)ast;
}

static SAstStat* ParseStatElse(int row, int col, const SAst* block)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatElse, NULL, NULL, False, False);
	if (block->TypeId != AstTypeId_StatIf)
	{
		Err(L"EP0043", NewPos(SrcName, row, col));
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatSwitch(int row, int col)
{
	SAstStatSwitch* ast = (SAstStatSwitch*)Alloc(sizeof(SAstStatSwitch));
	InitAst((SAst*)ast, AstTypeId_StatSwitch, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->Cases = ListNew();
	ast->DefaultStats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
	{
		AssertNextChar(L')', True);
		AssertNextChar(L'\n', True);
	}
	{
		SAstStat* stat;
		EAstTypeId type_id;
		stat = ParseStat((SAst*)ast);
		if (stat == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		type_id = ((SAst*)stat)->TypeId;
		if (!(type_id == AstTypeId_StatCase || type_id == AstTypeId_StatDefault || type_id == AstTypeId_StatEnd))
			Err(L"EP0044", NewPos(SrcName, Row, Col));
		while (type_id == AstTypeId_StatCase)
		{
			SAstStatCase* case_ = (SAstStatCase*)stat;
			case_->Stats = ListNew();
			for (; ; )
			{
				stat = ParseStat((SAst*)ast);
				if (stat == (SAstStat*)DummyPtr)
					return (SAstStat*)DummyPtr;
				type_id = ((SAst*)stat)->TypeId;
				if (type_id == AstTypeId_StatCase || type_id == AstTypeId_StatDefault || type_id == AstTypeId_StatEnd)
					break;
				ListAdd(case_->Stats, stat);
			}
			ListAdd(ast->Cases, case_);
		}
		while (type_id == AstTypeId_StatDefault)
		{
			for (; ; )
			{
				stat = ParseStat((SAst*)ast);
				if (stat == (SAstStat*)DummyPtr)
					return (SAstStat*)DummyPtr;
				type_id = ((SAst*)stat)->TypeId;
				if (type_id == AstTypeId_StatEnd)
					break;
				if (type_id == AstTypeId_StatCase || type_id == AstTypeId_StatDefault)
					Err(L"EP0045", NewPos(SrcName, Row, Col));
				ListAdd(ast->DefaultStats, stat);
			}
		}
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatCase(int row, int col, const SAst* block)
{
	SAstStatCase* ast = (SAstStatCase*)Alloc(sizeof(SAstStatCase));
	InitAst((SAst*)ast, AstTypeId_StatCase, NULL, NULL, False, False);
	ast->Conds = ListNew();
	ast->Stats = NULL;
	if (block->TypeId != AstTypeId_StatSwitch)
	{
		Err(L"EP0046", NewPos(SrcName, row, col));
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	for (; ; )
	{
		Char c;
		SAstExpr** exprs = (SAstExpr**)Alloc(sizeof(SAstExpr*) * 2);
		exprs[0] = ParseExpr();
		if (LocalErr)
		{
			LocalErr = False;
			ReadUntilRet();
			return (SAstStat*)DummyPtr;
		}
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
				ReadUntilRet();
				return (SAstStat*)DummyPtr;
			}
		}
		exprs[1] = ParseExpr();
		if (LocalErr)
		{
			LocalErr = False;
			ReadUntilRet();
			return (SAstStat*)DummyPtr;
		}
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
	if (block->TypeId != AstTypeId_StatSwitch)
	{
		Err(L"EP0048", NewPos(SrcName, row, col));
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatWhile(void)
{
	SAstStatWhile* ast = (SAstStatWhile*)Alloc(sizeof(SAstStatWhile));
	InitAst((SAst*)ast, AstTypeId_StatWhile, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	((SAstStatSkipable*)ast)->SkipPoint = AsmLabel();
	ast->Skip = False;
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
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
		if (stat == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatFor(int row, int col)
{
	SAstStatFor* ast = (SAstStatFor*)Alloc(sizeof(SAstStatFor));
	InitAst((SAst*)ast, AstTypeId_StatFor, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	((SAstStatSkipable*)ast)->SkipPoint = AsmLabel();
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Start = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
	{
		AssertNextChar(L',', True);
		ast->Cond = ParseExpr();
		if (LocalErr)
		{
			LocalErr = False;
			ReadUntilRet();
		}
		else
		{
			Char c = ReadChar();
			if (c == L',')
			{
				ast->Step = ParseExpr();
				if (LocalErr)
				{
					LocalErr = False;
					ReadUntilRet();
				}
				else
				{
					AssertNextChar(L')', True);
					AssertNextChar(L'\n', True);
				}
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
		if (stat == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatForEach(int row, int col)
{
	SAstStatForEach* ast = (SAstStatForEach*)Alloc(sizeof(SAstStatForEach));
	InitAst((SAst*)ast, AstTypeId_StatForEach, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	((SAstStatSkipable*)ast)->SkipPoint = AsmLabel();
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	ast->Cond = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
	}
	else
	{
		AssertNextChar(L')', True);
		AssertNextChar(L'\n', True);
	}
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatTry(int row, int col)
{
	SAstStatTry* ast = (SAstStatTry*)Alloc(sizeof(SAstStatTry));
	InitAst((SAst*)ast, AstTypeId_StatTry, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = MakeBlockVar(row, col);
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->Stats = ListNew();
	ast->Catches = ListNew();
	ast->FinallyStats = NULL;
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
		SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
		InitAst((SAst*)type, AstTypeId_TypeUser, NewPos(SrcName, row, col), NULL, False, False);
		((SAst*)type)->RefName = L"kuin@Excpt";
		AddScopeRefeds((SAst*)type);
		((SAstStatBreakable*)ast)->BlockVar->Type = (SAstType*)type;
	}
	{
		SAstStat* stat;
		EAstTypeId type_id;
		for (; ; )
		{
			stat = ParseStat((SAst*)ast);
			if (stat == (SAstStat*)DummyPtr)
				return (SAstStat*)DummyPtr;
			type_id = ((SAst*)stat)->TypeId;
			if (type_id == AstTypeId_StatCatch || type_id == AstTypeId_StatFinally)
				break;
			ListAdd(ast->Stats, stat);
		}
		while (type_id == AstTypeId_StatCatch)
		{
			SAstStatCatch* catch_ = (SAstStatCatch*)stat;
			catch_->Stats = ListNew();
			for (; ; )
			{
				stat = ParseStat((SAst*)ast);
				if (stat == (SAstStat*)DummyPtr)
					return (SAstStat*)DummyPtr;
				type_id = ((SAst*)stat)->TypeId;
				if (type_id == AstTypeId_StatCatch || type_id == AstTypeId_StatFinally || type_id == AstTypeId_StatEnd)
					break;
				ListAdd(catch_->Stats, stat);
			}
			ListAdd(ast->Catches, catch_);
		}
		if (type_id == AstTypeId_StatFinally)
		{
			ast->FinallyStats = ListNew();
			for (; ; )
			{
				stat = ParseStat((SAst*)ast);
				if (stat == (SAstStat*)DummyPtr)
					return (SAstStat*)DummyPtr;
				type_id = ((SAst*)stat)->TypeId;
				if (type_id == AstTypeId_StatEnd)
					break;
				if (type_id == AstTypeId_StatCatch || type_id == AstTypeId_StatFinally)
					Err(L"EP0050", NewPos(SrcName, Row, Col));
				ListAdd(ast->FinallyStats, stat);
			}
		}
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatCatch(int row, int col, const SAst* block)
{
	SAstStatCatch* ast = (SAstStatCatch*)Alloc(sizeof(SAstStatCatch));
	InitAst((SAst*)ast, AstTypeId_StatCatch, NULL, NULL, False, False);
	ast->Conds = ListNew();
	ast->Stats = NULL;
	if (block->TypeId != AstTypeId_StatTry)
	{
		Err(L"EP0051", NewPos(SrcName, row, col));
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
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
				if (LocalErr)
				{
					LocalErr = False;
					ReadUntilRet();
					return (SAstStat*)DummyPtr;
				}
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
				if (LocalErr)
				{
					LocalErr = False;
					ReadUntilRet();
					return (SAstStat*)DummyPtr;
				}
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
	if (block->TypeId != AstTypeId_StatTry)
	{
		Err(L"EP0052", NewPos(SrcName, row, col));
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatThrow(void)
{
	SAstStatThrow* ast = (SAstStatThrow*)Alloc(sizeof(SAstStatThrow));
	InitAst((SAst*)ast, AstTypeId_StatThrow, NULL, NULL, False, False);
	ast->Msg = NULL;
	ast->Code = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	{
		Char c = ReadChar();
		if (c == L',')
		{
			ast->Msg = ParseExpr();
			if (LocalErr)
			{
				LocalErr = False;
				ReadUntilRet();
				return (SAstStat*)DummyPtr;
			}
			c = ReadChar();
		}
		if (c != L'\n')
			NextCharErr(L'\n', c);
	}
	return (SAstStat*)ast;
}

static SAstStat* ParseStatIfDef(void)
{
	SAstStatIfDef* ast = (SAstStatIfDef*)Alloc(sizeof(SAstStatIfDef));
	InitAst((SAst*)ast, AstTypeId_StatIfDef, NULL, NULL, False, True);
	((SAstStatBreakable*)ast)->BlockVar = NULL;
	((SAstStatBreakable*)ast)->BreakPoint = AsmLabel();
	ast->Stats = ListNew();
	PushDummyScope((SAst*)ast);
	ObtainBlockName((SAst*)ast);
	{
		int row = Row;
		int col = Col;
		const Char* s = ReadIdentifier(True, False);
		if (wcscmp(s, L"dbg") == 0)
			ast->Dbg = True;
		else if (wcscmp(s, L"rls") == 0)
			ast->Dbg = False;
		else
			Err(L"EP0053", NewPos(SrcName, row, col), s);
	}
	AssertNextChar(L')', True);
	AssertNextChar(L'\n', True);
	for (; ; )
	{
		SAstStat* stat = ParseStat((SAst*)ast);
		if (stat == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatBlock(void)
{
	SAstStatBlock* ast = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
	InitAst((SAst*)ast, AstTypeId_StatBlock, NULL, NULL, False, True);
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
		if (stat == (SAstStat*)DummyPtr)
			return (SAstStat*)DummyPtr;
		if (((SAst*)stat)->TypeId == AstTypeId_StatEnd)
			break;
		ListAdd(ast->Stats, stat);
	}
	Scope = StackPop(Scope);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatRet(void)
{
	SAstStatRet* ast = (SAstStatRet*)Alloc(sizeof(SAstStatRet));
	InitAst((SAst*)ast, AstTypeId_StatRet, NULL, NULL, False, False);
	{
		Char c = ReadChar();
		if (c != L'\n')
		{
			FileBuf = c;
			ast->Value = ParseExpr();
			if (LocalErr)
			{
				LocalErr = False;
				ReadUntilRet();
				return (SAstStat*)DummyPtr;
			}
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
	InitAst((SAst*)ast, AstTypeId_StatDo, NULL, NULL, False, False);
	ast->Expr = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	AssertNextChar(L'\n', True);
	return (SAstStat*)ast;
}

static SAstStat* ParseStatBreak(void)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatBreak, NULL, NULL, False, False);
	((SAst*)ast)->RefName = ReadIdentifier(True, False);
	AddScopeRefeds((SAst*)ast);
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatSkip(void)
{
	SAstStat* ast = (SAstStat*)Alloc(sizeof(SAstStat));
	InitAst((SAst*)ast, AstTypeId_StatSkip, NULL, NULL, False, False);
	((SAst*)ast)->RefName = ReadIdentifier(True, False);
	AddScopeRefeds((SAst*)ast);
	AssertNextChar(L'\n', True);
	return ast;
}

static SAstStat* ParseStatAssert(void)
{
	int row = Row;
	int col = Col;
	SAstStatAssert* ast = (SAstStatAssert*)Alloc(sizeof(SAstStatAssert));
	InitAst((SAst*)ast, AstTypeId_StatAssert, NULL, NULL, False, False);
	ast->Cond = ParseExpr();
	if (LocalErr)
	{
		LocalErr = False;
		ReadUntilRet();
		return (SAstStat*)DummyPtr;
	}
	ast->Msg = (SAstExpr*)ObtainStrValue(NewPos(SrcName, row, col), NewStr(NULL, L"(%s: %d, %d)", SrcName, row, col));
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
			if (LocalErr)
				return (SAstType*)DummyPtr;
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
							if (LocalErr)
								return (SAstType*)DummyPtr;
							ListAdd(ast2->Args, arg);
							c = ReadChar();
							if (c == L')')
								break;
							if (c != L',')
								NextCharErr(L',', c);
						}
					}
					c = ReadChar();
					if (c == L':')
					{
						ast2->Ret = ParseType();
						if (LocalErr)
							return (SAstType*)DummyPtr;
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
					if (LocalErr)
						return (SAstType*)DummyPtr;
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
					if (LocalErr)
						return (SAstType*)DummyPtr;
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
					if (LocalErr)
						return (SAstType*)DummyPtr;
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
					if (LocalErr)
						return (SAstType*)DummyPtr;
					AssertNextChar(L',', True);
					ast2->ItemTypeValue = ParseType();
					if (LocalErr)
						return (SAstType*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
						Err(L"EP0054", NewPos(SrcName, row, col), c2);
						LocalErr = True;
						ReadUntilRet();
						return (SAstExpr*)DummyPtr;
				}
			}
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExpr();
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
				if (LocalErr)
					return (SAstExpr*)DummyPtr;
				AssertNextChar(L',', True);
				ast2->Children[2] = ParseExpr();
				if (LocalErr)
					return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
							if (LocalErr)
								return (SAstExpr*)DummyPtr;
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
								if (LocalErr)
									return (SAstExpr*)DummyPtr;
								ast = (SAstExpr*)ast2;
							}
							else if (c == L'$')
							{
								SAstExprAs* ast2 = (SAstExprAs*)Alloc(sizeof(SAstExprAs));
								InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprAs, NewPos(SrcName, row, col));
								ast2->Kind = AstExprAsKind_NIs;
								ast2->Child = ast;
								ast2->ChildType = ParseType();
								if (LocalErr)
									return (SAstExpr*)DummyPtr;
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
									if (LocalErr)
										return (SAstExpr*)DummyPtr;
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
								if (LocalErr)
									return (SAstExpr*)DummyPtr;
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
						if (LocalErr)
							return (SAstExpr*)DummyPtr;
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
							if (LocalErr)
								return (SAstExpr*)DummyPtr;
							ast = (SAstExpr*)ast2;
						}
						else if (c == L'$')
						{
							SAstExprAs* ast2 = (SAstExprAs*)Alloc(sizeof(SAstExprAs));
							InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprAs, NewPos(SrcName, row, col));
							ast2->Kind = AstExprAsKind_Is;
							ast2->Child = ast;
							ast2->ChildType = ParseType();
							if (LocalErr)
								return (SAstExpr*)DummyPtr;
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
								if (LocalErr)
									return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
			ast = (SAstExpr*)ast2;
		}
		else if (c == L'-')
		{
			SAstExpr2* ast2 = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr2, NewPos(SrcName, row, col));
			ast2->Kind = AstExpr2Kind_Sub;
			ast2->Children[0] = ast;
			ast2->Children[1] = ParseExprMul();
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
						if (LocalErr)
							return (SAstExpr*)DummyPtr;
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
						if (LocalErr)
							return (SAstExpr*)DummyPtr;
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
						if (LocalErr)
							return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
					if (LocalErr)
						return (SAstExpr*)DummyPtr;
					c = ReadChar();
					if (c == L'\0')
						break;
					if (c == L']')
						break;
					if (c != L',')
					{
						NextCharErr(L',', c);
						LocalErr = True;
						return (SAstExpr*)DummyPtr;
					}
				}
				ast2->ItemType = ParseType();
				if (LocalErr)
					return (SAstExpr*)DummyPtr;
				ast = (SAstExpr*)ast2;
			}
			else if (c == L'#')
			{
				SAstExpr1* ast2 = (SAstExpr1*)Alloc(sizeof(SAstExpr1));
				InitAstExpr((SAstExpr*)ast2, AstTypeId_Expr1, NewPos(SrcName, row, col));
				ast2->Kind = AstExpr1Kind_Copy;
				ast2->Child = ParseExprPlus();
				if (LocalErr)
					return (SAstExpr*)DummyPtr;
				ast = (SAstExpr*)ast2;
			}
			else
			{
				FileBuf = c;
				{
					SAstExprNew* ast2 = (SAstExprNew*)Alloc(sizeof(SAstExprNew));
					InitAstExpr((SAstExpr*)ast2, AstTypeId_ExprNew, NewPos(SrcName, row, col));
					ast2->ItemType = ParseType();
					if (LocalErr)
						return (SAstExpr*)DummyPtr;
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
					Err(L"EP0054", NewPos(SrcName, row, col), c);
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
			}
			ast2->Child = ParseExprPlus();
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
			if (LocalErr)
				return (SAstExpr*)DummyPtr;
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
	if (LocalErr)
		return (SAstExpr*)DummyPtr;
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
								c = ReadChar();
								if (c == L'&')
									arg->RefVar = True;
								else
								{
									arg->RefVar = False;
									FileBuf = c;
								}
								arg->Arg = ParseExpr();
								if (LocalErr)
									return (SAstExpr*)DummyPtr;
								ListAdd(ast2->Args, arg);
								c = ReadChar();
								if (c == L'\0')
									break;
								if (c == L')')
									break;
								if (c != L',')
								{
									NextCharErr(L',', c);
									LocalErr = True;
									return (SAstExpr*)DummyPtr;
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
						if (LocalErr)
							return (SAstExpr*)DummyPtr;
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
							if (LocalErr)
								return (SAstExpr*)DummyPtr;
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
							if (LocalErr)
								return (SAstExpr*)DummyPtr;
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
								if (LocalErr)
									return (SAstExpr*)DummyPtr;
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
							LocalErr = True;
							ReadUntilRet();
							return (SAstExpr*)DummyPtr;
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
						LocalErr = True;
						ReadUntilRet();
						return (SAstExpr*)DummyPtr;
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
							Err(L"EP0013", NewPos(SrcName, row, col), buf);
							LocalErr = True;
							ReadUntilRet();
							return (SAstExpr*)DummyPtr;
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
						Err(L"EP0013", NewPos(SrcName, row, col), buf);
						LocalErr = True;
						ReadUntilRet();
						return (SAstExpr*)DummyPtr;
					}
					buf = c;
					set = True;
				}
				if (!set)
				{
					Err(L"EP0014", NewPos(SrcName, row, col));
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
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
				if (LocalErr)
					return (SAstExpr*)DummyPtr;
				if (ReadChar() != L')')
				{
					Err(L"EP0015", NewPos(SrcName, Row, Col));
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
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
						if (LocalErr)
							return (SAstExpr*)DummyPtr;
						ListAdd(ast->Values, expr);
						c = ReadChar();
						if (c == L'\0')
							break;
						if (c == L']')
							break;
						if (c != L',')
						{
							NextCharErr(L',', c);
							LocalErr = True;
							return (SAstExpr*)DummyPtr;
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
			{
				SAstExpr* ast = ParseExprNumber(row, col, c);
				if (LocalErr)
					return (SAstExpr*)DummyPtr;
				return ast;
			}
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
		Bool chg_base = False;
		Bool float_type = False;
		int bit_size = 0; // The size for bit types.
		for (; ; )
		{
			if (c == L'#')
			{
				if (chg_base || float_type)
				{
					Err(L"EP0016", NewPos(SrcName, row, col));
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				{
					Char* end_ptr;
					errno = 0;
					buf[len] = L'\0';
					base = wcstol(buf, &end_ptr, 10);
					if (*end_ptr != L'\0' || errno == ERANGE || !(base == 2 || base == 8 || base == 16))
					{
						Err(L"EP0019", NewPos(SrcName, row, col), base);
						LocalErr = True;
						ReadUntilRet();
						return (SAstExpr*)DummyPtr;
					}
				}
				len = 0;
				chg_base = True;
			}
			else if (c == L'.')
			{
				if (chg_base || float_type)
				{
					Err(L"EP0017", NewPos(SrcName, row, col));
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				if (len == 1024)
				{
					buf[1024] = L'\0';
					Err(L"EP0018", NewPos(SrcName, row, col), buf);
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
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
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
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
			LocalErr = True;
			ReadUntilRet();
			return (SAstExpr*)DummyPtr;
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
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				buf[len] = c;
				len++;
				c = Read();
				if (c != L'+' && c != L'-')
				{
					Err(L"EP0056", NewPos(SrcName, row, col));
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				if (len == 1024)
				{
					buf[1024] = L'\0';
					Err(L"EP0018", NewPos(SrcName, row, col), buf);
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				buf[len] = c;
				len++;
				c = Read();
				if (!(L'0' <= c && c <= L'9'))
				{
					Err(L"EP0056", NewPos(SrcName, row, col));
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				do
				{
					if (len == 1024)
					{
						buf[1024] = L'\0';
						Err(L"EP0018", NewPos(SrcName, row, col), buf);
						LocalErr = True;
						ReadUntilRet();
						return (SAstExpr*)DummyPtr;
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
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
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
						LocalErr = True;
						ReadUntilRet();
						return (SAstExpr*)DummyPtr;
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
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
				}
				if (bit_size == 1 && value > _UI8_MAX || bit_size == 2 && value > _UI16_MAX || bit_size == 4 && value > _UI32_MAX || bit_size == 0 && value > _I64_MAX)
				{
					Err(L"EP0021", NewPos(SrcName, row, col), buf);
					LocalErr = True;
					ReadUntilRet();
					return (SAstExpr*)DummyPtr;
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
