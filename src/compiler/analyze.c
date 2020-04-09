#include "analyze.h"

#include "list.h"
#include "log.h"
#include "mem.h"
#include "util.h"

static const Char* BuildInFuncs[] =
{
	L"abs\0          \x0b",
	L"add\0          \x07",
	L"and\0          \x02",
	L"clamp\0        \x0b",
	L"clampMax\0     \x0b",
	L"clampMin\0     \x0b",
	L"del\0          \x0f",
	L"delNext\0      \x09",
	L"endian\0       \x04",
	L"exist\0        \x0d",
	L"fill\0         \x05",
	L"find\0         \x0e",
	L"findBin\0      \x05",
	L"findLast\0     \x0e",
	L"findStr\0      \x06",
	L"findStrEx\0    \x06",
	L"findStrLast\0  \x06",
	L"forEach\0      \x0d",
	L"get\0          \x08",
	L"getOffset\0    \x09",
	L"getPtr\0       \x09",
	L"head\0         \x09",
	L"idx\0          \x09",
	L"ins\0          \x09",
	L"join\0         \x0c",
	L"lower\0        \x06",
	L"max\0          \x05",
	L"min\0          \x05",
	L"moveOffset\0   \x09",
	L"next\0         \x09",
	L"not\0          \x02",
	L"offset\0       \x03",
	L"or\0           \x02",
	L"peek\0         \x0a",
	L"prev\0         \x09",
	L"repeat\0       \x05",
	L"replace\0      \x06",
	L"reverse\0      \x05",
	L"sar\0          \x04",
	L"setPtr\0       \x09",
	L"shl\0          \x04",
	L"shr\0          \x04",
	L"shuffle\0      \x05",
	L"sign\0         \x0b",
	L"sort\0         \x0e",
	L"sortDesc\0     \x0e",
	L"split\0        \x06",
	L"sub\0          \x05",
	L"tail\0         \x09",
	L"term\0         \x09",
	L"termOffset\0   \x09",
	L"toArray\0      \x09",
	L"toArrayKey\0   \x0d",
	L"toArrayValue\0 \x0d",
	L"toBit64\0      \x06",
	L"toFloat\0      \x06",
	L"toInt\0        \x06",
	L"toStr\0        \x01",
	L"toStrFmt\0     \x0b",
	L"trim\0         \x06",
	L"trimLeft\0     \x06",
	L"trimRight\0    \x06",
	L"upper\0        \x06",
	L"xor\0          \x02",
};

static SDict* Asts;
static const SOption* Option;
static SDict* Dlls;

static SAstFunc* SearchMain(void);
static const void* ResolveIdentifierCallback(const Char* key, const void* value, void* param);
static void ResolveIdentifierRecursion(const Char* src, const SAst* scope);
static void InitAst(SAst* ast, EAstTypeId type_id, const SPos* pos);
static SList* RefreshStats(SList* stats, SAstType* ret_type, SAstFunc* parent_func);
static Bool CmpType(const SAstType* type1, const SAstType* type2);
static Bool IsComparable(const SAstType* type, Bool less_or_greater);
static U64 BitCast(int size, U64 n);
static SAstFunc* AddSpecialFunc(SAstClass* class_, const Char* name);
static SAst* SearchStdItem(const Char* src, const Char* identifier, Bool make_expr_ref);
static SAstExprDot* MakeMeDot(SAstClass* class_, SAstArg* arg, const Char* name);
static SAstExprValue* MakeExprNull(const SPos* pos);
static SAstExpr* CacheSubExpr(SList* stats, SAstExpr* ast, const SPos* pos);
static void AddDllFunc(const Char* dll_name, const Char* func_name);
static int GetBuildInFuncType(const Char* name);
static S64 GetEnumElementValue(SAstExprValue* ast, SAstEnum* enum_);
static const Char* GetTypeNameNew(const SAstType* type);
static const void* AddInitFuncs(const Char* key, const void* value, void* param);
static const void* AddFinFuncs(const Char* key, const void* value, void* param);
static SAstFunc* Rebuild(const SAstFunc* main_func);
static const void* RebuildEnumCallback(U64 key, const void* value, void* param);
static const void* RebuildRootCallback(const Char* key, const void* value, void* param);
static void RebuildRoot(SAstRoot* ast);
static void RebuildFunc(SAstFunc* ast);
static void RebuildVar(SAstVar* ast);
static void RebuildConst(SAstConst* ast);
static void RebuildAlias(SAstAlias* ast, SAstAlias* parent);
static void RebuildClass(SAstClass* ast);
static void RebuildEnum(SAstEnum* ast);
static void RebuildEnumElement(SAstExpr* enum_element, SAstType* type);
static void RebuildArg(SAstArg* ast);
static SAstStat* RebuildStat(SAstStat* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildIf(SAstStatIf* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildSwitch(SAstStatSwitch* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildWhile(SAstStatWhile* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildFor(SAstStatFor* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildTry(SAstStatTry* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildThrow(SAstStatThrow* ast);
static SAstStat* RebuildBlock(SAstStatBlock* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildRet(SAstStatRet* ast, SAstType* ret_type);
static SAstStat* RebuildDo(SAstStatDo* ast);
static SAstStat* RebuildBreak(SAstStat* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildSkip(SAstStat* ast, SAstType* ret_type, SAstFunc* parent_func);
static SAstStat* RebuildAssert(SAstStatAssert* ast);
static SAstType* RebuildType(SAstType* ast, SAstAlias* parent_alias);
static SAstExpr* RebuildExpr(SAstExpr* ast, Bool nullable);
static SAstExpr* RebuildExpr1(SAstExpr1* ast);
static SAstExpr* RebuildExpr2(SAstExpr2* ast);
static SAstExpr* RebuildExpr3(SAstExpr3* ast);
static SAstExpr* RebuildExprNew(SAstExprNew* ast);
static SAstExpr* RebuildExprNewArray(SAstExprNewArray* ast);
static SAstExpr* RebuildExprAs(SAstExprAs* ast);
static SAstExpr* RebuildExprToBin(SAstExprToBin* ast);
static SAstExpr* RebuildExprFromBin(SAstExprFromBin* ast);
static SAstExpr* RebuildExprCall(SAstExprCall* ast);
static SAstExpr* RebuildExprArray(SAstExprArray* ast);
static SAstExpr* RebuildExprDot(SAstExprDot* ast);
static SAstExpr* RebuildExprValue(SAstExprValue* ast);
static SAstExpr* RebuildExprValueArray(SAstExprValueArray* ast);
static SAstExpr* RebuildExprRef(SAstExpr* ast);

Bool AddAsm(S64* a, S64 b);
Bool SubAsm(S64* a, S64 b);
Bool MulAsm(S64* a, S64 b);

SAstFunc* Analyze(SDict* asts, const SOption* option, SDict** dlls)
{
	SAstFunc* result = NULL;

#if defined(_DEBUG)
	{
		int len = (int)(sizeof(BuildInFuncs) / sizeof(Char*));
		int i;
		for (i = 0; i < len - 1; i++)
			ASSERT(wcscmp(BuildInFuncs[i], BuildInFuncs[i + 1]) < 0);
	}
#endif

	Asts = asts;
	Option = option;
	Dlls = NULL;
	{
		SAstFunc* main_func = SearchMain();
		if (main_func == NULL)
		{
			Err(L"EA0058", NULL);
			return NULL;
		}
		DictForEach(Asts, ResolveIdentifierCallback, NULL);
		result = Rebuild(main_func);
#if defined(_DEBUG)
		Dump1(NewStr(NULL, L"%s_analyzed.xml", Option->OutputFile), (SAst*)main_func);
#endif
	}
	{
		// DLL functions loaded when the program starts.
		const Char* dll_name = L"d0000.knd";
		AddDllFunc(dll_name, L"_freeSet");
		AddDllFunc(dll_name, L"_toBin");
		AddDllFunc(dll_name, L"_fromBin");
		AddDllFunc(dll_name, L"_copy");
		AddDllFunc(dll_name, L"_powInt");
		AddDllFunc(dll_name, L"_powFloat");
		AddDllFunc(dll_name, L"_mod");
		AddDllFunc(dll_name, L"_cmpStr");
		AddDllFunc(dll_name, L"_newArray");
	}
	*dlls = Dlls;
	return result;
}

int GetBuildInFuncsNum(void)
{
	return (int)(sizeof(BuildInFuncs) / sizeof(Char*));
}

const Char** GetBuildInFuncs(void)
{
	return BuildInFuncs;
}

static SAstFunc* SearchMain(void)
{
	const SAst* ast = (const SAst*)DictSearch(Asts, NewStr(NULL, L"\\%s", Option->SrcName));
	SAst* func;
	if (ast == NULL)
		return NULL;
	func = (SAst*)DictSearch(ast->ScopeChildren, L"main");
	if (func != NULL && (func->TypeId & AstTypeId_Func) == AstTypeId_Func)
	{
		SAstFunc* func2 = (SAstFunc*)func;
		if (func2->Args->Len != 0 || func2->Ret != NULL || func2->FuncAttr != FuncAttr_None || func2->DllName != NULL)
			Err(L"EA0001", ((SAst*)func2)->Pos);
		return func2;
	}
	return NULL;
}

static const void* ResolveIdentifierCallback(const Char* key, const void* value, void* param)
{
	if (value == DummyPtr)
		return value; // Sub-source.
	if (param != NULL)
	{
		// 'key' is the source name and 'param' is 'NULL' when the compiler searches the roots of sources.
		// 'key' is the identifier of the scope's parent and 'param' is the source name when the compiler searches each scope.
		key = (Char*)param;
	}
	ResolveIdentifierRecursion(key, (const SAst*)value);
	return value;
}

static void ResolveIdentifierRecursion(const Char* src, const SAst* scope)
{
	if (scope->ScopeRefedItems == NULL)
	{
		ASSERT(scope->ScopeChildren == NULL);
		return;
	}
	{
		// Search scopes for identifiers and resolve references.
		SListNode* ptr = scope->ScopeRefedItems->Top;
		while (ptr != NULL)
		{
			SAst* ast = (SAst*)ptr->Data;
			ASSERT(ast->RefItem == NULL); // Must not resolve references that have been already resolved.
			ASSERT(ast->RefName != NULL);
			{
				Bool other_file = False;
				const Char* ptr_at = wcschr(ast->RefName, L'@');
				const Char* ptr_name; // The identifier right after '@'.
				const Char* s = ptr_at == NULL ? ast->RefName : ptr_at + 1;
				ptr_name = s;
				{
					const SAst* found_ast = NULL;
					if (ptr_at != NULL)
					{
						// Search the root of the source.
						Char src2[129];
						const Char* ptr_src; // The source name before '@'.
						if (ptr_at == ast->RefName)
							ptr_src = src;
						else
						{
							int len = (int)(ptr_at - ast->RefName);
							memcpy(src2, ast->RefName, sizeof(Char) * (size_t)len);
							src2[len] = L'\0';
							ptr_src = src2;
							if (wcscmp(ptr_src, src) == 0)
								Err(L"EA0002", ast->Pos, ptr_src);
							other_file = True;
						}
						{
							const SAst* ast2 = (const SAst*)DictSearch(Asts, ptr_src);
							if (ast2 != NULL)
								found_ast = (const SAst*)DictSearch(ast2->ScopeChildren, ptr_name);
						}
					}
					else
					{
						// Search from the current scope toward its parent's scope.
						const SAst* ast2 = scope;
						Bool over_func = False;
						for (; ; )
						{
							if (ast2->ScopeParent == NULL)
								break; // No more parent scope exists.
							if (ast2->Name != NULL && wcscmp(ast2->Name, ptr_name) == 0)
							{
								if ((ast2->TypeId & AstTypeId_Func) == AstTypeId_Func && ((SAst*)ast2)->RefName != NULL)
									Err(L"EA0057", ast->Pos, ptr_name);
								else
								{
									// The compiler also looks at the current scope's name.
									found_ast = ast2;
									break;
								}
							}
							{
								const SAst* ast3 = (const SAst*)DictSearch(ast2->ScopeChildren, ptr_name);
								if (ast3 != NULL)
								{
									if (over_func && (ast3->TypeId == AstTypeId_Arg && (((SAstArg*)ast3)->Kind == AstArgKind_Member || ((SAstArg*)ast3)->Kind == AstArgKind_LocalVar || ((SAstArg*)ast3)->Kind == AstArgKind_LocalArg) || (ast3->TypeId & AstTypeId_StatBreakable) != 0) || (ast3->TypeId & AstTypeId_Func) == AstTypeId_Func && ((SAst*)ast3)->RefName != NULL)
										Err(L"EA0057", ast->Pos, ptr_name);
									else
									{
										found_ast = ast3;
										break;
									}
								}
							}
							if ((ast2->TypeId & AstTypeId_Func) == AstTypeId_Func)
								over_func = True;
							ast2 = ast2->ScopeParent;
						}
					}
					if (found_ast != NULL)
					{
						if (other_file && !found_ast->Public)
							Err(L"EA0003", ast->Pos, ast->RefName);
						ast->RefItem = (SAst*)found_ast;
					}
					else
					{
						if (ast->RefName[0] != L'\0')
						{
							Err(L"EA0000", ast->Pos, ast->RefName);
							ast->TypeId = AstTypeId_Ast;
						}
						ast->RefItem = NULL;
					}
				}
			}
			ptr = ptr->Next;
		}
	}
	DictForEach(scope->ScopeChildren, ResolveIdentifierCallback, (void*)src);
}

static void InitAst(SAst* ast, EAstTypeId type_id, const SPos* pos)
{
	ast->TypeId = type_id;
	ast->Pos = pos;
	ast->Name = NULL;
	ast->ScopeParent = NULL;
	ast->ScopeChildren = NULL;
	ast->ScopeRefedItems = NULL;
	ast->RefName = NULL;
	ast->RefItem = NULL;
	ast->AnalyzedCache = NULL;
	ast->Public = False;
}

static void InitAstExpr(SAstExpr* ast, EAstTypeId type_id, const SPos* pos)
{
	InitAst((SAst*)ast, type_id, pos);
	ast->Type = NULL;
	ast->VarKind = AstExprVarKind_Unknown;
}

static SList* RefreshStats(SList* stats, SAstType* ret_type, SAstFunc* parent_func)
{
	SList* stats2 = ListNew();
	{
		SListNode* ptr = stats->Top;
		while (ptr != NULL)
		{
			SAstStat* stat = RebuildStat((SAstStat*)ptr->Data, ret_type, parent_func);
			if (stat != NULL)
				ListAdd(stats2, stat);
			ptr = ptr->Next;
		}
	}
	return stats2;
}

static Bool CmpType(const SAstType* type1, const SAstType* type2)
{
	EAstTypeId type_id1;
	EAstTypeId type_id2;
	if (type1 == NULL || type2 == NULL)
		return False;
	type_id1 = ((SAst*)type1)->TypeId;
	type_id2 = ((SAst*)type2)->TypeId;
	{
		// Comparing 'null' and 'nullable' should be true.
		Bool nullable1 = type_id1 == AstTypeId_TypeUser && ((SAst*)type1)->RefItem->TypeId == AstTypeId_Enum ? False : ((type_id1 & AstTypeId_TypeNullable) == AstTypeId_TypeNullable);
		Bool nullable2 = type_id2 == AstTypeId_TypeUser && ((SAst*)type2)->RefItem->TypeId == AstTypeId_Enum ? False : ((type_id2 & AstTypeId_TypeNullable) == AstTypeId_TypeNullable);
		if (nullable1 && type_id2 == AstTypeId_TypeNull || type_id1 == AstTypeId_TypeNull && nullable2 || type_id1 == AstTypeId_TypeNull && type_id2 == AstTypeId_TypeNull)
			return True;
	}
	if (type_id1 == AstTypeId_TypeArray && type_id2 == AstTypeId_TypeArray)
		return CmpType(((SAstTypeArray*)type1)->ItemType, ((SAstTypeArray*)type2)->ItemType);
	if (type_id1 == AstTypeId_TypeBit && type_id2 == AstTypeId_TypeBit)
		return ((SAstTypeBit*)type1)->Size == ((SAstTypeBit*)type2)->Size;
	if (type_id1 == AstTypeId_TypeFunc && type_id2 == AstTypeId_TypeFunc)
	{
		SAstTypeFunc* func1 = (SAstTypeFunc*)type1;
		SAstTypeFunc* func2 = (SAstTypeFunc*)type2;
		SListNode* ptr1 = func1->Args->Top;
		SListNode* ptr2 = func2->Args->Top;
		while (ptr1 != NULL && ptr2 != NULL)
		{
			SAstTypeFuncArg* arg1 = (SAstTypeFuncArg*)ptr1->Data;
			SAstTypeFuncArg* arg2 = (SAstTypeFuncArg*)ptr2->Data;
			if (arg1->RefVar != arg2->RefVar || !CmpType(arg1->Arg, arg2->Arg))
				return False;
			ptr1 = ptr1->Next;
			ptr2 = ptr2->Next;
		}
		if (!(ptr1 == NULL && ptr2 == NULL))
			return False;
		if (func1->Ret == NULL && func2->Ret == NULL)
			return True;
		if (func1->Ret == NULL || func2->Ret == NULL)
			return False;
		return CmpType(func1->Ret, func2->Ret);
	}
	if (type_id1 == AstTypeId_TypeGen && type_id2 == AstTypeId_TypeGen)
	{
		if (((SAstTypeGen*)type1)->Kind != ((SAstTypeGen*)type2)->Kind)
			return False;
		return CmpType(((SAstTypeGen*)type1)->ItemType, ((SAstTypeGen*)type2)->ItemType);
	}
	if (type_id1 == AstTypeId_TypeDict && type_id2 == AstTypeId_TypeDict)
		return CmpType(((SAstTypeDict*)type1)->ItemTypeKey, ((SAstTypeDict*)type2)->ItemTypeKey) && CmpType(((SAstTypeDict*)type1)->ItemTypeValue, ((SAstTypeDict*)type2)->ItemTypeValue);
	if (type_id1 == AstTypeId_TypePrim && type_id2 == AstTypeId_TypePrim)
		return ((SAstTypePrim*)type1)->Kind == ((SAstTypePrim*)type2)->Kind;
	if (type_id1 == AstTypeId_TypeUser && type_id2 == AstTypeId_TypeUser)
	{
		ASSERT(((SAst*)type1)->RefItem->TypeId != AstTypeId_Alias && ((SAst*)type2)->RefItem->TypeId != AstTypeId_Alias);
		if (((SAst*)type1)->RefItem->TypeId == AstTypeId_Class && ((SAst*)type2)->RefItem->TypeId == AstTypeId_Class)
		{
			// Check whether they are parent-child relationship.
			SAstClass* class0 = (SAstClass*)((SAst*)type1)->RefItem;
			SAstClass* class1 = (SAstClass*)((SAst*)type2)->RefItem;
			Bool found = False;
			SAstClass* ptr = class1;
			while (ptr != NULL)
			{
				if (ptr == class0)
				{
					found = True;
					break;
				}
				ptr = (SAstClass*)((SAst*)ptr)->RefItem;
			}
			if (found)
				return True;
			ptr = class0;
			while (ptr != NULL)
			{
				if (ptr == class1)
				{
					found = True;
					break;
				}
				ptr = (SAstClass*)((SAst*)ptr)->RefItem;
			}
			if (found)
				return True;
			return False;
		}
		return ((SAst*)type1)->RefItem == ((SAst*)type2)->RefItem;
	}
	if ((type_id1 == AstTypeId_TypeUser && ((SAst*)type1)->RefItem->TypeId == AstTypeId_Enum || type_id1 == AstTypeId_TypeEnumElement) && (type_id2 == AstTypeId_TypeUser && ((SAst*)type2)->RefItem->TypeId == AstTypeId_Enum || type_id2 == AstTypeId_TypeEnumElement) && !(type_id1 == AstTypeId_TypeEnumElement && type_id2 == AstTypeId_TypeEnumElement))
		return True;
	return False;
}

static Bool IsComparable(const SAstType* type, Bool less_or_greater)
{
	// Note: 'null' itself is an incomparable type.
	// The following types can be compared.
	if (((SAst*)type)->TypeId == AstTypeId_TypeBit || IsInt(type) || IsFloat(type) || IsChar(type) || IsEnum(type) || IsClass(type) || IsStr(type) || ((SAst*)type)->TypeId == AstTypeId_TypeEnumElement)
		return True;
	// 'bool' can be just determined whether the values match.
	if (!less_or_greater && IsBool(type))
		return True;
	return False;
}

static U64 BitCast(int size, U64 n)
{
	switch (size)
	{
		case 1: return (U64)(U8)n;
		case 2: return (U64)(U16)n;
		case 4: return (U64)(U32)n;
		case 8: return n;
		default:
			ASSERT(False);
			break;
	}
	return 0;
}

static SAstFunc* AddSpecialFunc(SAstClass* class_, const Char* name)
{
	// Make frameworks for '_dtor', '_copy', '_toBin', and '_fromBin'.
	SAstFunc* func = (SAstFunc*)Alloc(sizeof(SAstFunc));
	InitAst((SAst*)func, AstTypeId_Func, ((SAst*)class_)->Pos);
	((SAst*)func)->Name = name;
	func->AddrTop = NewAddr();
	func->AddrBottom = -1;
	func->PosRowBottom = -1;
	func->DllName = NULL;
	func->DllFuncName = NULL;
	func->FuncAttr = FuncAttr_None;
	func->Args = ListNew();
	func->Ret = NULL;
	func->Stats = ListNew();
	func->RetPoint = AsmLabel();
	{
		SAstArg* me = (SAstArg*)Alloc(sizeof(SAstArg));
		InitAst((SAst*)me, AstTypeId_Arg, ((SAst*)class_)->Pos);
		me->Addr = NewAddr();
		me->Kind = AstArgKind_LocalArg;
		me->RefVar = False;
		me->Expr = NULL;
		{
			SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
			InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)class_)->Pos);
			((SAst*)type)->RefItem = (SAst*)class_;
			me->Type = (SAstType*)type;
		}
		ListAdd(func->Args, me);
	}
	{
		// These functions override functions of the root class.
		SAstClassItem* item = (SAstClassItem*)Alloc(sizeof(SAstClassItem));
		item->Override = True;
		item->Def = (SAst*)func;
		item->ParentItem = NULL;
		item->Addr = -1;
		{
			SAstClass* ptr = (SAstClass*)((SAst*)class_)->RefItem;
			while (((SAst*)ptr)->RefItem != NULL)
				ptr = (SAstClass*)((SAst*)ptr)->RefItem;
			{
				SListNode* ptr2 = ptr->Items->Top;
				while (ptr2 != NULL)
				{
					SAstClassItem* item2 = (SAstClassItem*)ptr2->Data;
					if (wcscmp(item2->Def->Name, name) == 0)
					{
						item->ParentItem = item2;
						break;
					}
					ptr2 = ptr2->Next;
				}
				ASSERT(item->ParentItem != NULL);
				item->Public = item->ParentItem->Public;
			}
		}
		ListAdd(class_->Items, item);
	}
	return func;
}

static SAst* SearchStdItem(const Char* src, const Char* identifier, Bool make_expr_ref)
{
	SAst* ast = (SAst*)DictSearch(Asts, src);
	if (ast == NULL)
	{
		Err(L"EK0000", NULL, src);
		return NULL;
	}
	{
		SAst* ast2 = (SAst*)DictSearch(ast->ScopeChildren, identifier);
		if (ast2 == NULL)
		{
			Err(L"EK0001", NULL, src);
			return NULL;
		}
		if (make_expr_ref)
		{
			SAstExpr* expr = (SAstExpr*)Alloc(sizeof(SAstExpr));
			InitAstExpr(expr, AstTypeId_ExprRef, NewPos(L"kuin", 1, 1));
			((SAst*)expr)->RefItem = ast2;
			return (SAst*)RebuildExprRef(expr);
		}
		return ast2;
	}
}

static SAstExprDot* MakeMeDot(SAstClass* class_, SAstArg* arg, const Char* name)
{
	SAstExprDot* dot = (SAstExprDot*)Alloc(sizeof(SAstExprDot));
	InitAstExpr((SAstExpr*)dot, AstTypeId_ExprDot, ((SAst*)class_)->Pos);
	dot->Member = name;
	dot->ClassItem = NULL;
	{
		SAstExpr* me = (SAstExpr*)Alloc(sizeof(SAstExpr));
		InitAstExpr(me, AstTypeId_ExprRef, ((SAst*)class_)->Pos);
		((SAst*)me)->RefName = L"me";
		((SAst*)me)->RefItem = (SAst*)arg;
		{
			SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
			InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)class_)->Pos);
			((SAst*)type)->RefItem = (SAst*)class_;
			me->Type = (SAstType*)type;
		}
		dot->Var = me;
	}
	return dot;
}

static SAstExprValue* MakeExprNull(const SPos* pos)
{
	SAstExprValue* value = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
	InitAstExpr((SAstExpr*)value, AstTypeId_ExprValue, pos);
	*(S64*)value->Value = 0;
	{
		SAstTypeNull* null_ = (SAstTypeNull*)Alloc(sizeof(SAstTypeNull));
		InitAst((SAst*)null_, AstTypeId_TypeNull, pos);
		((SAstExpr*)value)->Type = (SAstType*)null_;
	}
	return value;
}

static SAstExpr* CacheSubExpr(SList* stats, SAstExpr* ast, const SPos* pos)
{
	if (ast == NULL)
		return NULL;
	if (((SAst*)ast)->TypeId == AstTypeId_ExprRef || ((SAst*)ast)->TypeId == AstTypeId_ExprValue)
		return ast;
	SAstExpr* ref = (SAstExpr*)Alloc(sizeof(SAstExpr));
	InitAstExpr(ref, AstTypeId_ExprRef, pos);
	((SAst*)ref)->RefName = L"$";
	ref->VarKind = AstExprVarKind_LocalVar;
	((SAst*)ref)->AnalyzedCache = (SAst*)ref;
	{
		SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
		InitAst((SAst*)arg, AstTypeId_Arg, pos);
		arg->Addr = NewAddr();
		arg->Kind = AstArgKind_LocalVar;
		arg->RefVar = False;
		arg->Type = ast->Type;
		arg->Expr = NULL;
		((SAst*)arg)->AnalyzedCache = (SAst*)arg;
		((SAst*)ref)->RefItem = (SAst*)arg;
		ref->Type = arg->Type;
	}
	{
		SAstStatDo* do_stat = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
		InitAst((SAst*)do_stat, AstTypeId_StatDo, pos);
		((SAstStat*)do_stat)->AsmTop = NULL;
		((SAstStat*)do_stat)->AsmBottom = NULL;
		((SAstStat*)do_stat)->PosRowBottom = pos->Row;
		{
			SAstExpr2* expr_assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
			InitAstExpr((SAstExpr*)expr_assign, AstTypeId_Expr2, pos);
			expr_assign->Kind = AstExpr2Kind_Assign;
			expr_assign->Children[0] = ref;
			expr_assign->Children[1] = ast;
			do_stat->Expr = (SAstExpr*)expr_assign;
		}
		ListAdd(stats, RebuildDo(do_stat));
	}
	return ref;
}

static void AddDllFunc(const Char* dll_name, const Char* func_name)
{
	SDict* dll = (SDict*)DictSearch(Dlls, dll_name);
	dll = DictAdd(dll, NewStr(NULL, L"%s$%s", dll_name, func_name), func_name);
	Dlls = DictAdd(Dlls, dll_name, dll);
}

static int GetBuildInFuncType(const Char* name)
{
	int idx = BinSearch(BuildInFuncs, (int)(sizeof(BuildInFuncs) / sizeof(Char*)), name);
	if (idx == -1)
		return -1;
	return (int)BuildInFuncs[idx][14];
}

static S64 GetEnumElementValue(SAstExprValue* ast, SAstEnum* enum_)
{
	ASSERT(((SAst*)((SAstExpr*)ast)->Type)->TypeId == AstTypeId_TypeEnumElement);
	RebuildEnum(enum_);
	{
		SListNode* ptr = enum_->Items->Top;
		const Char* name = *(const Char**)ast->Value;
		while (ptr != NULL)
		{
			SAstExpr* item = (SAstExpr*)ptr->Data;
			if (wcscmp(name, ((SAst*)item)->Name) == 0)
			{
				if (((SAst*)item)->TypeId != AstTypeId_ExprValue)
					return 0;
				return *(S64*)((SAstExprValue*)item)->Value;
			}
			ptr = ptr->Next;
		}
		Err(L"EA0059", ((SAst*)ast)->Pos, name);
	}
	return 0;
}

static const Char* GetTypeNameNew(const SAstType* type)
{
	Char buf[1024 + 1];
	size_t len = 0;
	buf[0] = L'\0';
	GetTypeName(buf, &len, 1024, type);
	return NewStr(NULL, L"%s", buf);
}

static const void* AddInitFuncs(const Char* key, const void* value, void* param)
{
	if (key[0] == L'\\')
		return value;
	SList* funcs = (SList*)param;
	if (wcscmp(key, L"draw2d") == 0 ||
		wcscmp(key, L"game") == 0 ||
		wcscmp(key, L"math") == 0 ||
		wcscmp(key, L"net") == 0 ||
		wcscmp(key, L"num") == 0 ||
		wcscmp(key, L"regex") == 0 ||
		wcscmp(key, L"sql") == 0 ||
		wcscmp(key, L"xml") == 0 ||
		wcscmp(key, L"zip") == 0)
	{
		ListAdd(funcs, SearchStdItem(key, L"_init", False));
	}
	return value;
}

static const void* AddFinFuncs(const Char* key, const void* value, void* param)
{
	if (key[0] == L'\\')
		return value;
	SList* funcs = (SList*)param;
	if (wcscmp(key, L"draw2d") == 0 || 
		wcscmp(key, L"net") == 0)
		ListAdd(funcs, SearchStdItem(key, L"_fin", False));
	return value;
}

static SAstFunc* Rebuild(const SAstFunc* main_func)
{
	// Build the entry point.
	const SPos* pos = NewPos(L"kuin", 1, 1);
	SAstFunc* func = (SAstFunc*)Alloc(sizeof(SAstFunc));
	InitAst((SAst*)func, AstTypeId_Func, pos);
	((SAst*)func)->Name = L"$";
	func->AddrTop = NewAddr();
	func->AddrBottom = -1;
	func->PosRowBottom = -1;
	func->DllName = NULL;
	func->DllFuncName = NULL;
	func->FuncAttr = FuncAttr_None;
	func->Args = ListNew();
	func->Ret = NULL;
	func->Stats = ListNew();
	func->RetPoint = NULL;
	{
		SAstStatTry* try_ = (SAstStatTry*)Alloc(sizeof(SAstStatTry));
		InitAst((SAst*)try_, AstTypeId_StatTry, pos);
		((SAstStat*)try_)->AsmTop = NULL;
		((SAstStat*)try_)->AsmBottom = NULL;
		((SAstStat*)try_)->PosRowBottom = -1;
		{
			SAstArg* var = (SAstArg*)Alloc(sizeof(SAstArg));
			InitAst((SAst*)var, AstTypeId_Arg, pos);
			((SAst*)var)->Name = L"$";
			var->Addr = NewAddr();
			var->Kind = AstArgKind_LocalVar;
			var->RefVar = False;
			{
				SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)type, AstTypeId_TypePrim, pos);
				type->Kind = AstTypePrimKind_Int;
				var->Type = (SAstType*)type;
			}
			var->Expr = NULL;
			((SAstStatBreakable*)try_)->BlockVar = var;
		}
		((SAstStatBreakable*)try_)->BreakPoint = AsmLabel();
		{
			SAstStatBlock* block = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
			InitAst((SAst*)block, AstTypeId_StatBlock, pos);
			((SAstStat*)block)->AsmTop = NULL;
			((SAstStat*)block)->AsmBottom = NULL;
			((SAstStat*)block)->PosRowBottom = -1;
			((SAst*)block)->Name = L"$";
			((SAstStatBreakable*)block)->BlockVar = NULL;
			((SAstStatBreakable*)block)->BreakPoint = NULL;
			block->Stats = ListNew();
			try_->StatBlock = block;
		}
		try_->Catches = ListNew();
		{
			SAstStatBlock* block = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
			InitAst((SAst*)block, AstTypeId_StatBlock, pos);
			((SAstStat*)block)->AsmTop = NULL;
			((SAstStat*)block)->AsmBottom = NULL;
			((SAstStat*)block)->PosRowBottom = -1;
			((SAst*)block)->Name = L"$";
			((SAstStatBreakable*)block)->BlockVar = NULL;
			((SAstStatBreakable*)block)->BreakPoint = NULL;
			block->Stats = ListNew();
			try_->FinallyStatBlock = block;
		}
		{
			// Make the program to call 'init' and 'main'.
			SList* funcs = ListNew();
			ListAdd(funcs, SearchStdItem(L"kuin", L"_init", False));
			switch (Option->Env)
			{
				case Env_Wnd:
					ListAdd(funcs, SearchStdItem(L"wnd", L"_init", False));
					break;
				case Env_Cui:
					ListAdd(funcs, SearchStdItem(L"cui", L"_init", False));
					break;
				default:
					ASSERT(False);
					break;
			}
			DictForEach(Asts, AddInitFuncs, funcs);
			ListAdd(funcs, SearchStdItem(L"kuin", L"_initVars", False));
			ListAdd(funcs, main_func);
			{
				SListNode* ptr = funcs->Top;
				while (ptr != NULL)
				{
					SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
					InitAst((SAst*)do_, AstTypeId_StatDo, pos);
					((SAstStat*)do_)->AsmTop = NULL;
					((SAstStat*)do_)->AsmBottom = NULL;
					((SAstStat*)do_)->PosRowBottom = -1;
					{
						SAstExprCall* call = (SAstExprCall*)Alloc(sizeof(SAstExprCall));
						InitAstExpr((SAstExpr*)call, AstTypeId_ExprCall, pos);
						call->Args = ListNew();
						{
							SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
							InitAstExpr(ref_, AstTypeId_ExprRef, pos);
							((SAst*)ref_)->RefItem = (SAst*)ptr->Data;
							call->Func = ref_;
						}
						do_->Expr = (SAstExpr*)call;
					}
					ListAdd(try_->StatBlock->Stats, do_);
					ptr = ptr->Next;
				}
			}
		}
		{
			SAstStatCatch* catch_ = (SAstStatCatch*)Alloc(sizeof(SAstStatCatch));
			InitAst((SAst*)catch_, AstTypeId_StatCatch, pos);
			((SAstStat*)catch_)->AsmTop = NULL;
			((SAstStat*)catch_)->AsmBottom = NULL;
			((SAstStat*)catch_)->PosRowBottom = -1;
			catch_->Conds = ListNew();
			{
				SAstStatBlock* block = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
				InitAst((SAst*)block, AstTypeId_StatBlock, pos);
				((SAstStat*)block)->AsmTop = NULL;
				((SAstStat*)block)->AsmBottom = NULL;
				((SAstStat*)block)->PosRowBottom = -1;
				((SAst*)block)->Name = L"$";
				((SAstStatBreakable*)block)->BlockVar = NULL;
				((SAstStatBreakable*)block)->BreakPoint = NULL;
				block->Stats = ListNew();
				catch_->StatBlock = block;
			}
			{
				SAstExpr** exprs = (SAstExpr**)Alloc(sizeof(SAstExpr*) * 2);
				{
					exprs[0] = (SAstExpr*)Alloc(sizeof(SAstExprValue));
					InitAst((SAst*)exprs[0], AstTypeId_ExprValue, pos);
					exprs[0]->VarKind = AstExprVarKind_Value;
					*(S64*)((SAstExprValue*)exprs[0])->Value = 0;
					{
						SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
						InitAst((SAst*)type, AstTypeId_TypePrim, pos);
						type->Kind = AstTypePrimKind_Int;
						exprs[0]->Type = (SAstType*)type;
					}
				}
				{
					exprs[1] = (SAstExpr*)Alloc(sizeof(SAstExprValue));
					InitAst((SAst*)exprs[1], AstTypeId_ExprValue, pos);
					exprs[1]->VarKind = AstExprVarKind_Value;
					*(S64*)((SAstExprValue*)exprs[1])->Value = _UI32_MAX;
					{
						SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
						InitAst((SAst*)type, AstTypeId_TypePrim, pos);
						type->Kind = AstTypePrimKind_Int;
						exprs[1]->Type = (SAstType*)type;
					}
				}
				ListAdd(catch_->Conds, exprs);
			}
			{
				// Make the program to call 'err'.
				SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
				InitAst((SAst*)do_, AstTypeId_StatDo, pos);
				((SAstStat*)do_)->AsmTop = NULL;
				((SAstStat*)do_)->AsmBottom = NULL;
				((SAstStat*)do_)->PosRowBottom = -1;
				{
					SAstExprCall* call = (SAstExprCall*)Alloc(sizeof(SAstExprCall));
					InitAstExpr((SAstExpr*)call, AstTypeId_ExprCall, pos);
					call->Args = ListNew();
					{
						SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
						InitAstExpr(ref_, AstTypeId_ExprRef, pos);
						((SAst*)ref_)->RefItem = SearchStdItem(L"kuin", L"_err", False);
						call->Func = ref_;
					}
					{
						SAstExprCallArg* excpt = (SAstExprCallArg*)Alloc(sizeof(SAstExprCallArg));
						excpt->RefVar = False;
						excpt->SkipVar = False;
						{
							SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
							InitAstExpr(ref_, AstTypeId_ExprRef, pos);
							((SAst*)ref_)->RefItem = (SAst*)((SAstStatBreakable*)try_)->BlockVar;
							excpt->Arg = ref_;
						}
						ListAdd(call->Args, excpt);
					}
					do_->Expr = (SAstExpr*)call;
				}
				ListAdd(catch_->StatBlock->Stats, do_);
			}
			ListAdd(try_->Catches, catch_);
		}
		{
			// Make the program to call 'fin'.
			SList* funcs = ListNew();
			ListAdd(funcs, SearchStdItem(L"kuin", L"_finVars", False));
			DictForEach(Asts, AddFinFuncs, funcs);
			switch (Option->Env)
			{
				case Env_Wnd:
					ListAdd(funcs, SearchStdItem(L"wnd", L"_fin", False));
					break;
				case Env_Cui:
					ListAdd(funcs, SearchStdItem(L"cui", L"_fin", False));
					break;
				default:
					ASSERT(False);
					break;
			}
			ListAdd(funcs, SearchStdItem(L"kuin", L"_fin", False));
			{
				SListNode* ptr = funcs->Top;
				while (ptr != NULL)
				{
					SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
					InitAst((SAst*)do_, AstTypeId_StatDo, pos);
					((SAstStat*)do_)->AsmTop = NULL;
					((SAstStat*)do_)->AsmBottom = NULL;
					((SAstStat*)do_)->PosRowBottom = -1;
					{
						SAstExprCall* call = (SAstExprCall*)Alloc(sizeof(SAstExprCall));
						InitAstExpr((SAstExpr*)call, AstTypeId_ExprCall, pos);
						call->Args = ListNew();
						{
							SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
							InitAstExpr(ref_, AstTypeId_ExprRef, pos);
							((SAst*)ref_)->RefItem = (SAst*)ptr->Data;
							call->Func = ref_;
						}
						do_->Expr = (SAstExpr*)call;
					}
					ListAdd(try_->FinallyStatBlock->Stats, do_);
					ptr = ptr->Next;
				}
			}
		}
		ListAdd(func->Stats, try_);
	}
	RebuildFunc(func);
	ListAdd(((const SAstRoot*)DictSearch(Asts, NewStr(NULL, L"\\%s", Option->SrcName)))->Items, func);
	DictForEach(Asts, RebuildRootCallback, NULL);
	return func;
}

static const void* RebuildEnumCallback(U64 key, const void* value, void* param)
{
	UNUSED(param);
	RebuildEnum((SAstEnum*)key);
	return value;
}

static const void* RebuildRootCallback(const Char* key, const void* value, void* param)
{
	if (value == DummyPtr)
		return value; // Sub-source.
	UNUSED(param);
	UNUSED(key);
	RebuildRoot((SAstRoot*)value);
	return value;
}

static void RebuildRoot(SAstRoot* ast)
{
	// Initialize and finalize global variables of each source file.
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	{
		SAstFunc* init_vars = (SAstFunc*)SearchStdItem(L"kuin", L"_initVars", False);
		SAstFunc* fin_vars = (SAstFunc*)SearchStdItem(L"kuin", L"_finVars", False);
		{
			SListNode* ptr = ast->Items->Top;
			while (ptr != NULL)
			{
				SAst* item = (SAst*)ptr->Data;
				if ((item->TypeId & AstTypeId_Func) == AstTypeId_Func)
					RebuildFunc((SAstFunc*)item);
				else if (item->TypeId == AstTypeId_Var)
				{
					SAstVar* var = (SAstVar*)item;
					ASSERT(var->Var->Kind == AstArgKind_Global);
					if (var->Var->Expr != NULL)
					{
						// Add initialization processing of global variables to '_initVars'.
						var->Var->Expr = RebuildExpr(var->Var->Expr, False);
						{
							SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
							InitAst((SAst*)do_, AstTypeId_StatDo, ((SAst*)ast)->Pos);
							((SAstStat*)do_)->AsmTop = NULL;
							((SAstStat*)do_)->AsmBottom = NULL;
							((SAstStat*)do_)->PosRowBottom = -1;
							{
								SAstExpr2* assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
								InitAstExpr((SAstExpr*)assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
								assign->Kind = AstExpr2Kind_Assign;
								{
									SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
									InitAstExpr((SAstExpr*)ref_, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
									((SAstExpr*)ref_)->Type = var->Var->Type;
									((SAst*)ref_)->RefItem = (SAst*)var->Var;
									assign->Children[0] = ref_;
								}
								assign->Children[1] = var->Var->Expr;
								do_->Expr = (SAstExpr*)assign;
							}
							ListAdd(init_vars->Stats, RebuildStat((SAstStat*)do_, NULL, NULL));
						}
					}
					if (var->Var->Type != NULL && IsRef(var->Var->Type))
					{
						// Add finalization processing of global variables to '_finVars'.
						SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
						InitAst((SAst*)do_, AstTypeId_StatDo, ((SAst*)ast)->Pos);
						((SAstStat*)do_)->AsmTop = NULL;
						((SAstStat*)do_)->AsmBottom = NULL;
						((SAstStat*)do_)->PosRowBottom = -1;
						{
							SAstExpr2* assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
							InitAstExpr((SAstExpr*)assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
							assign->Kind = AstExpr2Kind_Assign;
							{
								SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
								InitAstExpr((SAstExpr*)ref_, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
								((SAstExpr*)ref_)->Type = var->Var->Type;
								((SAst*)ref_)->RefItem = (SAst*)var->Var;
								assign->Children[0] = ref_;
							}
							assign->Children[1] = (SAstExpr*)MakeExprNull(((SAst*)ast)->Pos);
							do_->Expr = (SAstExpr*)assign;
						}
						ListAdd(fin_vars->Stats, RebuildStat((SAstStat*)do_, NULL, NULL));
					}
				}
				else
					ASSERT(item->TypeId == AstTypeId_Const || item->TypeId == AstTypeId_Alias || item->TypeId == AstTypeId_Class || item->TypeId == AstTypeId_Enum);
				ptr = ptr->Next;
			}
		}
	}
}

static void RebuildFunc(SAstFunc* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (ast->DllName != NULL)
	{
		Bool correct = True;
		if (Option->Env != Env_Wnd)
		{
			if (wcscmp(ast->DllName, L"d0001.knd") == 0)
				correct = False; // lib_wnd
		}
		if (Option->Env != Env_Cui)
		{
			if (wcscmp(ast->DllName, L"d0002.knd") == 0)
				correct = False; // lib_cui
		}
		if (!correct)
			Err(L"EA0062", NULL, ((SAst*)ast)->Pos->SrcName);
		if (ast->DllFuncName != NULL)
			AddDllFunc(ast->DllName, ast->DllFuncName);
		else
			AddDllFunc(ast->DllName, ((SAst*)ast)->Name);
	}
	{
		SListNode* ptr = ast->Args->Top;
		while (ptr != NULL)
		{
			RebuildArg((SAstArg*)ptr->Data);
			ptr = ptr->Next;
		}
	}
	if (ast->Ret != NULL)
		ast->Ret = RebuildType(ast->Ret, NULL);
	ast->Stats = RefreshStats(ast->Stats, ast->Ret, ast);
}

static void RebuildVar(SAstVar* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	RebuildArg(ast->Var);
}

static void RebuildConst(SAstConst* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	RebuildArg(ast->Var);
}

static void RebuildAlias(SAstAlias* ast, SAstAlias* parent)
{
	// Make sure that the enum references do not circulate.
	if (ast == parent)
	{
		Err(L"EA0065", ((SAst*)parent)->Pos, ((SAst*)parent)->Name);
		ast->Type = NULL;
		return;
	}

	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Type = RebuildType(ast->Type, ast);
}

static void RebuildClass(SAstClass* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (((SAst*)ast)->RefItem != NULL)
	{
		SAst* parent = ((SAst*)ast)->RefItem;
		if (parent->TypeId == AstTypeId_Alias)
		{
			RebuildAlias((SAstAlias*)parent, NULL);
			((SAst*)ast)->RefItem = ((SAst*)((SAstAlias*)parent)->Type)->RefItem;
			ASSERT(((SAst*)ast)->RefItem->TypeId == AstTypeId_Class);
		}
		else
		{
			ASSERT(parent->TypeId == AstTypeId_Class);
			RebuildClass((SAstClass*)parent);
		}
	}
	{
		// Make sure that the class references do not circulate.
		SAstClass* parent = ast;
		SDictI* chk = NULL;
		while (parent != NULL)
		{
			U64 addr = (U64)parent;
			if (DictISearch(chk, addr) != NULL)
			{
				Err(L"EA0004", ((SAst*)ast)->Pos, ((SAst*)ast)->Name);
				return;
			}
			chk = DictIAdd(chk, addr, DummyPtr);
			parent = (SAstClass*)((SAst*)parent)->RefItem;
		}
	}
	{
		SAstFunc* dtor = NULL;
		SAstFunc* copy = NULL;
		SAstFunc* to_bin = NULL;
		SAstFunc* from_bin = NULL;
		{
			SListNode* ptr = ast->Items->Top;
			while (ptr != NULL)
			{
				SAstClassItem* item = (SAstClassItem*)ptr->Data;
				const Char* member_name;
				{
					SAst* def = item->Def;
					if (def->TypeId == AstTypeId_Var)
						member_name = ((SAst*)((SAstVar*)def)->Var)->Name;
					else if (def->TypeId == AstTypeId_Const)
						member_name = ((SAst*)((SAstConst*)def)->Var)->Name;
					else
						member_name = def->Name;
				}
				{
					// Check whether functions are overriding another.
					SAstClassItem* parent_item = NULL;
					{
						SAstClass* parent = (SAstClass*)((SAst*)ast)->RefItem;
						while (parent_item == NULL && parent != NULL)
						{
							SListNode* ptr2 = parent->Items->Top;
							while (ptr2 != NULL)
							{
								const Char* parent_name;
								{
									SAst* def = ((SAstClassItem*)ptr2->Data)->Def;
									if (def->TypeId == AstTypeId_Var)
										parent_name = ((SAst*)((SAstVar*)def)->Var)->Name;
									else if (def->TypeId == AstTypeId_Const)
										parent_name = ((SAst*)((SAstConst*)def)->Var)->Name;
									else
										parent_name = def->Name;
								}
								if (wcscmp(member_name, parent_name) == 0)
								{
									parent_item = (SAstClassItem*)ptr2->Data;
								}
								ptr2 = ptr2->Next;
							}
							parent = (SAstClass*)((SAst*)parent)->RefItem;
						}
					}
					if (parent_item == NULL)
					{
						if (item->Override)
						{
							Err(L"EA0005", item->Def->Pos, member_name);
							return;
						}
					}
					else
					{
						if (!item->Override)
						{
							Err(L"EA0006", item->Def->Pos, member_name);
							return;
						}
						if (!(item->Def->TypeId == AstTypeId_Func && parent_item->Def->TypeId == AstTypeId_Func))
						{
							Err(L"EA0007", item->Def->Pos, member_name);
							return;
						}
						if (item->Public != parent_item->Public)
						{
							Err(L"EA0008", item->Def->Pos, member_name);
							return;
						}
						{
							SAstFunc* func1 = (SAstFunc*)item->Def;
							SAstFunc* func2 = (SAstFunc*)parent_item->Def;
							if (func1->Ret == NULL && func2->Ret != NULL || func1->Ret != NULL && func2->Ret == NULL || (func1->Ret != NULL && func2->Ret != NULL && !CmpType(func1->Ret, func2->Ret)) || func1->Args->Len != func2->Args->Len)
							{
								Err(L"EA0009", item->Def->Pos, member_name);
								return;
							}
							{
								int i;
								SListNode* node1 = func1->Args->Top;
								SListNode* node2 = func2->Args->Top;
								for (i = 0; i < func1->Args->Len; i++)
								{
									SAstArg* arg1 = (SAstArg*)node1->Data;
									SAstArg* arg2 = (SAstArg*)node2->Data;
									if (((SAst*)arg1->Type)->TypeId == AstTypeId_TypeUser && ((SAst*)arg1->Type)->RefItem == NULL ||
										((SAst*)arg2->Type)->TypeId == AstTypeId_TypeUser && ((SAst*)arg2->Type)->RefItem == NULL ||
										!CmpType(arg1->Type, arg2->Type) || ((SAst*)arg1)->Name != NULL && ((SAst*)arg2)->Name != NULL && wcscmp(((SAst*)arg1)->Name, ((SAst*)arg2)->Name) != 0 || arg1->RefVar != arg2->RefVar)
									{
										Err(L"EA0009", item->Def->Pos, member_name);
										return;
									}
									node1 = node1->Next;
									node2 = node2->Next;
								}
							}
						}
						item->ParentItem = parent_item;
					}
				}
				ptr = ptr->Next;
			}
		}
		{
			SListNode* ptr = ast->Items->Top;
			while (ptr != NULL)
			{
				SAstClassItem* item = (SAstClassItem*)ptr->Data;
				const Char* member_name;
				{
					SAst* def = item->Def;
					if (def->TypeId == AstTypeId_Var)
						member_name = ((SAst*)((SAstVar*)def)->Var)->Name;
					else if (def->TypeId == AstTypeId_Const)
						member_name = ((SAst*)((SAstConst*)def)->Var)->Name;
					else
						member_name = def->Name;
				}
				if (wcscmp(member_name, L"_dtor") == 0 || wcscmp(member_name, L"_copy") == 0 || wcscmp(member_name, L"_toBin") == 0 || wcscmp(member_name, L"_fromBin") == 0)
				{
					ASSERT(item->Def->TypeId == AstTypeId_Func);
					if (item->Override && (((SAstFunc*)item->Def)->FuncAttr & FuncAttr_Force) == 0)
					{
						Err(L"EA0010", item->Def->Pos, member_name);
						return;
					}
					switch (member_name[1])
					{
						case L'd':
							dtor = (SAstFunc*)item->Def;
							if (item->Override)
								ast->IndirectCreation = True;
							break;
						case L'c': copy = (SAstFunc*)item->Def; break;
						case L't': to_bin = (SAstFunc*)item->Def; break;
						case L'f': from_bin = (SAstFunc*)item->Def; break;
						default:
							ASSERT(False);
					}
					// Skip 'RebuildFunc' to add the contents later.
				}
				else
				{
					// Analyze functions and variables in classes because they can be referred to as instances.
					SAst* def = item->Def;
					if (def->TypeId == AstTypeId_Func)
						RebuildFunc((SAstFunc*)def);
					else if (def->TypeId == AstTypeId_Var)
						RebuildVar((SAstVar*)def);
				}
				ptr = ptr->Next;
			}
		}
		{
			// Get 'me' of special functions.
			if (dtor == NULL)
				dtor = AddSpecialFunc(ast, L"_dtor");
			if (copy == NULL)
			{
				copy = AddSpecialFunc(ast, L"_copy");
				{
					SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
					InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos);
					((SAst*)type)->RefItem = (SAst*)ast;
					copy->Ret = (SAstType*)type;
				}
			}
			if (to_bin == NULL)
			{
				to_bin = AddSpecialFunc(ast, L"_toBin");
				{
					SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
					InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
					{
						SAstTypeBit* type2 = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
						InitAst((SAst*)type2, AstTypeId_TypeBit, ((SAst*)ast)->Pos);
						type2->Size = 1;
						type->ItemType = (SAstType*)type2;
					}
					to_bin->Ret = (SAstType*)type;
				}
			}
			if (from_bin == NULL)
			{
				from_bin = AddSpecialFunc(ast, L"_fromBin");
				{
					// 'bin'.
					SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
					InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos);
					arg->Addr = NewAddr();
					arg->Kind = AstArgKind_LocalArg;
					arg->RefVar = False;
					arg->Expr = NULL;
					{
						SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
						InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
						{
							SAstTypeBit* type2 = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
							InitAst((SAst*)type2, AstTypeId_TypeBit, ((SAst*)ast)->Pos);
							type2->Size = 1;
							type->ItemType = (SAstType*)type2;
						}
						arg->Type = (SAstType*)type;
					}
					ListAdd(from_bin->Args, arg);
				}
				{
					// 'idx'.
					SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
					InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos);
					arg->Addr = NewAddr();
					arg->Kind = AstArgKind_LocalArg;
					arg->RefVar = True;
					arg->Expr = NULL;
					{
						SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
						InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
						type->Kind = AstTypePrimKind_Int;
						arg->Type = (SAstType*)type;
					}
					ListAdd(from_bin->Args, arg);
				}
				from_bin->Ret = ((SAstArg*)from_bin->Args->Top->Data)->Type;
			}
			// The '_dtor' function.
			{
				SAstClass* ptr = ast;
				while (ptr != NULL)
				{
					SListNode* ptr2 = ptr->Items->Top;
					while (ptr2 != NULL)
					{
						SAstClassItem* item = (SAstClassItem*)ptr2->Data;
						if (item->Def->TypeId == AstTypeId_Var && IsRef(((SAstVar*)item->Def)->Var->Type))
						{
							SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
							InitAst((SAst*)do_, AstTypeId_StatDo, ((SAst*)ast)->Pos);
							((SAstStat*)do_)->AsmTop = NULL;
							((SAstStat*)do_)->AsmBottom = NULL;
							((SAstStat*)do_)->PosRowBottom = -1;
							{
								SAstExpr2* assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
								InitAstExpr((SAstExpr*)assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
								assign->Kind = AstExpr2Kind_Assign;
								assign->Children[0] = (SAstExpr*)MakeMeDot(ast, (SAstArg*)dtor->Args->Top->Data, ((SAst*)((SAstVar*)item->Def)->Var)->Name);
								assign->Children[1] = (SAstExpr*)MakeExprNull(((SAst*)ast)->Pos);
								do_->Expr = (SAstExpr*)assign;
							}
							ListAdd(dtor->Stats, RebuildStat((SAstStat*)do_, dtor->Ret, dtor));
						}
						ptr2 = ptr2->Next;
					}
					ptr = (SAstClass*)((SAst*)ptr)->RefItem;
				}
			}
			// The '_copy' function.
			if (!ast->IndirectCreation)
			{
				SAstExpr* result;
				{
					SAstStatVar* var = (SAstStatVar*)Alloc(sizeof(SAstStatVar));
					InitAst((SAst*)var, AstTypeId_StatVar, ((SAst*)ast)->Pos);
					((SAstStat*)var)->AsmTop = NULL;
					((SAstStat*)var)->AsmBottom = NULL;
					((SAstStat*)var)->PosRowBottom = -1;
					{
						SAstVar* var2 = (SAstVar*)Alloc(sizeof(SAstVar));
						InitAst((SAst*)var2, AstTypeId_Var, ((SAst*)ast)->Pos);
						{
							SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
							InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos);
							arg->Addr = NewAddr();
							arg->Kind = AstArgKind_LocalVar;
							arg->RefVar = False;
							{
								SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
								InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos);
								((SAst*)type)->RefItem = (SAst*)ast;
								arg->Type = (SAstType*)type;
							}
							{
								SAstExprNew* new_ = (SAstExprNew*)Alloc(sizeof(SAstExprNew));
								InitAstExpr((SAstExpr*)new_, AstTypeId_ExprNew, ((SAst*)ast)->Pos);
								new_->ItemType = arg->Type;
								new_->AutoCreated = True;
								arg->Expr = (SAstExpr*)new_;
							}
							var2->Var = arg;
						}
						var->Def = var2;
					}
					ListAdd(copy->Stats, RebuildStat((SAstStat*)var, copy->Ret, copy));
					{
						result = (SAstExpr*)Alloc(sizeof(SAstExpr));
						InitAstExpr(result, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
						((SAst*)result)->RefName = L"me";
						((SAst*)result)->RefItem = (SAst*)var->Def->Var;
						{
							SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
							InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos);
							((SAst*)type)->RefItem = (SAst*)ast;
							result->Type = (SAstType*)type;
						}
					}
				}
				{
					SAstClass* ptr = ast;
					while (ptr != NULL)
					{
						SListNode* ptr2 = ptr->Items->Top;
						while (ptr2 != NULL)
						{
							SAstClassItem* item = (SAstClassItem*)ptr2->Data;
							if (item->Def->TypeId == AstTypeId_Var)
							{
								SAstArg* member = ((SAstVar*)item->Def)->Var;
								{
									SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
									InitAst((SAst*)do_, AstTypeId_StatDo, ((SAst*)ast)->Pos);
									((SAstStat*)do_)->AsmTop = NULL;
									((SAstStat*)do_)->AsmBottom = NULL;
									((SAstStat*)do_)->PosRowBottom = -1;
									{
										SAstExpr2* assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
										InitAstExpr((SAstExpr*)assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
										assign->Kind = AstExpr2Kind_Assign;
										{
											SAstExprDot* dot = (SAstExprDot*)Alloc(sizeof(SAstExprDot));
											InitAstExpr((SAstExpr*)dot, AstTypeId_ExprDot, ((SAst*)ast)->Pos);
											dot->Var = result;
											dot->Member = ((SAst*)member)->Name;
											dot->ClassItem = NULL;
											assign->Children[0] = (SAstExpr*)dot;
										}
										if (IsRef(member->Type))
										{
											if (IsClass(member->Type) && ((SAstClass*)((SAst*)member->Type)->RefItem)->IndirectCreation)
												assign->Children[1] = (SAstExpr*)MakeExprNull(((SAst*)ast)->Pos);
											else
											{
												SAstExpr1* copy2 = (SAstExpr1*)Alloc(sizeof(SAstExpr1));
												InitAstExpr((SAstExpr*)copy2, AstTypeId_Expr1, ((SAst*)ast)->Pos);
												copy2->Kind = AstExpr1Kind_Copy;
												copy2->Child = (SAstExpr*)MakeMeDot(ast, (SAstArg*)copy->Args->Top->Data, ((SAst*)member)->Name);
												assign->Children[1] = (SAstExpr*)copy2;
											}
										}
										else
											assign->Children[1] = (SAstExpr*)MakeMeDot(ast, (SAstArg*)copy->Args->Top->Data, ((SAst*)member)->Name);
										do_->Expr = (SAstExpr*)assign;
									}
									ListAdd(copy->Stats, RebuildStat((SAstStat*)do_, copy->Ret, copy));
								}
							}
							ptr2 = ptr2->Next;
						}
						ptr = (SAstClass*)((SAst*)ptr)->RefItem;
					}
				}
				{
					SAstStatRet* ret = (SAstStatRet*)Alloc(sizeof(SAstStatRet));
					InitAst((SAst*)ret, AstTypeId_StatRet, ((SAst*)ast)->Pos);
					((SAstStat*)ret)->AsmTop = NULL;
					((SAstStat*)ret)->AsmBottom = NULL;
					((SAstStat*)ret)->PosRowBottom = -1;
					{
						SAstExprAs* as = (SAstExprAs*)Alloc(sizeof(SAstExprAs));
						InitAstExpr((SAstExpr*)as, AstTypeId_ExprAs, ((SAst*)ast)->Pos);
						as->Kind = AstExprAsKind_As;
						as->Child = result;
						as->ChildType = copy->Ret;
						ret->Value = (SAstExpr*)as;
					}
					ListAdd(copy->Stats, RebuildStat((SAstStat*)ret, copy->Ret, copy));
				}
			}
			// The '_toBin' function.
			if (!ast->IndirectCreation)
			{
				SAstExpr* result;
				{
					SAstStatVar* var = (SAstStatVar*)Alloc(sizeof(SAstStatVar));
					InitAst((SAst*)var, AstTypeId_StatVar, ((SAst*)ast)->Pos);
					((SAstStat*)var)->AsmTop = NULL;
					((SAstStat*)var)->AsmBottom = NULL;
					((SAstStat*)var)->PosRowBottom = -1;
					{
						SAstVar* var2 = (SAstVar*)Alloc(sizeof(SAstVar));
						InitAst((SAst*)var2, AstTypeId_Var, ((SAst*)ast)->Pos);
						{
							SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
							InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos);
							arg->Addr = NewAddr();
							arg->Kind = AstArgKind_LocalVar;
							arg->RefVar = False;
							{
								SAstExprNewArray* new_ = (SAstExprNewArray*)Alloc(sizeof(SAstExprNewArray));
								InitAstExpr((SAstExpr*)new_, AstTypeId_ExprNewArray, ((SAst*)ast)->Pos);
								new_->Idces = ListNew();
								{
									SAstExprValue* value = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
									InitAstExpr((SAstExpr*)value, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
									*(S64*)value->Value = 8;
									{
										SAstTypePrim* prim = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
										InitAst((SAst*)prim, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
										prim->Kind = AstTypePrimKind_Int;
										((SAstExpr*)value)->Type = (SAstType*)prim;
									}
									ListAdd(new_->Idces, value);
								}
								{
									SAstTypeBit* type = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
									InitAst((SAst*)type, AstTypeId_TypeBit, ((SAst*)ast)->Pos);
									type->Size = 1;
									new_->ItemType = (SAstType*)type;
								}
								arg->Expr = (SAstExpr*)new_;
							}
							{
								SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
								InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
								{
									SAstTypeBit* type2 = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
									InitAst((SAst*)type2, AstTypeId_TypeBit, ((SAst*)ast)->Pos);
									type2->Size = 1;
									type->ItemType = (SAstType*)type2;
								}
								arg->Type = (SAstType*)type;
							}
							var2->Var = arg;
						}
						var->Def = var2;
					}
					ListAdd(to_bin->Stats, RebuildStat((SAstStat*)var, to_bin->Ret, to_bin));
					{
						result = (SAstExpr*)Alloc(sizeof(SAstExpr));
						InitAstExpr(result, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
						((SAst*)result)->RefItem = (SAst*)var->Def->Var;
						{
							SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
							InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos);
							((SAst*)type)->RefItem = (SAst*)ast;
							result->Type = (SAstType*)type;
						}
					}
				}
				{
					SAstClass* ptr = ast;
					while (ptr != NULL)
					{
						SListNode* ptr2 = ptr->Items->Top;
						while (ptr2 != NULL)
						{
							SAstClassItem* item = (SAstClassItem*)ptr2->Data;
							if (item->Def->TypeId == AstTypeId_Var)
							{
								SAstArg* member = ((SAstVar*)item->Def)->Var;
								{
									SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
									InitAst((SAst*)do_, AstTypeId_StatDo, ((SAst*)ast)->Pos);
									((SAstStat*)do_)->AsmTop = NULL;
									((SAstStat*)do_)->AsmBottom = NULL;
									((SAstStat*)do_)->PosRowBottom = -1;
									{
										SAstExpr2* assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
										InitAstExpr((SAstExpr*)assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
										assign->Kind = AstExpr2Kind_AssignCat;
										assign->Children[0] = result;
										if (IsClass(member->Type) && ((SAstClass*)((SAst*)member->Type)->RefItem)->IndirectCreation)
										{
											ptr2 = ptr2->Next;
											continue;
										}
										else
										{
											SAstExprToBin* expr = (SAstExprToBin*)Alloc(sizeof(SAstExprToBin));
											InitAstExpr((SAstExpr*)expr, AstTypeId_ExprToBin, ((SAst*)ast)->Pos);
											expr->Child = (SAstExpr*)MakeMeDot(ast, (SAstArg*)to_bin->Args->Top->Data, ((SAst*)member)->Name);
											{
												SAstTypeArray* array_ = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
												InitAst((SAst*)array_, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
												{
													SAstTypeBit* bit = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
													InitAst((SAst*)bit, AstTypeId_TypeBit, ((SAst*)ast)->Pos);
													bit->Size = 1;
													array_->ItemType = (SAstType*)bit;
												}
												expr->ChildType = (SAstType*)array_;
											}
											assign->Children[1] = (SAstExpr*)expr;
										}
										do_->Expr = (SAstExpr*)assign;
									}
									ListAdd(to_bin->Stats, RebuildStat((SAstStat*)do_, to_bin->Ret, to_bin));
								}
							}
							ptr2 = ptr2->Next;
						}
						ptr = (SAstClass*)((SAst*)ptr)->RefItem;
					}
				}
				{
					SAstStatRet* ret = (SAstStatRet*)Alloc(sizeof(SAstStatRet));
					InitAst((SAst*)ret, AstTypeId_StatRet, ((SAst*)ast)->Pos);
					((SAstStat*)ret)->AsmTop = NULL;
					((SAstStat*)ret)->AsmBottom = NULL;
					((SAstStat*)ret)->PosRowBottom = -1;
					ret->Value = result;
					ListAdd(to_bin->Stats, RebuildStat((SAstStat*)ret, to_bin->Ret, to_bin));
				}
			}
			// The '_fromBin' function.
			if (!ast->IndirectCreation)
			{
				SAstExpr* result;
				{
					SAstStatVar* var = (SAstStatVar*)Alloc(sizeof(SAstStatVar));
					InitAst((SAst*)var, AstTypeId_StatVar, ((SAst*)ast)->Pos);
					((SAstStat*)var)->AsmTop = NULL;
					((SAstStat*)var)->AsmBottom = NULL;
					((SAstStat*)var)->PosRowBottom = -1;
					{
						SAstVar* var2 = (SAstVar*)Alloc(sizeof(SAstVar));
						InitAst((SAst*)var2, AstTypeId_Var, ((SAst*)ast)->Pos);
						{
							SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
							InitAst((SAst*)arg, AstTypeId_Arg, ((SAst*)ast)->Pos);
							arg->Addr = NewAddr();
							arg->Kind = AstArgKind_LocalVar;
							arg->RefVar = False;
							arg->Type = ((SAstArg*)from_bin->Args->Top->Data)->Type;
							{
								SAstExprNew* new_ = (SAstExprNew*)Alloc(sizeof(SAstExprNew));
								InitAstExpr((SAstExpr*)new_, AstTypeId_ExprNew, ((SAst*)ast)->Pos);
								new_->ItemType = arg->Type;
								new_->AutoCreated = True;
								arg->Expr = (SAstExpr*)new_;
							}
							var2->Var = arg;
						}
						var->Def = var2;
					}
					ListAdd(from_bin->Stats, RebuildStat((SAstStat*)var, from_bin->Ret, from_bin));
					{
						result = (SAstExpr*)Alloc(sizeof(SAstExpr));
						InitAstExpr(result, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
						((SAst*)result)->RefItem = (SAst*)var->Def->Var;
						((SAst*)result)->RefName = L"me"; // In fact, the referenced member name is not 'me', but it needs to be set to 'me' to access private members.
						{
							SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
							InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos);
							((SAst*)type)->RefItem = (SAst*)ast;
							result->Type = (SAstType*)type;
						}
					}
				}
				{
					SAstClass* ptr = ast;
					while (ptr != NULL)
					{
						SListNode* ptr2 = ptr->Items->Top;
						while (ptr2 != NULL)
						{
							SAstClassItem* item = (SAstClassItem*)ptr2->Data;
							if (item->Def->TypeId == AstTypeId_Var)
							{
								SAstArg* member = ((SAstVar*)item->Def)->Var;
								{
									SAstStatDo* do_ = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
									InitAst((SAst*)do_, AstTypeId_StatDo, ((SAst*)ast)->Pos);
									((SAstStat*)do_)->AsmTop = NULL;
									((SAstStat*)do_)->AsmBottom = NULL;
									((SAstStat*)do_)->PosRowBottom = -1;
									{
										SAstExpr2* assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
										InitAstExpr((SAstExpr*)assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
										assign->Kind = AstExpr2Kind_Assign;
										{
											SAstExprDot* dot = (SAstExprDot*)Alloc(sizeof(SAstExprDot));
											InitAstExpr((SAstExpr*)dot, AstTypeId_ExprDot, ((SAst*)ast)->Pos);
											dot->ClassItem = NULL;
											dot->Var = result;
											dot->Member = ((SAst*)member)->Name;
											assign->Children[0] = (SAstExpr*)dot;
										}
										if (IsClass(member->Type) && ((SAstClass*)((SAst*)member->Type)->RefItem)->IndirectCreation)
											assign->Children[1] = (SAstExpr*)MakeExprNull(((SAst*)ast)->Pos);
										else
										{
											SAstExprFromBin* expr = (SAstExprFromBin*)Alloc(sizeof(SAstExprFromBin));
											InitAstExpr((SAstExpr*)expr, AstTypeId_ExprFromBin, ((SAst*)ast)->Pos);
											{
												SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
												InitAstExpr(ref_, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
												((SAst*)ref_)->RefItem = (SAst*)from_bin->Args->Top->Next->Data;
												expr->Child = ref_;
											}
											expr->ChildType = member->Type;
											{
												SAstExpr* ref_ = (SAstExpr*)Alloc(sizeof(SAstExpr));
												InitAstExpr(ref_, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
												((SAst*)ref_)->RefItem = (SAst*)from_bin->Args->Top->Next->Next->Data;
												expr->Offset = ref_;
											}
											assign->Children[1] = (SAstExpr*)expr;
										}
										do_->Expr = (SAstExpr*)assign;
									}
									ListAdd(from_bin->Stats, RebuildStat((SAstStat*)do_, from_bin->Ret, from_bin));
								}
							}
							ptr2 = ptr2->Next;
						}
						ptr = (SAstClass*)((SAst*)ptr)->RefItem;
					}
				}
				{
					SAstStatRet* ret = (SAstStatRet*)Alloc(sizeof(SAstStatRet));
					InitAst((SAst*)ret, AstTypeId_StatRet, ((SAst*)ast)->Pos);
					((SAstStat*)ret)->AsmTop = NULL;
					((SAstStat*)ret)->AsmBottom = NULL;
					((SAstStat*)ret)->PosRowBottom = -1;
					ret->Value = result;
					ListAdd(from_bin->Stats, RebuildStat((SAstStat*)ret, from_bin->Ret, from_bin));
				}
			}
			RebuildFunc(dtor);
			RebuildFunc(copy);
			RebuildFunc(to_bin);
			RebuildFunc(from_bin);
		}
	}
}

static void RebuildEnum(SAstEnum* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	{
		SAstTypeUser* type = (SAstTypeUser*)Alloc(sizeof(SAstTypeUser));
		{
			InitAst((SAst*)type, AstTypeId_TypeUser, ((SAst*)ast)->Pos);
			((SAst*)type)->AnalyzedCache = (SAst*)type;
			((SAst*)type)->RefItem = (SAst*)ast;
		}
		{
			// Assign values to items.
			S64 default_num = -1;
			SListNode* ptr = ast->Items->Top;
			SDictI* enum_values = NULL;
			while (ptr != NULL)
			{
				SAstExpr* item = (SAstExpr*)ptr->Data;
				const Char* item_name = ((SAst*)item)->Name;
				item = RebuildExpr(item, item->Type == NULL);
				if (item == NULL)
					continue;
				((SAst*)item)->Name = item_name;
				ptr->Data = item;
				if (((SAst*)item)->TypeId != AstTypeId_ExprValue || (item->Type != NULL && !IsInt(item->Type)))
					Err(L"EA0011", ((SAst*)ast)->Pos, ((SAst*)ast)->Name, ((SAst*)item)->Name);
				if (item->Type == NULL)
				{
					// 'Type' is 'NULL' when the value is not set.
					if (default_num == LLONG_MAX)
						Err(L"EA0012", ((SAst*)ast)->Pos, ((SAst*)ast)->Name, ((SAst*)item)->Name);
					default_num++;
					*((S64*)((SAstExprValue*)item)->Value) = default_num;
				}
				else
					default_num = *((S64*)((SAstExprValue*)item)->Value);
				{
					U64 value = *(U64*)((SAstExprValue*)item)->Value;
					if (DictISearch(enum_values, value) != NULL)
						Err(L"EA0013", ((SAst*)ast)->Pos, ((SAst*)ast)->Name, ((SAst*)item)->Name, (S64)value);
					enum_values = DictIAdd(enum_values, value, DummyPtr);
				}
				item->Type = (SAstType*)type; // Cast values of 'int' to 'enum' so as not to being treated as 'int'.
				ptr = ptr->Next;
			}
		}
	}
}

static void RebuildEnumElement(SAstExpr* enum_element, SAstType* type)
{
	ASSERT(((SAst*)enum_element)->TypeId == AstTypeId_ExprValue);
	ASSERT(IsEnum(type));
	*(S64*)((SAstExprValue*)enum_element)->Value = GetEnumElementValue((SAstExprValue*)enum_element, (SAstEnum*)((SAst*)type)->RefItem);
	enum_element->Type = type;
}

static void RebuildArg(SAstArg* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Type = RebuildType(ast->Type, NULL);
	if (ast->Expr != NULL)
	{
		ast->Expr = RebuildExpr(ast->Expr, False);
		if (ast->Expr == NULL)
			return;
		if (ast->Kind == AstArgKind_Global && !(((SAst*)ast->Expr)->TypeId == AstTypeId_ExprValue))
			Err(L"EA0014", ((SAst*)ast)->Pos, ((SAst*)ast)->Name);
		if (ast->Kind == AstArgKind_Const && !(((SAst*)ast->Expr)->TypeId == AstTypeId_ExprValue))
			Err(L"EA0015", ((SAst*)ast)->Pos, ((SAst*)ast)->Name);
		if (!CmpType(ast->Type, ast->Expr->Type))
			Err(L"EA0056", ((SAst*)ast)->Pos);
		else if (((SAst*)ast->Expr->Type)->TypeId == AstTypeId_TypeEnumElement)
			RebuildEnumElement(ast->Expr, ast->Type);
	}
}

static SAstStat* RebuildStat(SAstStat* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (ast == NULL)
		return NULL;
	switch (((SAst*)ast)->TypeId)
	{
		case AstTypeId_StatFunc:
		case AstTypeId_StatConst:
		case AstTypeId_StatAlias:
		case AstTypeId_StatClass:
		case AstTypeId_StatEnum:
			return NULL;
		case AstTypeId_StatVar:
			{
				SAstStatVar* ast2 = (SAstStatVar*)ast;
				RebuildVar(ast2->Def);
				if (((SAst*)ast2->Def->Var)->Name != NULL && wcscmp(((SAst*)ast2->Def->Var)->Name, L"super") == 0)
				{
					ASSERT(parent_func != NULL && ((SAst*)parent_func)->Name != NULL);
					ASSERT(((SAst*)((SAstArg*)ast2->Def->Var)->Type)->TypeId == AstTypeId_TypeFunc);
					SAstClass* ref_class = (SAstClass*)((SAst*)((SAstTypeFuncArg*)((SAstTypeFunc*)((SAstArg*)ast2->Def->Var)->Type)->Args->Top->Data)->Arg)->RefItem;
					ASSERT(((SAst*)ref_class)->TypeId == AstTypeId_Class);
					SListNode* ptr = ref_class->Items->Top;
					while (ptr != NULL)
					{
						const SAstClassItem* item = (const SAstClassItem*)ptr->Data;
						if (item->Def->Name != NULL && wcscmp(item->Def->Name, ((SAst*)parent_func)->Name) == 0) // TODO:
						{
							ASSERT(item->Override);
							SAstExpr* ast_ref = (SAstExpr*)Alloc(sizeof(SAstExpr));
							InitAstExpr(ast_ref, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
							((SAst*)ast_ref)->RefItem = item->ParentItem->Def;
							ast2->Def->Var->Expr = ast_ref;
							break;
						}
						ptr = ptr->Next;
					}
					ASSERT(ptr != NULL);
				}
				if (ast2->Def->Var->Expr == NULL)
					return NULL;
				{
					// Replace initializers with assignment operators.
					SAstStatDo* ast_do = (SAstStatDo*)Alloc(sizeof(SAstStatDo));
					InitAst((SAst*)ast_do, AstTypeId_StatDo, ((SAst*)ast)->Pos);
					((SAstStat*)ast_do)->AsmTop = NULL;
					((SAstStat*)ast_do)->AsmBottom = NULL;
					((SAstStat*)ast_do)->PosRowBottom = ((SAst*)ast_do)->Pos->Row;
					{
						SAstExpr2* ast_assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
						InitAst((SAst*)ast_assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
						ast_assign->Kind = AstExpr2Kind_Assign;
						{
							SAstExpr* ast_ref = (SAstExpr*)Alloc(sizeof(SAstExpr));
							InitAstExpr(ast_ref, AstTypeId_ExprRef, ((SAst*)ast)->Pos);
							((SAst*)ast_ref)->RefItem = (SAst*)ast2->Def->Var;
							ast_assign->Children[0] = ast_ref;
						}
						ast_assign->Children[1] = ast2->Def->Var->Expr;
						ast_do->Expr = (SAstExpr*)ast_assign;
					}
					ast2->Def->Var->Expr = NULL;
					ast = RebuildStat((SAstStat*)ast_do, ret_type, parent_func);
				}
			}
			break;
		case AstTypeId_StatIf: ast = RebuildIf((SAstStatIf*)ast, ret_type, parent_func); break;
		case AstTypeId_StatSwitch: ast = RebuildSwitch((SAstStatSwitch*)ast, ret_type, parent_func); break;
		case AstTypeId_StatWhile: ast = RebuildWhile((SAstStatWhile*)ast, ret_type, parent_func); break;
		case AstTypeId_StatFor: ast = RebuildFor((SAstStatFor*)ast, ret_type, parent_func); break;
		case AstTypeId_StatTry: ast = RebuildTry((SAstStatTry*)ast, ret_type, parent_func); break;
		case AstTypeId_StatThrow: ast = RebuildThrow((SAstStatThrow*)ast); break;
		case AstTypeId_StatBlock: ast = RebuildBlock((SAstStatBlock*)ast, ret_type, parent_func); break;
		case AstTypeId_StatRet: ast = RebuildRet((SAstStatRet*)ast, ret_type); break;
		case AstTypeId_StatDo: ast = RebuildDo((SAstStatDo*)ast); break;
		case AstTypeId_StatBreak: ast = RebuildBreak((SAstStat*)ast, ret_type, parent_func); break;
		case AstTypeId_StatSkip: ast = RebuildSkip((SAstStat*)ast, ret_type, parent_func); break;
		case AstTypeId_StatAssert: ast = RebuildAssert((SAstStatAssert*)ast); break;
		default:
			ASSERT(False);
			break;
	}
	if (ast == NULL)
		return NULL;
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	return ast;
}

static SAstStat* RebuildIf(SAstStatIf* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Cond = RebuildExpr(ast->Cond, False);
	if (ast->Cond != NULL && !IsBool(ast->Cond->Type))
		Err(L"EA0016", ((SAst*)ast->Cond)->Pos);
	ast->StatBlock = (SAstStatBlock*)RebuildBlock(ast->StatBlock, ret_type, parent_func);
	{
		SListNode* ptr = ast->ElIfs->Top;
		while (ptr != NULL)
		{
			SAstStatElIf* elif = (SAstStatElIf*)ptr->Data;
			elif->Cond = RebuildExpr(elif->Cond, False);
			if (elif->Cond != NULL && !IsBool(elif->Cond->Type))
				Err(L"EA0017", ((SAst*)elif->Cond)->Pos);
			elif->StatBlock = (SAstStatBlock*)RebuildBlock(elif->StatBlock, ret_type, parent_func);
			ptr = ptr->Next;
		}
	}
	if (ast->ElseStatBlock != NULL)
		ast->ElseStatBlock = (SAstStatBlock*)RebuildBlock(ast->ElseStatBlock, ret_type, parent_func);
	if (ast->Cond != NULL)
	{
		// Optimize the code.
		SAstStatBlock* stats = NULL;
		if (((SAst*)ast->Cond)->TypeId != AstTypeId_ExprValue)
			return (SAstStat*)ast;
		if (*((S64*)((SAstExprValue*)ast->Cond)->Value) != 0)
			stats = ast->StatBlock;
		if (stats == NULL)
		{
			SListNode* ptr = ast->ElIfs->Top;
			while (ptr != NULL)
			{
				SAstStatElIf* elif = (SAstStatElIf*)ptr->Data;
				if (((SAst*)elif->Cond)->TypeId != AstTypeId_ExprValue)
					return (SAstStat*)ast;
				if (*((S64*)((SAstExprValue*)elif->Cond)->Value) != 0)
				{
					stats = elif->StatBlock;
					break;
				}
				ptr = ptr->Next;
			}
			if (stats == NULL)
			{
				if (ast->ElseStatBlock == NULL)
				{
					SAstStatBlock* block = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
					InitAst((SAst*)block, AstTypeId_StatBlock, ((SAst*)ast)->Pos);
					((SAstStat*)block)->AsmTop = NULL;
					((SAstStat*)block)->AsmBottom = NULL;
					((SAstStat*)block)->PosRowBottom = -1;
					((SAst*)block)->AnalyzedCache = (SAst*)block;
					((SAst*)block)->Name = L"$";
					((SAstStatBreakable*)block)->BlockVar = NULL;
					((SAstStatBreakable*)block)->BreakPoint = NULL;
					block->Stats = ListNew();
					stats = block;
				}
				else
					stats = ast->ElseStatBlock;
			}
		}
		ast->Cond = NULL;
		ast->StatBlock = stats;
	}
	return (SAstStat*)ast;
}

static SAstStat* RebuildSwitch(SAstStatSwitch* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Cond = RebuildExpr(ast->Cond, False);
	if (ast->Cond != NULL)
	{
		if (!IsComparable(ast->Cond->Type, True))
			Err(L"EA0018", ((SAst*)ast->Cond)->Pos);
		((SAstStatBreakable*)ast)->BlockVar->Type = ast->Cond->Type;
	}
	{
		SListNode* ptr = ast->Cases->Top;
		while (ptr != NULL)
		{
			SAstStatCase* case_ = (SAstStatCase*)ptr->Data;
			SListNode* ptr2 = case_->Conds->Top;
			while (ptr2 != NULL)
			{
				SAstExpr** exprs = (SAstExpr**)ptr2->Data;
				exprs[0] = RebuildExpr(exprs[0], False);
				if (ast->Cond != NULL && exprs[0] != NULL)
				{
					if (!CmpType(ast->Cond->Type, exprs[0]->Type))
						Err(L"EA0019", ((SAst*)exprs[0])->Pos);
					else if (((SAst*)exprs[0]->Type)->TypeId == AstTypeId_TypeEnumElement)
						RebuildEnumElement(exprs[0], ast->Cond->Type);
				}
				if (exprs[1] != NULL)
				{
					exprs[1] = RebuildExpr(exprs[1], False);
					if (ast->Cond != NULL && exprs[1] != NULL)
					{
						if (!CmpType(ast->Cond->Type, exprs[1]->Type))
							Err(L"EA0019", ((SAst*)exprs[1])->Pos);
						else if (((SAst*)exprs[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
							RebuildEnumElement(exprs[1], ast->Cond->Type);
					}
				}
				ptr2 = ptr2->Next;
			}
			case_->StatBlock = (SAstStatBlock*)RebuildBlock(case_->StatBlock, ret_type, parent_func);
			ptr = ptr->Next;
		}
	}
	if (ast->DefaultStatBlock != NULL)
		ast->DefaultStatBlock = (SAstStatBlock*)RebuildBlock(ast->DefaultStatBlock, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildWhile(SAstStatWhile* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (ast->Cond != NULL)
	{
		ast->Cond = RebuildExpr(ast->Cond, False);
		if (ast->Cond != NULL && !IsBool(ast->Cond->Type))
			Err(L"EA0020", ((SAst*)ast->Cond)->Pos);
	}
	ast->Stats = RefreshStats(ast->Stats, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildFor(SAstStatFor* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Start = RebuildExpr(ast->Start, False);
	if (ast->Start != NULL)
	{
		if (!IsInt(ast->Start->Type))
			Err(L"EA0021", ((SAst*)ast->Start)->Pos);
		((SAstStatBreakable*)ast)->BlockVar->Type = ast->Start->Type;
	}
	ast->Cond = RebuildExpr(ast->Cond, False);
	if (ast->Cond != NULL && !IsInt(ast->Cond->Type))
		Err(L"EA0022", ((SAst*)ast->Cond)->Pos);
	ast->Step = RebuildExpr(ast->Step, False);
	if (ast->Step != NULL)
	{
		if (!IsInt(ast->Step->Type))
			Err(L"EA0023", ((SAst*)ast->Step)->Pos);
		if (((SAst*)ast->Step)->TypeId != AstTypeId_ExprValue)
			Err(L"EA0024", ((SAst*)ast->Step)->Pos);
		if (*((S64*)((SAstExprValue*)ast->Step)->Value) == 0)
			Err(L"EA0025", ((SAst*)ast->Step)->Pos);
	}
	ast->Stats = RefreshStats(ast->Stats, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildTry(SAstStatTry* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	RebuildArg(((SAstStatBreakable*)ast)->BlockVar);
	ast->StatBlock = (SAstStatBlock*)RebuildBlock(ast->StatBlock, ret_type, parent_func);
	if (ast->Catches->Len != 0)
	{
		SListNode* ptr = ast->Catches->Top;
		while (ptr != NULL)
		{
			SAstStatCatch* catch_ = (SAstStatCatch*)ptr->Data;
			SListNode* ptr2 = catch_->Conds->Top;
			while (ptr2 != NULL)
			{
				SAstExpr** exprs = (SAstExpr**)ptr2->Data;
				exprs[0] = RebuildExpr(exprs[0], False);
				if (exprs[0] != NULL && (!IsInt(exprs[0]->Type) || ((SAst*)exprs[0])->TypeId != AstTypeId_ExprValue))
					Err(L"EA0027", ((SAst*)exprs[0])->Pos);
				if (exprs[1] != NULL)
				{
					exprs[1] = RebuildExpr(exprs[1], False);
					if (exprs[1] != NULL && (!IsInt(exprs[1]->Type) || ((SAst*)exprs[1])->TypeId != AstTypeId_ExprValue))
						Err(L"EA0027", ((SAst*)exprs[1])->Pos);
				}
				ptr2 = ptr2->Next;
			}
			catch_->StatBlock = (SAstStatBlock*)RebuildBlock(catch_->StatBlock, ret_type, parent_func);
			ptr = ptr->Next;
		}
	}
	if (ast->FinallyStatBlock != NULL)
		ast->FinallyStatBlock = (SAstStatBlock*)RebuildBlock(ast->FinallyStatBlock, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildThrow(SAstStatThrow* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Code = RebuildExpr(ast->Code, False);
	if (ast->Code != NULL && !IsInt(ast->Code->Type))
		Err(L"EA0028", ((SAst*)ast->Code)->Pos);
	return (SAstStat*)ast;
}

static SAstStat* RebuildBlock(SAstStatBlock* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Stats = RefreshStats(ast->Stats, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildRet(SAstStatRet* ast, SAstType* ret_type)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (ast->Value == NULL)
	{
		if (ret_type != NULL)
			Err(L"EA0030", ((SAst*)ast)->Pos);
	}
	else
	{
		ast->Value = RebuildExpr(ast->Value, False);
		if (ast->Value != NULL)
		{
			if (ret_type == NULL || !CmpType(ast->Value->Type, ret_type))
				Err(L"EA0031", ((SAst*)ast)->Pos);
			else if (((SAst*)ast->Value->Type)->TypeId == AstTypeId_TypeEnumElement)
				RebuildEnumElement(ast->Value, ret_type);
		}
	}
	return (SAstStat*)ast;
}

static SAstStat* RebuildDo(SAstStatDo* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (ast->Expr != NULL && ((SAst*)ast->Expr)->TypeId == AstTypeId_Expr2)
	{
		SAstExpr2* expr = (SAstExpr2*)ast->Expr;
		// Replace all assignment operators that are not '::' with '::'
		{
			EAstExpr2Kind kind = AstExpr2Kind_Assign;
			switch (expr->Kind)
			{
				case AstExpr2Kind_AssignAdd: kind = AstExpr2Kind_Add; break;
				case AstExpr2Kind_AssignSub: kind = AstExpr2Kind_Sub; break;
				case AstExpr2Kind_AssignMul: kind = AstExpr2Kind_Mul; break;
				case AstExpr2Kind_AssignDiv: kind = AstExpr2Kind_Div; break;
				case AstExpr2Kind_AssignMod: kind = AstExpr2Kind_Mod; break;
				case AstExpr2Kind_AssignPow: kind = AstExpr2Kind_Pow; break;
				case AstExpr2Kind_AssignCat: kind = AstExpr2Kind_Cat; break;
			}
			if (kind != AstExpr2Kind_Assign)
			{
				SAstStatBlock* block = (SAstStatBlock*)Alloc(sizeof(SAstStatBlock));
				InitAst((SAst*)block, AstTypeId_StatBlock, ((SAst*)ast)->Pos);
				((SAstStat*)block)->AsmTop = NULL;
				((SAstStat*)block)->AsmBottom = NULL;
				((SAstStat*)block)->PosRowBottom = -1;
				((SAst*)block)->AnalyzedCache = (SAst*)block;
				((SAst*)block)->Name = L"$";
				((SAstStatBreakable*)block)->BlockVar = NULL;
				((SAstStatBreakable*)block)->BreakPoint = NULL;
				block->Stats = ListNew();
				{
					SAstExpr* lhs = RebuildExpr(expr->Children[0], False);
					if (lhs == NULL)
						return NULL;
					if (((SAst*)lhs)->TypeId == AstTypeId_ExprDot)
						((SAstExprDot*)lhs)->Var = CacheSubExpr(block->Stats, ((SAstExprDot*)lhs)->Var, ((SAst*)ast)->Pos);
					else if (((SAst*)lhs)->TypeId == AstTypeId_ExprArray)
					{
						((SAstExprArray*)lhs)->Var = CacheSubExpr(block->Stats, ((SAstExprArray*)lhs)->Var, ((SAst*)ast)->Pos);
						((SAstExprArray*)lhs)->Idx = CacheSubExpr(block->Stats, ((SAstExprArray*)lhs)->Idx, ((SAst*)ast)->Pos);
					}
					SAstExpr2* expr_assign = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
					InitAstExpr((SAstExpr*)expr_assign, AstTypeId_Expr2, ((SAst*)ast)->Pos);
					expr_assign->Kind = AstExpr2Kind_Assign;
					expr_assign->Children[0] = lhs;
					{
						SAstExpr2* expr_ope = (SAstExpr2*)Alloc(sizeof(SAstExpr2));
						InitAst((SAst*)expr_ope, AstTypeId_Expr2, ((SAst*)ast)->Pos);
						expr_ope->Kind = kind;
						expr_ope->Children[0] = lhs;
						expr_ope->Children[1] = expr->Children[1];
						expr_assign->Children[1] = (SAstExpr*)expr_ope;
					}
					ast->Expr = RebuildExpr((SAstExpr*)expr_assign, True);
					ListAdd(block->Stats, ast);
				}
				return (SAstStat*)block;
			}
		}
	}
	ast->Expr = RebuildExpr(ast->Expr, True);
	if (ast->Expr == NULL)
		return NULL;
	// 'do' needs to end with side effects.
	if (!(((SAst*)ast->Expr)->TypeId == AstTypeId_Expr2 && (((SAstExpr2*)ast->Expr)->Kind == AstExpr2Kind_Assign || ((SAstExpr2*)ast->Expr)->Kind == AstExpr2Kind_Swap) || ((SAst*)ast->Expr)->TypeId == AstTypeId_ExprCall))
		Err(L"EA0032", ((SAst*)ast->Expr)->Pos);
	return (SAstStat*)ast;
}

static SAstStat* RebuildBreak(SAstStat* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (((SAst*)ast)->RefItem == NULL || (((SAst*)ast)->RefItem->TypeId & AstTypeId_StatBreakable) != AstTypeId_StatBreakable)
	{
		Err(L"EA0033", ((SAst*)ast)->Pos);
		return (SAstStat*)DummyPtr;
	}
	((SAst*)ast)->RefItem = (SAst*)RebuildStat((SAstStat*)((SAst*)ast)->RefItem, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildSkip(SAstStat* ast, SAstType* ret_type, SAstFunc* parent_func)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	if (((SAst*)ast)->RefItem == NULL || (((SAst*)ast)->RefItem->TypeId & AstTypeId_StatSkipable) != AstTypeId_StatSkipable)
	{
		Err(L"EA0034", ((SAst*)ast)->Pos);
		return (SAstStat*)DummyPtr;
	}
	((SAst*)ast)->RefItem = (SAst*)RebuildStat((SAstStat*)((SAst*)ast)->RefItem, ret_type, parent_func);
	return (SAstStat*)ast;
}

static SAstStat* RebuildAssert(SAstStatAssert* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstStat*)((SAst*)ast)->AnalyzedCache;
	if (Option->Rls)
		return NULL;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Cond = RebuildExpr(ast->Cond, False);
	if (ast->Cond != NULL && !IsBool(ast->Cond->Type))
		Err(L"EA0035", ((SAst*)ast->Cond)->Pos);
	return (SAstStat*)ast;
}

static SAstType* RebuildType(SAstType* ast, SAstAlias* parent_alias)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstType*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	{
		EAstTypeId type = ((SAst*)ast)->TypeId;
		switch (type)
		{
			case AstTypeId_TypeUser:
				{
					SAst* ref_item = ((SAst*)ast)->RefItem;
					if (ref_item == NULL)
						return NULL;
					if (ref_item->TypeId == AstTypeId_Class)
						RebuildClass((SAstClass*)ref_item);
					else if (ref_item->TypeId == AstTypeId_Enum)
						RebuildEnum((SAstEnum*)ref_item);
					else if (ref_item->TypeId == AstTypeId_Alias)
					{
						RebuildAlias((SAstAlias*)ref_item, parent_alias);
						((SAst*)ast)->AnalyzedCache = (SAst*)((SAstAlias*)ref_item)->Type;
						ast = ((SAstAlias*)ref_item)->Type;
					}
					else
					{
						Err(L"EA0037", ((SAst*)ast)->Pos);
						return NULL;
					}
				}
				break;
			case AstTypeId_TypeArray:
				((SAstTypeArray*)ast)->ItemType = RebuildType(((SAstTypeArray*)ast)->ItemType, parent_alias);
				break;
			case AstTypeId_TypeFunc:
				{
					SAstTypeFunc* ast2 = (SAstTypeFunc*)ast;
					SListNode* ptr = ast2->Args->Top;
					while (ptr != NULL)
					{
						SAstTypeFuncArg* arg = (SAstTypeFuncArg*)ptr->Data;
						arg->Arg = RebuildType(arg->Arg, parent_alias);
						ptr = ptr->Next;
					}
					if (ast2->Ret != NULL)
						ast2->Ret = RebuildType(ast2->Ret, parent_alias);
				}
				break;
			case AstTypeId_TypeGen:
				((SAstTypeGen*)ast)->ItemType = RebuildType(((SAstTypeGen*)ast)->ItemType, parent_alias);
				break;
			case AstTypeId_TypeDict:
				((SAstTypeDict*)ast)->ItemTypeKey = RebuildType(((SAstTypeDict*)ast)->ItemTypeKey, parent_alias);
				((SAstTypeDict*)ast)->ItemTypeValue = RebuildType(((SAstTypeDict*)ast)->ItemTypeValue, parent_alias);
				break;
			default:
				ASSERT(type == AstTypeId_Ast /* Error */ || type == AstTypeId_TypeBit || type == AstTypeId_TypePrim || type == AstTypeId_TypeNull);
				break;
		}
	}
	return (SAstType*)ast;
}

static SAstExpr* RebuildExpr(SAstExpr* ast, Bool nullable)
{
	if (ast == NULL)
		return NULL;
	switch (((SAst*)ast)->TypeId)
	{
		case AstTypeId_Ast: return NULL;
		case AstTypeId_Expr1: ast = RebuildExpr1((SAstExpr1*)ast); break;
		case AstTypeId_Expr2: ast = RebuildExpr2((SAstExpr2*)ast); break;
		case AstTypeId_Expr3: ast = RebuildExpr3((SAstExpr3*)ast); break;
		case AstTypeId_ExprNew: ast = RebuildExprNew((SAstExprNew*)ast); break;
		case AstTypeId_ExprNewArray: ast = RebuildExprNewArray((SAstExprNewArray*)ast); break;
		case AstTypeId_ExprAs: ast = RebuildExprAs((SAstExprAs*)ast); break;
		case AstTypeId_ExprToBin: ast = RebuildExprToBin((SAstExprToBin*)ast); break;
		case AstTypeId_ExprFromBin: ast = RebuildExprFromBin((SAstExprFromBin*)ast); break;
		case AstTypeId_ExprCall: ast = RebuildExprCall((SAstExprCall*)ast); break;
		case AstTypeId_ExprArray: ast = RebuildExprArray((SAstExprArray*)ast); break;
		case AstTypeId_ExprDot: ast = RebuildExprDot((SAstExprDot*)ast); break;
		case AstTypeId_ExprValue: ast = RebuildExprValue((SAstExprValue*)ast); break;
		case AstTypeId_ExprValueArray: ast = RebuildExprValueArray((SAstExprValueArray*)ast); break;
		case AstTypeId_ExprRef: ast = RebuildExprRef((SAstExpr*)ast); break;
		default:
			ASSERT(False);
	}
	if (ast == NULL)
		return NULL;
	if (!nullable && ast->Type == NULL)
	{
		// 'Type' is NULL, for example, when calling a function whose return value is 'void'.
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExpr1(SAstExpr1* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Child = RebuildExpr(ast->Child, False);
	if (ast->Child == NULL)
		return NULL;
	ASSERT(((SAstExpr*)ast)->Type == NULL);
	switch (ast->Kind)
	{
		case AstExpr1Kind_Plus:
			if (IsInt(ast->Child->Type) || IsFloat(ast->Child->Type) || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeBit)
			{
				if (((SAst*)ast->Child)->TypeId == AstTypeId_ExprValue)
				{
					((SAst*)ast)->AnalyzedCache = (SAst*)ast->Child;
					return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
				}
				((SAstExpr*)ast)->Type = ast->Child->Type;
			}
			break;
		case AstExpr1Kind_Minus:
			if (IsInt(ast->Child->Type) || IsFloat(ast->Child->Type) || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeBit)
			{
				if (((SAst*)ast->Child)->TypeId == AstTypeId_ExprValue)
				{
					SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
					InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
					((SAstExpr*)expr)->Type = ast->Child->Type;
					if (IsInt(ast->Child->Type))
						*(S64*)expr->Value = -*(S64*)((SAstExprValue*)ast->Child)->Value;
					else if (IsFloat(ast->Child->Type))
						*(double*)expr->Value = -*(double*)((SAstExprValue*)ast->Child)->Value;
					else
					{
						ASSERT(((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeBit);
						{
							U64 n = *(U64*)((SAstExprValue*)ast->Child)->Value;
							n = 0 - n;
							n = BitCast(((SAstTypeBit*)ast->Child->Type)->Size, n);
							*(U64*)expr->Value = n;
						}
					}
					expr = (SAstExprValue*)RebuildExprValue(expr);
					((SAst*)ast)->AnalyzedCache = (SAst*)expr;
					return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
				}
				((SAstExpr*)ast)->Type = ast->Child->Type;
			}
			break;
		case AstExpr1Kind_Not:
			if (IsBool(ast->Child->Type))
			{
				if (((SAst*)ast->Child)->TypeId == AstTypeId_ExprValue)
				{
					SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
					InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
					((SAstExpr*)expr)->Type = ast->Child->Type;
					*(U64*)expr->Value = *(U64*)((SAstExprValue*)ast->Child)->Value != 0 ? 0 : 1;
					expr = (SAstExprValue*)RebuildExprValue(expr);
					((SAst*)ast)->AnalyzedCache = (SAst*)expr;
					return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
				}
				((SAstExpr*)ast)->Type = ast->Child->Type;
			}
			break;
		case AstExpr1Kind_Copy:
			if (IsClass(ast->Child->Type))
			{
				if (((SAstClass*)((SAst*)ast->Child->Type)->RefItem)->IndirectCreation)
				{
					Err(L"EA0066", ((SAst*)ast)->Pos, ((SAst*)ast->Child->Type)->RefItem->Name);
					return NULL;
				}
				((SAstExpr*)ast)->Type = ast->Child->Type;
			}
			else if (((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeArray || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeGen || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeDict)
				((SAstExpr*)ast)->Type = ast->Child->Type;
			break;
		case AstExpr1Kind_Len:
			if (((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeArray || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeGen || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeDict)
			{
				SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
				type->Kind = AstTypePrimKind_Int;
				((SAstExpr*)ast)->Type = (SAstType*)type;
			}
			break;
		default:
			ASSERT(False);
			break;
	}
	if (((SAstExpr*)ast)->Type == NULL)
	{
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExpr2(SAstExpr2* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Children[0] = RebuildExpr(ast->Children[0], False);
	if (ast->Children[0] == NULL)
		return NULL;
	ast->Children[1] = RebuildExpr(ast->Children[1], False);
	if (ast->Children[1] == NULL)
		return NULL;
	if (!CmpType(ast->Children[0]->Type, ast->Children[1]->Type))
	{
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	{
		Bool correct = False;
		switch (ast->Kind)
		{
			case AstExpr2Kind_Assign:
				if (ast->Children[0]->VarKind == AstExprVarKind_Value)
				{
					Err(L"EA0038", ((SAst*)ast)->Pos);
					return NULL;
				}
				if (IsClass(ast->Children[0]->Type) && IsClass(ast->Children[1]->Type))
				{
					SAstClass* ptr = (SAstClass*)((SAst*)ast->Children[1]->Type)->RefItem;
					while ((SAstClass*)((SAst*)ast->Children[0]->Type)->RefItem != ptr)
					{
						ptr = (SAstClass*)((SAst*)ptr)->RefItem;
						if (ptr == NULL)
						{
							Err(L"EA0056", ((SAst*)ast)->Pos);
							return NULL;
						}
					}
				}
				if (((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
					RebuildEnumElement(ast->Children[1], ast->Children[0]->Type);
				((SAstExpr*)ast)->Type = NULL;
				correct = True;
				break;
			case AstExpr2Kind_Or:
			case AstExpr2Kind_And:
				if (IsBool(ast->Children[0]->Type))
				{
					if (((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue)
					{
						Bool b = *(U64*)((SAstExprValue*)ast->Children[0])->Value != 0;
						// 'true | x' becomes 'true'. 'false & x' becomes 'false'.
						if (ast->Kind == AstExpr2Kind_Or)
							((SAst*)ast)->AnalyzedCache = (SAst*)(b ? ast->Children[0] : ast->Children[1]);
						else
						{
							ASSERT(ast->Kind == AstExpr2Kind_And);
							((SAst*)ast)->AnalyzedCache = (SAst*)(!b ? ast->Children[0] : ast->Children[1]);
						}
						return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
					}
					((SAstExpr*)ast)->Type = ast->Children[0]->Type;
					correct = True;
				}
				break;
			case AstExpr2Kind_LT:
			case AstExpr2Kind_GT:
			case AstExpr2Kind_LE:
			case AstExpr2Kind_GE:
				if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeNull || ((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeNull)
				{
					Err(L"EA0039", ((SAst*)ast)->Pos);
					return NULL;
				}
				else if (IsComparable(ast->Children[0]->Type, True))
				{
					SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
					InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
					type->Kind = AstTypePrimKind_Bool;
					if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeEnumElement)
					{
						if (((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
						{
							ASSERT(((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue);
							Err(L"EA0060", ((SAst*)ast)->Pos, *(const Char**)((SAstExprValue*)ast->Children[0])->Value);
							return NULL;
						}
						RebuildEnumElement(ast->Children[0], ast->Children[1]->Type);
					}
					else if (((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
						RebuildEnumElement(ast->Children[1], ast->Children[0]->Type);
					if (((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue && ((SAst*)ast->Children[1])->TypeId == AstTypeId_ExprValue)
					{
						Bool b = False;
						if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeBit || IsChar(ast->Children[0]->Type))
						{
							U64 n1 = *(U64*)((SAstExprValue*)ast->Children[0])->Value;
							U64 n2 = *(U64*)((SAstExprValue*)ast->Children[1])->Value;
							switch (ast->Kind)
							{
								case AstExpr2Kind_LT: b = n1 < n2; break;
								case AstExpr2Kind_GT: b = n1 > n2; break;
								case AstExpr2Kind_LE: b = n1 <= n2; break;
								case AstExpr2Kind_GE: b = n1 >= n2; break;
								default:
									ASSERT(False);
									break;
							}
						}
						else if (IsInt(ast->Children[0]->Type) || IsEnum(ast->Children[0]->Type))
						{
							S64 n1 = *(S64*)((SAstExprValue*)ast->Children[0])->Value;
							S64 n2 = *(S64*)((SAstExprValue*)ast->Children[1])->Value;
							switch (ast->Kind)
							{
								case AstExpr2Kind_LT: b = n1 < n2; break;
								case AstExpr2Kind_GT: b = n1 > n2; break;
								case AstExpr2Kind_LE: b = n1 <= n2; break;
								case AstExpr2Kind_GE: b = n1 >= n2; break;
								default:
									ASSERT(False);
									break;
							}
						}
						else if (IsFloat(ast->Children[0]->Type))
						{
							double n1 = *(double*)((SAstExprValue*)ast->Children[0])->Value;
							double n2 = *(double*)((SAstExprValue*)ast->Children[1])->Value;
							switch (ast->Kind)
							{
								case AstExpr2Kind_LT: b = n1 < n2; break;
								case AstExpr2Kind_GT: b = n1 > n2; break;
								case AstExpr2Kind_LE: b = n1 <= n2; break;
								case AstExpr2Kind_GE: b = n1 >= n2; break;
								default:
									ASSERT(False);
									break;
							}
						}
						else
						{
							ASSERT(IsStr(ast->Children[0]->Type));
							{
								int cmp = wcscmp(*(const Char**)((SAstExprValue*)ast->Children[0])->Value, *(const Char**)((SAstExprValue*)ast->Children[1])->Value);
								switch (ast->Kind)
								{
									case AstExpr2Kind_LT: b = cmp < 0; break;
									case AstExpr2Kind_GT: b = cmp > 0; break;
									case AstExpr2Kind_LE: b = cmp <= 0; break;
									case AstExpr2Kind_GE: b = cmp >= 0; break;
									default:
										ASSERT(False);
										break;
								}
							}
						}
						{
							SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
							InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
							((SAstExpr*)expr)->Type = (SAstType*)type;
							*(U64*)expr->Value = b ? 1 : 0;
							expr = (SAstExprValue*)RebuildExprValue(expr);
							((SAst*)ast)->AnalyzedCache = (SAst*)expr;
							return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
						}
					}
					((SAstExpr*)ast)->Type = (SAstType*)type;
					correct = True;
				}
				break;
			case AstExpr2Kind_Eq:
			case AstExpr2Kind_NEq:
				if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeNull || ((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeNull)
				{
					Err(L"EA0039", ((SAst*)ast)->Pos);
					return NULL;
				}
				else if (IsComparable(ast->Children[0]->Type, False))
				{
					SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
					InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
					type->Kind = AstTypePrimKind_Bool;
					if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeEnumElement)
					{
						if (((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
						{
							ASSERT(((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue);
							Err(L"EA0060", ((SAst*)ast)->Pos, *(const Char**)((SAstExprValue*)ast->Children[0])->Value);
							return NULL;
						}
						RebuildEnumElement(ast->Children[0], ast->Children[1]->Type);
					}
					else if (((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
						RebuildEnumElement(ast->Children[1], ast->Children[0]->Type);
					if (((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue && ((SAst*)ast->Children[1])->TypeId == AstTypeId_ExprValue)
					{
						Bool b = False;
						if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeBit || IsInt(ast->Children[0]->Type) || IsFloat(ast->Children[0]->Type) || IsChar(ast->Children[0]->Type) || IsBool(ast->Children[0]->Type) || IsEnum(ast->Children[0]->Type))
						{
							U64 n1 = *(U64*)((SAstExprValue*)ast->Children[0])->Value;
							U64 n2 = *(U64*)((SAstExprValue*)ast->Children[1])->Value;
							switch (ast->Kind)
							{
								case AstExpr2Kind_Eq: b = n1 == n2; break;
								case AstExpr2Kind_NEq: b = n1 != n2; break;
								default:
									ASSERT(False);
									break;
							}
						}
						else
						{
							ASSERT(IsStr(ast->Children[0]->Type));
							{
								int cmp = wcscmp(*(const Char**)((SAstExprValue*)ast->Children[0])->Value, *(const Char**)((SAstExprValue*)ast->Children[1])->Value);
								switch (ast->Kind)
								{
									case AstExpr2Kind_Eq: b = cmp == 0; break;
									case AstExpr2Kind_NEq: b = cmp != 0; break;
									default:
										ASSERT(False);
										break;
								}
							}
						}
						{
							SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
							InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
							((SAstExpr*)expr)->Type = (SAstType*)type;
							*(U64*)expr->Value = b ? 1 : 0;
							expr = (SAstExprValue*)RebuildExprValue(expr);
							((SAst*)ast)->AnalyzedCache = (SAst*)expr;
							return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
						}
					}
					((SAstExpr*)ast)->Type = (SAstType*)type;
					correct = True;
				}
				break;
			case AstExpr2Kind_EqRef:
			case AstExpr2Kind_NEqRef:
				if (IsNullable(ast->Children[0]->Type) || ((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeNull)
				{
					SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
					InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
					type->Kind = AstTypePrimKind_Bool;
					((SAstExpr*)ast)->Type = (SAstType*)type;
					correct = True;
				}
				break;
			case AstExpr2Kind_Cat:
				if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeNull || ((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeNull)
				{
					Err(L"EA0040", ((SAst*)ast)->Pos);
					return NULL;
				}
				else if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeArray)
				{
					if (((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue && ((SAst*)ast->Children[1])->TypeId == AstTypeId_ExprValue)
					{
						if (IsStr(ast->Children[0]->Type))
						{
							const Char* s1 = *(const Char**)((SAstExprValue*)ast->Children[0])->Value;
							const Char* s2 = *(const Char**)((SAstExprValue*)ast->Children[1])->Value;
							size_t len1 = wcslen(s1);
							size_t len2 = wcslen(s2);
							Char* buf = (Char*)Alloc(sizeof(Char) * (len1 + len2 + 1));
							Char* p = buf;
							size_t i;
							for (i = 0; i < len1; i++)
							{
								*p = s1[i];
								p++;
							}
							for (i = 0; i < len2; i++)
							{
								*p = s2[i];
								p++;
							}
							*p = L'\0';
							{
								SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
								InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
								((SAstExpr*)expr)->Type = ast->Children[0]->Type;
								*(Char**)expr->Value = buf;
								expr = (SAstExprValue*)RebuildExprValue(expr);
								((SAst*)ast)->AnalyzedCache = (SAst*)expr;
								return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
							}
						}
					}
					((SAstExpr*)ast)->Type = ast->Children[0]->Type;
					correct = True;
				}
				break;
			case AstExpr2Kind_Add:
			case AstExpr2Kind_Sub:
			case AstExpr2Kind_Mul:
			case AstExpr2Kind_Div:
			case AstExpr2Kind_Mod:
				if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeBit || IsInt(ast->Children[0]->Type) || IsFloat(ast->Children[0]->Type))
				{
					if (((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue && ((SAst*)ast->Children[1])->TypeId == AstTypeId_ExprValue)
					{
						SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
						InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
						((SAstExpr*)expr)->Type = ast->Children[0]->Type;
						if (((SAst*)ast->Children[0]->Type)->TypeId == AstTypeId_TypeBit)
						{
							U64 n1 = *(U64*)((SAstExprValue*)ast->Children[0])->Value;
							U64 n2 = *(U64*)((SAstExprValue*)ast->Children[1])->Value;
							switch (ast->Kind)
							{
								case AstExpr2Kind_Add: n1 += n2; break;
								case AstExpr2Kind_Sub: n1 -= n2; break;
								case AstExpr2Kind_Mul: n1 *= n2; break;
								case AstExpr2Kind_Div:
									if (n2 == 0)
									{
										Err(L"EA0063", ((SAst*)ast)->Pos);
										return NULL;
									}
									n1 /= n2;
									break;
								case AstExpr2Kind_Mod:
									if (n2 == 0)
									{
										Err(L"EA0063", ((SAst*)ast)->Pos);
										return NULL;
									}
									n1 %= n2;
									break;
								default:
									ASSERT(False);
									break;
							}
							n1 = BitCast(((SAstTypeBit*)ast->Children[0]->Type)->Size, n1);
							*(U64*)expr->Value = n1;
						}
						else if (IsInt(ast->Children[0]->Type))
						{
							S64 n1 = *(S64*)((SAstExprValue*)ast->Children[0])->Value;
							S64 n2 = *(S64*)((SAstExprValue*)ast->Children[1])->Value;
							switch (ast->Kind)
							{
								case AstExpr2Kind_Add:
									if (AddAsm(&n1, n2))
									{
										Err(L"EA0064", ((SAst*)ast)->Pos);
										return NULL;
									}
									break;
								case AstExpr2Kind_Sub:
									if (SubAsm(&n1, n2))
									{
										Err(L"EA0064", ((SAst*)ast)->Pos);
										return NULL;
									}
									break;
								case AstExpr2Kind_Mul:
									if (MulAsm(&n1, n2))
									{
										Err(L"EA0064", ((SAst*)ast)->Pos);
										return NULL;
									}
									break;
								case AstExpr2Kind_Div:
									if (n2 == 0)
									{
										Err(L"EA0063", ((SAst*)ast)->Pos);
										return NULL;
									}
									n1 /= n2;
									break;
								case AstExpr2Kind_Mod:
									if (n2 == 0)
									{
										Err(L"EA0063", ((SAst*)ast)->Pos);
										return NULL;
									}
									n1 %= n2;
									break;
								default:
									ASSERT(False);
									break;
							}
							*(S64*)expr->Value = n1;
						}
						else
						{
							ASSERT(IsFloat(ast->Children[0]->Type));
							{
								double n1 = *(double*)((SAstExprValue*)ast->Children[0])->Value;
								double n2 = *(double*)((SAstExprValue*)ast->Children[1])->Value;
								switch (ast->Kind)
								{
									case AstExpr2Kind_Add: n1 += n2; break;
									case AstExpr2Kind_Sub: n1 -= n2; break;
									case AstExpr2Kind_Mul: n1 *= n2; break;
									case AstExpr2Kind_Div:
										if (n1 == 0.0 && n2 == 0.0)
										{
											Err(L"EA0063", ((SAst*)ast)->Pos);
											return NULL;
										}
										n1 /= n2;
										break;
									case AstExpr2Kind_Mod:
										if (n1 == 0.0 && n2 == 0.0)
										{
											Err(L"EA0063", ((SAst*)ast)->Pos);
											return NULL;
										}
										n1 = fmod(n1, n2);
										break;
									default:
										ASSERT(False);
										break;
								}
								*(double*)expr->Value = n1;
							}
						}
						expr = (SAstExprValue*)RebuildExprValue(expr);
						((SAst*)ast)->AnalyzedCache = (SAst*)expr;
						return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
					}
					((SAstExpr*)ast)->Type = ast->Children[0]->Type;
					correct = True;
				}
				break;
			case AstExpr2Kind_Pow:
				if (IsInt(ast->Children[0]->Type) || IsFloat(ast->Children[0]->Type))
				{
					((SAstExpr*)ast)->Type = ast->Children[0]->Type;
					correct = True;
				}
				break;
			case AstExpr2Kind_Swap:
				if (ast->Children[0]->VarKind == AstExprVarKind_Value || ast->Children[1]->VarKind == AstExprVarKind_Value)
				{
					Err(L"EA0041", ((SAst*)ast)->Pos);
					return NULL;
				}
				if (!(IsClass(ast->Children[0]->Type) && ((SAst*)ast->Children[0]->Type)->RefItem != ((SAst*)ast->Children[1]->Type)->RefItem))
				{
					((SAstExpr*)ast)->Type = ast->Children[0]->Type;
					correct = True;
				}
				break;
			default:
				break;
		}
		if (!correct)
		{
			Err(L"EA0056", ((SAst*)ast)->Pos);
			return NULL;
		}
	}
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExpr3(SAstExpr3* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Children[0] = RebuildExpr(ast->Children[0], False);
	if (ast->Children[0] == NULL)
		return NULL;
	ast->Children[1] = RebuildExpr(ast->Children[1], False);
	if (ast->Children[1] == NULL)
		return NULL;
	ast->Children[2] = RebuildExpr(ast->Children[2], False);
	if (ast->Children[2] == NULL)
		return NULL;
	if (!IsBool(ast->Children[0]->Type))
	{
		Err(L"EA0042", ((SAst*)ast)->Pos);
		return NULL;
	}
	if (!CmpType(ast->Children[1]->Type, ast->Children[2]->Type))
	{
		Err(L"EA0043", ((SAst*)ast)->Pos);
		return NULL;
	}
	if (((SAst*)ast->Children[0])->TypeId == AstTypeId_ExprValue)
	{
		((SAst*)ast)->AnalyzedCache = (SAst*)(*((S64*)((SAstExprValue*)ast->Children[0])->Value) != 0 ? ast->Children[1] : ast->Children[2]);
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	}
	if (((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeEnumElement)
	{
		if (((SAst*)ast->Children[2]->Type)->TypeId == AstTypeId_TypeEnumElement)
		{
			ASSERT(((SAst*)ast->Children[1])->TypeId == AstTypeId_ExprValue);
			Err(L"EA0060", ((SAst*)ast)->Pos, *(const Char**)((SAstExprValue*)ast->Children[1])->Value);
			return NULL;
		}
		RebuildEnumElement(ast->Children[1], ast->Children[2]->Type);
	}
	else if (((SAst*)ast->Children[2]->Type)->TypeId == AstTypeId_TypeEnumElement)
		RebuildEnumElement(ast->Children[2], ast->Children[1]->Type);
	((SAstExpr*)ast)->Type = ((SAst*)ast->Children[1]->Type)->TypeId == AstTypeId_TypeNull ? ast->Children[2]->Type : ast->Children[1]->Type;
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprNew(SAstExprNew* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->ItemType = RebuildType(ast->ItemType, NULL);
	if (ast->ItemType == NULL)
		return NULL;
	if (IsClass(ast->ItemType))
	{
		if (!ast->AutoCreated && ((SAstClass*)((SAst*)ast->ItemType)->RefItem)->IndirectCreation)
		{
			Err(L"EA0066", ((SAst*)ast)->Pos, ((SAst*)ast->ItemType)->RefItem->Name);
			return NULL;
		}
	}
	else if (!(((SAst*)ast->ItemType)->TypeId == AstTypeId_TypeGen || ((SAst*)ast->ItemType)->TypeId == AstTypeId_TypeDict))
	{
		Err(L"EA0044", ((SAst*)ast)->Pos);
		return NULL;
	}
	((SAstExpr*)ast)->Type = ast->ItemType;
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprNewArray(SAstExprNewArray* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	{
		SListNode* ptr = ast->Idces->Top;
		while (ptr != NULL)
		{
			ptr->Data = RebuildExpr((SAstExpr*)ptr->Data, False);
			if (ptr->Data != NULL && !IsInt(((SAstExpr*)ptr->Data)->Type))
			{
				Err(L"EA0045", ((SAst*)ptr->Data)->Pos);
				return NULL;
			}
			ptr = ptr->Next;
		}
	}
	ast->ItemType = RebuildType(ast->ItemType, NULL);
	if (ast->ItemType == NULL)
		return NULL;
	{
		SAstType* type = ast->ItemType;
		int i;
		for (i = 0; i < ast->Idces->Len; i++)
		{
			SAstTypeArray* type2 = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
			InitAst((SAst*)type2, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
			type2->ItemType = type;
			type = (SAstType*)type2;
		}
		((SAstExpr*)ast)->Type = type;
	}
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprAs(SAstExprAs* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Child = RebuildExpr(ast->Child, False);
	if (ast->Child == NULL)
		return NULL;
	ast->ChildType = RebuildType(ast->ChildType, NULL);
	if (ast->ChildType == NULL)
		return NULL;
	ASSERT(((SAstExpr*)ast)->Type == NULL);
	switch (ast->Kind)
	{
		case AstExprAsKind_As:
			{
				SAstType* t1 = ast->Child->Type;
				SAstType* t2 = ast->ChildType;
				if (((SAst*)t1)->TypeId == AstTypeId_TypeBit)
				{
					if (((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2) || IsFloat(t2) || IsChar(t2) || IsBool(t2) || IsEnum(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (IsInt(t1))
				{
					if (((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2) || IsFloat(t2) || IsChar(t2) || IsBool(t2) || IsEnum(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (IsFloat(t1))
				{
					if (((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2) || IsFloat(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (IsChar(t1))
				{
					if (((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2) || IsChar(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (IsBool(t1))
				{
					if (((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2) || IsBool(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (IsClass(t1))
				{
					if (IsClass(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (IsEnum(t1))
				{
					if (((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2) || IsEnum(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				else if (((SAst*)t1)->TypeId == AstTypeId_TypeEnumElement)
				{
					if (IsEnum(t2))
						((SAstExpr*)ast)->Type = t2;
				}
				if (((SAstExpr*)ast)->Type != NULL)
				{
					if (((SAst*)ast->Child)->TypeId == AstTypeId_ExprValue)
					{
						SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
						InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
						((SAstExpr*)expr)->Type = ((SAstExpr*)ast)->Type;
						if (((SAst*)t1)->TypeId == AstTypeId_TypeBit || IsChar(t1) || IsBool(t1))
						{
							U64 n = *(U64*)((SAstExprValue*)ast->Child)->Value;
							if (((SAst*)t2)->TypeId == AstTypeId_TypeBit)
								*(U64*)expr->Value = BitCast(((SAstTypeBit*)t2)->Size, n);
							else if (IsInt(t2) || IsEnum(t2))
								*(S64*)expr->Value = (S64)n;
							else if (IsFloat(t2))
								*(double*)expr->Value = (double)n;
							else if (IsChar(t2))
								*(U64*)expr->Value = BitCast(2, n);
							else
							{
								ASSERT(IsBool(t2));
								*(U64*)expr->Value = n != 0 ? 1 : 0;
							}
						}
						else if (IsInt(t1) || IsEnum(t1))
						{
							S64 n = *(S64*)((SAstExprValue*)ast->Child)->Value;
							if (((SAst*)t2)->TypeId == AstTypeId_TypeBit)
								*(U64*)expr->Value = BitCast(((SAstTypeBit*)t2)->Size, (U64)n);
							else if (IsInt(t2) || IsEnum(t2))
								*(S64*)expr->Value = n;
							else if (IsFloat(t2))
								*(double*)expr->Value = (double)n;
							else if (IsChar(t2))
								*(U64*)expr->Value = BitCast(2, (U64)n);
							else
							{
								ASSERT(IsBool(t2));
								*(U64*)expr->Value = n != 0 ? 1 : 0;
							}
						}
						else if (((SAst*)t1)->TypeId == AstTypeId_TypeEnumElement)
						{
							ASSERT(((SAst*)t2)->RefItem->TypeId == AstTypeId_Enum);
							*(S64*)expr->Value = GetEnumElementValue((SAstExprValue*)ast->Child, (SAstEnum*)((SAst*)t2)->RefItem);
						}
						else
						{
							ASSERT(IsFloat(t1));
							{
								double n = *(double*)((SAstExprValue*)ast->Child)->Value;
								if (((SAst*)t2)->TypeId == AstTypeId_TypeBit)
									*(U64*)expr->Value = BitCast(((SAstTypeBit*)t2)->Size, (U64)n);
								else if (IsFloat(t2))
									*(double*)expr->Value = n;
								else
								{
									ASSERT(IsInt(t2));
									*(U64*)expr->Value = (U64)n;
								}
							}
						}
						expr = (SAstExprValue*)RebuildExprValue(expr);
						((SAst*)ast)->AnalyzedCache = (SAst*)expr;
						return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
					}
				}
			}
			break;
		case AstExprAsKind_Is:
		case AstExprAsKind_NIs:
			if (IsClass(ast->Child->Type) && IsClass(ast->ChildType))
			{
				SAstTypePrim* type = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
				InitAst((SAst*)type, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
				type->Kind = AstTypePrimKind_Bool;
				((SAstExpr*)ast)->Type = (SAstType*)type;
			}
			break;
		default:
			ASSERT(False);
			break;
	}
	if (((SAstExpr*)ast)->Type == NULL)
	{
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprToBin(SAstExprToBin* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Child = RebuildExpr(ast->Child, False);
	if (ast->Child == NULL)
		return NULL;
	if (((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeNull || ((SAst*)ast->Child->Type)->TypeId == AstTypeId_TypeEnumElement)
	{
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	if (IsClass(ast->Child->Type))
	{
		if (((SAstClass*)((SAst*)ast->Child->Type)->RefItem)->IndirectCreation)
		{
			Err(L"EA0066", ((SAst*)ast)->Pos, ((SAst*)ast->Child->Type)->RefItem->Name);
			return NULL;
		}
	}
	if (((SAst*)ast->ChildType)->TypeId != AstTypeId_TypeArray || ((SAst*)((SAstTypeArray*)ast->ChildType)->ItemType)->TypeId != AstTypeId_TypeBit || ((SAstTypeBit*)((SAstTypeArray*)ast->ChildType)->ItemType)->Size != 1)
	{
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	((SAstExpr*)ast)->Type = ast->ChildType;
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprFromBin(SAstExprFromBin* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Child = RebuildExpr(ast->Child, False);
	if (ast->Child == NULL)
		return NULL;
	if (((SAst*)ast->Child->Type)->TypeId != AstTypeId_TypeArray || ((SAst*)((SAstTypeArray*)ast->Child->Type)->ItemType)->TypeId != AstTypeId_TypeBit || ((SAstTypeBit*)((SAstTypeArray*)ast->Child->Type)->ItemType)->Size != 1)
	{
		Err(L"EA0056", ((SAst*)ast)->Pos);
		return NULL;
	}
	if (IsClass(ast->ChildType))
	{
		if (((SAstClass*)((SAst*)ast->ChildType)->RefItem)->IndirectCreation)
		{
			Err(L"EA0066", ((SAst*)ast)->Pos, ((SAst*)ast->ChildType)->RefItem->Name);
			return NULL;
		}
	}
	((SAstExpr*)ast)->Type = ast->ChildType;
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	ast->Offset = RebuildExpr(ast->Offset, False);
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprCall(SAstExprCall* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Func = RebuildExpr(ast->Func, False);
	if (ast->Func == NULL)
		return NULL;
	{
		SAstTypeFunc* type = (SAstTypeFunc*)ast->Func->Type;
		if (((SAst*)type)->TypeId == AstTypeId_TypeFunc && (type->FuncAttr & FuncAttr_MakeInstance) != 0)
		{
			// Make an instance and add it to the second argument when '_make_instance' is specified.
			SAstExprCallArg* value_type = (SAstExprCallArg*)Alloc(sizeof(SAstExprCallArg));
			ASSERT(type->Ret != NULL);
			{
				SAstExprNew* expr = (SAstExprNew*)Alloc(sizeof(SAstExprNew));
				InitAstExpr((SAstExpr*)expr, AstTypeId_ExprNew, ((SAst*)ast)->Pos);
				expr->ItemType = type->Ret;
				expr->AutoCreated = True;
				value_type->Arg = RebuildExpr((SAstExpr*)expr, False);
			}
			value_type->RefVar = False;
			value_type->SkipVar = False;
			ListIns(ast->Args, ast->Args->Top, value_type);
		}
		if (((SAst*)ast->Func)->TypeId == AstTypeId_ExprDot && ((SAst*)ast->Func->Type)->TypeId == AstTypeId_TypeFunc)
		{
			{
				SAstExprCallArg* me = (SAstExprCallArg*)Alloc(sizeof(SAstExprCallArg));
				me->Arg = ((SAstExprDot*)ast->Func)->Var;
				me->RefVar = False;
				me->SkipVar = False;
				ListIns(ast->Args, ast->Args->Top, me);
			}
			if ((type->FuncAttr & FuncAttr_AnyType) != 0)
			{
				// Add the type of 'me' to the second argument when '_any_type' is specified.
				SAstExprCallArg* me_type = (SAstExprCallArg*)Alloc(sizeof(SAstExprCallArg));
				{
					SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
					InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
					*(S64*)expr->Value = 0;
					{
						SAstTypePrim* prim = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
						InitAst((SAst*)prim, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
						prim->Kind = AstTypePrimKind_Int;
						((SAstExpr*)expr)->Type = (SAstType*)prim;
					}
					me_type->Arg = RebuildExpr((SAstExpr*)expr, False);
				}
				me_type->RefVar = False;
				me_type->SkipVar = False;
				ListIns(ast->Args, ast->Args->Top->Next, me_type);
			}
			if ((type->FuncAttr & FuncAttr_TakeKeyValue) != 0)
			{
				// Add the type of 'value' to the third argument when '_take_key_value' is specified.
				SAstExprCallArg* value_type = (SAstExprCallArg*)Alloc(sizeof(SAstExprCallArg));
				{
					SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
					InitAstExpr((SAstExpr*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
					*(S64*)expr->Value = 0;
					{
						SAstTypePrim* prim = (SAstTypePrim*)Alloc(sizeof(SAstTypePrim));
						InitAst((SAst*)prim, AstTypeId_TypePrim, ((SAst*)ast)->Pos);
						prim->Kind = AstTypePrimKind_Int;
						((SAstExpr*)expr)->Type = (SAstType*)prim;
					}
					value_type->Arg = RebuildExpr((SAstExpr*)expr, False);
				}
				value_type->RefVar = False;
				value_type->SkipVar = False;
				ListIns(ast->Args, ast->Args->Top->Next->Next, value_type);
			}
		}
		else
		{
			if (((SAst*)type)->TypeId != AstTypeId_TypeFunc)
			{
				Err(L"EA0046", ((SAst*)ast)->Pos);
				return NULL;
			}
			type = (SAstTypeFunc*)ast->Func->Type;
		}
		((SAstExpr*)ast)->Type = type->Ret;
		if (ast->Args->Len != type->Args->Len)
		{
			Err(L"EA0047", ((SAst*)ast)->Pos, type->Args->Len, ast->Args->Len, GetTypeNameNew((const SAstType*)type));
			return NULL;
		}
		{
			SListNode* ptr_expr = ast->Args->Top;
			SListNode* ptr_type = type->Args->Top;
			int n = 0;
			while (ptr_expr != NULL)
			{
				SAstExprCallArg* arg_expr = (SAstExprCallArg*)ptr_expr->Data;
				SAstTypeFuncArg* arg_type = (SAstTypeFuncArg*)ptr_type->Data;
				if (arg_expr->SkipVar)
					((SAstArg*)((SAst*)arg_expr->Arg)->RefItem)->Type = arg_type->Arg;
				arg_expr->Arg = RebuildExpr(arg_expr->Arg, False);
				if (arg_expr->Arg != NULL)
				{
					if (arg_expr->RefVar && !arg_expr->SkipVar && arg_expr->Arg->VarKind == AstExprVarKind_Value)
					{
						Err(L"EA0067", ((SAst*)ast)->Pos, n + 1);
						return NULL;
					}
					if (arg_expr->RefVar != arg_type->RefVar || !CmpType(arg_expr->Arg->Type, arg_type->Arg))
					{
						Err(L"EA0048", ((SAst*)ast)->Pos, n + 1, arg_type->RefVar ? L"&" : L"", GetTypeNameNew(arg_type->Arg), arg_expr->RefVar ? L"&" : L"", GetTypeNameNew(arg_expr->Arg->Type));
						return NULL;
					}
					if (((SAst*)arg_expr->Arg->Type)->TypeId == AstTypeId_TypeEnumElement)
						RebuildEnumElement(arg_expr->Arg, arg_type->Arg);
				}
				ptr_expr = ptr_expr->Next;
				ptr_type = ptr_type->Next;
				n++;
			}
		}
		((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	}
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprArray(SAstExprArray* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Var = RebuildExpr(ast->Var, False);
	if (ast->Var == NULL)
		return NULL;
	if (((SAst*)ast->Var->Type)->TypeId != AstTypeId_TypeArray)
	{
		Err(L"EA0049", ((SAst*)ast)->Pos);
		return NULL;
	}
	ast->Idx = RebuildExpr(ast->Idx, False);
	if (ast->Idx == NULL)
		return NULL;
	if (!IsInt(ast->Idx->Type))
	{
		Err(L"EA0050", ((SAst*)ast->Idx)->Pos);
		return NULL;
	}
	((SAstExpr*)ast)->Type = ((SAstTypeArray*)ast->Var->Type)->ItemType;
	((SAstExpr*)ast)->VarKind = AstExprVarKind_GlobalVar;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprDot(SAstExprDot* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ast->Var = RebuildExpr(ast->Var, False);
	if (ast->Var == NULL)
		return NULL;
	if (IsClass(ast->Var->Type))
	{
		Bool found = False;
		SAstClass* ptr = (SAstClass*)((SAst*)ast->Var->Type)->RefItem;
		while (ptr != NULL)
		{
			SListNode* ptr2 = ptr->Items->Top;
			while (ptr2 != NULL)
			{
				SAstClassItem* item = (SAstClassItem*)ptr2->Data;
				if (item->Def->TypeId == AstTypeId_Var && wcscmp(ast->Member, ((SAst*)((SAstVar*)item->Def)->Var)->Name) == 0)
				{
					((SAstExpr*)ast)->Type = ((SAstVar*)item->Def)->Var->Type;
					((SAstExpr*)ast)->VarKind = AstExprVarKind_GlobalVar; // Properties' addresses are treated as those of global variables.
					found = True;
				}
				else if (item->Def->TypeId == AstTypeId_Func && wcscmp(ast->Member, ((SAst*)item->Def)->Name) == 0)
				{
					{
						SAstTypeFunc* type = (SAstTypeFunc*)Alloc(sizeof(SAstTypeFunc));
						InitAst((SAst*)type, AstTypeId_TypeFunc, ((SAst*)ast)->Pos);
						type->FuncAttr = ((SAstFunc*)item->Def)->FuncAttr;
						type->Args = ListNew();
						{
							SListNode* ptr3 = ((SAstFunc*)item->Def)->Args->Top;
							while (ptr3 != NULL)
							{
								SAstArg* arg = (SAstArg*)ptr3->Data;
								{
									SAstTypeFuncArg* item2 = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
									item2->Arg = arg->Type;
									item2->RefVar = arg->RefVar;
									ListAdd(type->Args, item2);
								}
								ptr3 = ptr3->Next;
							}
						}
						type->Ret = ((SAstFunc*)item->Def)->Ret;
						((SAstExpr*)ast)->Type = (SAstType*)type;
					}
					((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
					found = True;
				}
				if (found)
				{
					// 'me' and automatically generated arguments can be accessed even though they are private.
					if (!item->Public && (((SAst*)ast->Var)->RefName == NULL || wcscmp(((SAst*)ast->Var)->RefName, L"me") != 0))
					{
						Err(L"EA0051", ((SAst*)ast)->Pos, ast->Member);
						return NULL;
					}
					ast->ClassItem = item;
					return (SAstExpr*)ast;
				}
				ptr2 = ptr2->Next;
			}
			ptr = (SAstClass*)((SAst*)ptr)->RefItem;
		}
	}
	else
	{
		SAstType* var_type = ast->Var->Type;
		const Char* member = ast->Member;
		Bool correct = False;
		if (((SAst*)var_type)->TypeId == AstTypeId_TypeEnumElement)
		{
			ASSERT(((SAst*)ast->Var)->TypeId == AstTypeId_ExprValue);
			Err(L"EA0060", ((SAst*)ast)->Pos, *(const Char**)((SAstExprValue*)ast->Var)->Value);
			return NULL;
		}
		switch (GetBuildInFuncType(member))
		{
			case 0x0001:
				if (IsInt(var_type) || IsFloat(var_type) || IsChar(var_type) || IsBool(var_type) || ((SAst*)var_type)->TypeId == AstTypeId_TypeBit || IsStr(var_type))
					correct = True;
				break;
			case 0x0002:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeBit || IsEnum(var_type))
					correct = True;
				break;
			case 0x0003:
				if (IsChar(var_type))
					correct = True;
				break;
			case 0x0004:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeBit)
					correct = True;
				break;
			case 0x0005:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeArray)
					correct = True;
				break;
			case 0x0006:
				if (IsStr(var_type))
					correct = True;
				break;
			case 0x0007:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen)
				{
					correct = True;
					switch (((SAstTypeGen*)var_type)->Kind)
					{
						case AstTypeGenKind_List: member = L"addList"; break;
						case AstTypeGenKind_Stack: member = L"addStack"; break;
						case AstTypeGenKind_Queue: member = L"addQueue"; break;
					}
				}
				else if (((SAst*)var_type)->TypeId == AstTypeId_TypeDict)
				{
					correct = True;
					member = L"addDict";
				}
				break;
			case 0x0008:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen)
				{
					correct = True;
					switch (((SAstTypeGen*)var_type)->Kind)
					{
						case AstTypeGenKind_List: member = L"getList"; break;
						case AstTypeGenKind_Stack: member = L"getStack"; break;
						case AstTypeGenKind_Queue: member = L"getQueue"; break;
					}
				}
				else if (((SAst*)var_type)->TypeId == AstTypeId_TypeDict)
				{
					correct = True;
					member = L"getDict";
				}
				break;
			case 0x0009:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen && ((SAstTypeGen*)var_type)->Kind == AstTypeGenKind_List)
					correct = True;
				break;
			case 0x000a:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen && (((SAstTypeGen*)var_type)->Kind == AstTypeGenKind_Stack || ((SAstTypeGen*)var_type)->Kind == AstTypeGenKind_Queue))
					correct = True;
				break;
			case 0x000b:
				if (IsInt(var_type))
				{
					correct = True;
					if (wcscmp(member, L"abs") == 0)
						member = L"absInt";
					else if (wcscmp(member, L"clamp") == 0)
						member = L"clampInt";
					else if (wcscmp(member, L"clampMin") == 0)
						member = L"clampMinInt";
					else if (wcscmp(member, L"clampMax") == 0)
						member = L"clampMaxInt";
					else if (wcscmp(member, L"sign") == 0)
						member = L"signInt";
					else if (wcscmp(member, L"toStrFmt") == 0)
						member = L"toStrFmtInt";
					else
						ASSERT(False);
				}
				else if (IsFloat(var_type))
				{
					correct = True;
					if (wcscmp(member, L"abs") == 0)
						member = L"absFloat";
					else if (wcscmp(member, L"clamp") == 0)
						member = L"clampFloat";
					else if (wcscmp(member, L"clampMin") == 0)
						member = L"clampMinFloat";
					else if (wcscmp(member, L"clampMax") == 0)
						member = L"clampMaxFloat";
					else if (wcscmp(member, L"sign") == 0)
						member = L"signFloat";
					else if (wcscmp(member, L"toStrFmt") == 0)
						member = L"toStrFmtFloat";
					else
						ASSERT(False);
				}
				break;
			case 0x000c:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeArray && IsStr(((SAstTypeArray*)var_type)->ItemType))
					correct = True;
				break;
			case 0x000d:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeDict)
					correct = True;
				break;
			case 0x000e:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeArray)
				{
					correct = True;
					if (wcscmp(member, L"sort") == 0)
						member = L"sortArray";
					else if (wcscmp(member, L"sortDesc") == 0)
						member = L"sortDescArray";
					else if (wcscmp(member, L"find") == 0)
						member = L"findArray";
					else if (wcscmp(member, L"findLast") == 0)
						member = L"findLastArray";
					else
						ASSERT(False);
				}
				else if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen && ((SAstTypeGen*)var_type)->Kind == AstTypeGenKind_List)
				{
					correct = True;
					if (wcscmp(member, L"sort") == 0)
						member = L"sortList";
					else if (wcscmp(member, L"sortDesc") == 0)
						member = L"sortDescList";
					else if (wcscmp(member, L"find") == 0)
						member = L"findList";
					else if (wcscmp(member, L"findLast") == 0)
						member = L"findLastList";
					else
						ASSERT(False);
				}
				break;
			case 0x000f:
				if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen && ((SAstTypeGen*)var_type)->Kind == AstTypeGenKind_List)
					correct = True;
				else if (((SAst*)var_type)->TypeId == AstTypeId_TypeDict)
				{
					member = L"delDict";
					correct = True;
				}
				break;
		}
		if (correct)
		{
			SAstExpr* expr;
			Char name[32];
			wcscpy(name, L"_");
			wcscat(name, member);
			expr = (SAstExpr*)SearchStdItem(L"kuin", name, True);
			if (expr == NULL)
				return NULL;
			{
				SAstTypeFunc* func = (SAstTypeFunc*)expr->Type;
				if ((func->FuncAttr & FuncAttr_AnyType) != 0)
				{
					ASSERT(func->Args->Len >= 2);
#if defined(_DEBUG)
					{
						SAstType* arg = ((SAstTypeFuncArg*)func->Args->Top->Data)->Arg;
						ASSERT(((SAst*)arg)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)arg)->ItemType)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)((SAstTypeArray*)arg)->ItemType)->Size == 1);
					}
#endif
					ASSERT(IsInt(((SAstTypeFuncArg*)func->Args->Top->Next->Data)->Arg));
					((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Data)->Arg = var_type;
				}
				if ((func->FuncAttr & FuncAttr_TakeMe) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeChild) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeKeyValue) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeKeyValueFunc) == 0);
					ASSERT(func->Args->Len >= 3);
#if defined(_DEBUG)
					{
						SAstType* arg = ((SAstTypeFuncArg*)func->Args->Top->Next->Next->Data)->Arg;
						ASSERT(((SAst*)arg)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)arg)->ItemType)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)((SAstTypeArray*)arg)->ItemType)->Size == 1);
					}
#endif
					((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Data)->Arg = var_type;
				}
				if ((func->FuncAttr & FuncAttr_TakeChild) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeMe) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeKeyValue) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeKeyValueFunc) == 0);
					ASSERT(func->Args->Len >= 3);
#if defined(_DEBUG)
					{
						SAstType* arg = ((SAstTypeFuncArg*)func->Args->Top->Next->Next->Data)->Arg;
						ASSERT(((SAst*)arg)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)arg)->ItemType)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)((SAstTypeArray*)arg)->ItemType)->Size == 1);
					}
#endif
					if (((SAst*)var_type)->TypeId == AstTypeId_TypeArray)
						((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Data)->Arg = ((SAstTypeArray*)var_type)->ItemType;
					else if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen)
						((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Data)->Arg = ((SAstTypeGen*)var_type)->ItemType;
					else
					{
						ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeDict);
						((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Data)->Arg = ((SAstTypeDict*)var_type)->ItemTypeKey;
					}
				}
				if ((func->FuncAttr & FuncAttr_TakeKeyValue) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeMe) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeChild) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeKeyValueFunc) == 0);
					ASSERT(func->Args->Len >= 4);
#if defined(_DEBUG)
					{
						SAstType* arg = ((SAstTypeFuncArg*)func->Args->Top->Next->Next->Next->Data)->Arg;
						ASSERT(((SAst*)arg)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)arg)->ItemType)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)((SAstTypeArray*)arg)->ItemType)->Size == 1);
					}
					{
						SAstType* arg = ((SAstTypeFuncArg*)func->Args->Top->Next->Next->Next->Next->Data)->Arg;
						ASSERT(((SAst*)arg)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)arg)->ItemType)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)((SAstTypeArray*)arg)->ItemType)->Size == 1);
					}
#endif
					ASSERT(IsInt(((SAstTypeFuncArg*)func->Args->Top->Next->Next->Data)->Arg));
					ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeDict);
					((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Next->Data)->Arg = ((SAstTypeDict*)var_type)->ItemTypeKey;
					((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Next->Next->Data)->Arg = ((SAstTypeDict*)var_type)->ItemTypeValue;
				}
				if ((func->FuncAttr & FuncAttr_TakeKeyValueFunc) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeMe) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeChild) == 0);
					ASSERT((func->FuncAttr & FuncAttr_TakeKeyValue) == 0);
					ASSERT(func->Args->Len >= 3);
#if defined(_DEBUG)
					ASSERT(IsInt(((SAstTypeFuncArg*)func->Args->Top->Next->Next->Data)->Arg));
#endif
					ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeDict);
					{
						SAstTypeFunc* type = (SAstTypeFunc*)Alloc(sizeof(SAstTypeFunc));
						InitAst((SAst*)type, AstTypeId_TypeFunc, ((SAst*)ast)->Pos);
						type->FuncAttr = FuncAttr_None;
						type->Args = ListNew();
						{
							SAstTypeFuncArg* item = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
							item->Arg = ((SAstTypeDict*)var_type)->ItemTypeKey;
							item->RefVar = False;
							ListAdd(type->Args, item);
						}
						{
							SAstTypeFuncArg* item = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
							item->Arg = ((SAstTypeDict*)var_type)->ItemTypeValue;
							item->RefVar = False;
							ListAdd(type->Args, item);
						}
						{
							SAstTypeFuncArg* item = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
							item->Arg = ((SAstTypeFuncArg*)func->Args->Top->Next->Next->Next->Data)->Arg;
							item->RefVar = False;
							ListAdd(type->Args, item);
						}
						type->Ret = func->Ret;
						((SAstTypeFuncArg*)((SAstTypeFunc*)expr->Type)->Args->Top->Next->Next->Data)->Arg = (SAstType*)type;
					}
				}
				if ((func->FuncAttr & FuncAttr_RetMe) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT((func->FuncAttr & FuncAttr_RetChild) == 0);
					ASSERT(IsInt(func->Ret));
					((SAstTypeFunc*)expr->Type)->Ret = var_type;
				}
				if ((func->FuncAttr & FuncAttr_RetChild) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT((func->FuncAttr & FuncAttr_RetMe) == 0);
					ASSERT(IsInt(func->Ret));
					if (((SAst*)var_type)->TypeId == AstTypeId_TypeArray)
						((SAstTypeFunc*)expr->Type)->Ret = ((SAstTypeArray*)var_type)->ItemType;
					else if (((SAst*)var_type)->TypeId == AstTypeId_TypeGen)
						((SAstTypeFunc*)expr->Type)->Ret = ((SAstTypeGen*)var_type)->ItemType;
					else
					{
						ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeDict);
						((SAstTypeFunc*)expr->Type)->Ret = ((SAstTypeDict*)var_type)->ItemTypeValue;
					}
				}
				if ((func->FuncAttr & FuncAttr_RetArrayOfListChild) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT(IsInt(func->Ret));
					ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeGen && ((SAstTypeGen*)var_type)->Kind == AstTypeGenKind_List);
					{
						SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
						InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
						type->ItemType = ((SAstTypeGen*)var_type)->ItemType;
						((SAstTypeFunc*)expr->Type)->Ret = (SAstType*)type;
					}
				}
				if ((func->FuncAttr & FuncAttr_RetArrayOfDictKey) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT(IsInt(func->Ret));
					ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeDict);
					{
						SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
						InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
						type->ItemType = ((SAstTypeDict*)var_type)->ItemTypeKey;
						((SAstTypeFunc*)expr->Type)->Ret = (SAstType*)type;
					}
				}
				if ((func->FuncAttr & FuncAttr_RetArrayOfDictValue) != 0)
				{
					ASSERT((func->FuncAttr & FuncAttr_AnyType) != 0);
					ASSERT(IsInt(func->Ret));
					ASSERT(((SAst*)var_type)->TypeId == AstTypeId_TypeDict);
					{
						SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
						InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)ast)->Pos);
						type->ItemType = ((SAstTypeDict*)var_type)->ItemTypeValue;
						((SAstTypeFunc*)expr->Type)->Ret = (SAstType*)type;
					}
				}
			}
			((SAst*)ast)->RefItem = (SAst*)expr;
			((SAstExpr*)ast)->Type = expr->Type;
			((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
			return (SAstExpr*)ast;
		}
	}
	Err(L"EA0052", ((SAst*)ast)->Pos, ast->Member);
	return NULL;
}

static SAstExpr* RebuildExprValue(SAstExprValue* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprValueArray(SAstExprValueArray* ast)
{
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	ASSERT(((SAstExpr*)ast)->Type == NULL);
	{
		SListNode* ptr = ast->Values->Top;
		Bool null_set = False;
		Bool enum_set = False;
		while (ptr != NULL)
		{
			ptr->Data = RebuildExpr((SAstExpr*)ptr->Data, False);
			if (ptr->Data == NULL)
				return NULL;
			{
				SAstType* data_type;
				data_type = ((SAstExpr*)ptr->Data)->Type;
				if (((SAstExpr*)ast)->Type == NULL)
				{
					if (((SAst*)data_type)->TypeId == AstTypeId_TypeNull)
					{
						if (enum_set)
						{
							Err(L"EA0054", ((SAst*)ast)->Pos);
							return NULL;
						}
						null_set = True;
					}
					else if (((SAst*)data_type)->TypeId == AstTypeId_TypeEnumElement)
					{
						if (null_set)
						{
							Err(L"EA0053", ((SAst*)ast)->Pos);
							return NULL;
						}
						enum_set = True;
					}
					else
					{
						// Determine the type of the array initializer when a value other than 'null' is specified.
						if (null_set && !IsNullable(data_type))
						{
							Err(L"EA0053", ((SAst*)ast)->Pos);
							return NULL;
						}
						if (enum_set && !IsEnum(data_type))
						{
							Err(L"EA0054", ((SAst*)ast)->Pos);
							return NULL;
						}
						{
							SAstTypeArray* type = (SAstTypeArray*)Alloc(sizeof(SAstTypeArray));
							InitAst((SAst*)type, AstTypeId_TypeArray, ((SAst*)data_type)->Pos);
							type->ItemType = data_type;
							((SAstExpr*)ast)->Type = (SAstType*)type;
						}
					}
				}
				else if (!CmpType(((SAstTypeArray*)((SAstExpr*)ast)->Type)->ItemType, data_type))
				{
					// The types of the second and subsequent elements of the array initializer do not match the type of the first element.
					Err(L"EA0054", ((SAst*)ast)->Pos);
					return NULL;
				}
			}
			ptr = ptr->Next;
		}
		if (((SAstExpr*)ast)->Type == NULL)
		{
			if (enum_set)
			{
				Err(L"EA0061", ((SAst*)ast)->Pos);
				return NULL;
			}
			else
			{
				Err(L"EA0055", ((SAst*)ast)->Pos);
				return NULL;
			}
		}
	}
	if (IsEnum(((SAstTypeArray*)((SAstExpr*)ast)->Type)->ItemType))
	{
		SListNode* ptr = ast->Values->Top;
		while (ptr != NULL)
		{
			if (((SAst*)((SAstExpr*)ptr->Data)->Type)->TypeId == AstTypeId_TypeEnumElement)
				RebuildEnumElement((SAstExpr*)ptr->Data, ((SAstTypeArray*)((SAstExpr*)ast)->Type)->ItemType);
			ptr = ptr->Next;
		}
	}
	if (IsStr(((SAstExpr*)ast)->Type))
	{
		// Replace constants consisting only of characters with string literals.
		Bool is_const = True;
		{
			SListNode* ptr = ast->Values->Top;
			while (ptr != NULL)
			{
				if (((SAst*)ptr->Data)->TypeId != AstTypeId_ExprValue)
				{
					is_const = False;
					break;
				}
				ptr = ptr->Next;
			}
		}
		if (is_const)
		{
			SAstExprValue* ast2 = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
			InitAst((SAst*)ast2, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
			((SAstExpr*)ast2)->Type = ((SAstExpr*)ast)->Type;
			*(const Char**)ast2->Value = (Char*)Alloc(sizeof(Char) * (size_t)(ast->Values->Len + 1));
			{
				SListNode* ptr = ast->Values->Top;
				Char* p = *(Char**)ast2->Value;
				while (ptr != NULL)
				{
					*p = *(Char*)((SAstExprValue*)ptr->Data)->Value;
					ptr = ptr->Next;
					p++;
				}
				*p = L'\0';
			}
			ast2 = (SAstExprValue*)RebuildExprValue(ast2);
			((SAst*)ast)->AnalyzedCache = (SAst*)ast2;
			return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
		}
	}
	((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
	return (SAstExpr*)ast;
}

static SAstExpr* RebuildExprRef(SAstExpr* ast)
{
	ASSERT(((SAst*)ast)->TypeId == AstTypeId_ExprRef);
	if (((SAst*)ast)->AnalyzedCache != NULL)
		return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
	((SAst*)ast)->AnalyzedCache = (SAst*)ast;
	{
		SAst* ref_item = ((SAst*)ast)->RefItem;
		if (ref_item == NULL)
			return NULL;
		switch (ref_item->TypeId)
		{
			case AstTypeId_Func:
				{
					SAstFunc* func = (SAstFunc*)((SAst*)ast)->RefItem;
					RebuildFunc(func);
					{
						SAstTypeFunc* type = (SAstTypeFunc*)Alloc(sizeof(SAstTypeFunc));
						InitAst((SAst*)type, AstTypeId_TypeFunc, ((SAst*)ast)->Pos);
						type->FuncAttr = func->FuncAttr;
						type->Args = ListNew();
						{
							SListNode* ptr = func->Args->Top;
							while (ptr != NULL)
							{
								SAstTypeFuncArg* arg = (SAstTypeFuncArg*)Alloc(sizeof(SAstTypeFuncArg));
								arg->RefVar = ((SAstArg*)ptr->Data)->RefVar;
								arg->Arg = ((SAstArg*)ptr->Data)->Type;
								ListAdd(type->Args, arg);
								ptr = ptr->Next;
							}
						}
						type->Ret = func->Ret;
						((SAstExpr*)ast)->Type = (SAstType*)type;
					}
					((SAstExpr*)ast)->VarKind = AstExprVarKind_Value;
				}
				break;
			case AstTypeId_Arg:
				{
					SAstArg* arg = (SAstArg*)((SAst*)ast)->RefItem;
					RebuildArg(arg);
					switch (arg->Kind)
					{
						case AstArgKind_Global:
							ast->Type = arg->Type;
							((SAstExpr*)ast)->VarKind = AstExprVarKind_GlobalVar;
							break;
						case AstArgKind_LocalArg:
							ast->Type = arg->Type;
							((SAstExpr*)ast)->VarKind = arg->RefVar ? AstExprVarKind_RefVar : AstExprVarKind_LocalVar;
							break;
						case AstArgKind_LocalVar:
							ast->Type = arg->Type;
							((SAstExpr*)ast)->VarKind = AstExprVarKind_LocalVar;
							break;
						case AstArgKind_Const:
							if (arg->Expr == NULL)
								return NULL;
							{
								SAstExprValue* expr = (SAstExprValue*)Alloc(sizeof(SAstExprValue));
								InitAst((SAst*)expr, AstTypeId_ExprValue, ((SAst*)ast)->Pos);
								((SAstExpr*)expr)->Type = arg->Expr->Type;
								*(U64*)expr->Value = *(U64*)((SAstExprValue*)arg->Expr)->Value;
								expr = (SAstExprValue*)RebuildExprValue(expr);
								((SAst*)ast)->AnalyzedCache = (SAst*)expr;
							}
							return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
						case AstArgKind_Member:
							{
								Err(L"EA0057", ((SAst*)ast)->Pos, ((SAst*)ast)->RefName);
								return NULL;
						}
							break;
						default:
							ASSERT(False);
					}
				}
				break;
			case AstTypeId_StatSwitch:
			case AstTypeId_StatFor:
			case AstTypeId_StatTry:
				ASSERT(ref_item->AnalyzedCache != NULL);
				((SAst*)ast)->RefItem = (SAst*)((SAstStatBreakable*)ref_item)->BlockVar;
				((SAstExpr*)ast)->Type = ((SAstArg*)((SAstStatBreakable*)ref_item)->BlockVar)->Type;
				((SAstExpr*)ast)->VarKind = AstExprVarKind_LocalVar;
				break;
			default:
				if ((ref_item->TypeId & AstTypeId_Expr) != 0 && ref_item->AnalyzedCache != NULL && IsEnum(((SAstExpr*)ref_item->AnalyzedCache)->Type))
				{
					((SAst*)ast)->AnalyzedCache = ref_item->AnalyzedCache;
					return (SAstExpr*)((SAst*)ast)->AnalyzedCache;
				}
				else
				{
					Err(L"EA0036", ((SAst*)ast)->Pos, ((SAst*)ast)->RefName);
					return NULL;
				}
				break;
		}
	}
	return (SAstExpr*)ast;
}
