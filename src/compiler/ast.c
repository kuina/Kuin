#include "ast.h"

#include "util.h"

static FILE* FilePtr;
static int Tab;

static void PrintTab(void);
static void PrintAst(const Char* tag, const SAst* ast);
static void PrintAstList(const Char* tag, const SList* list);
static void DumpRecursion(const SAst* ast);
static void DumpAst(const SAst* ast);
static void DumpAstRoot(const SAstRoot* ast);
static void DumpAstFunc(const SAstFunc* ast);
static void DumpAstFuncRaw(const SAstFuncRaw* ast);
static void DumpAstVar(const SAstVar* ast);
static void DumpAstConst(const SAstConst* ast);
static void DumpAstAlias(const SAstAlias* ast);
static void DumpAstClass(const SAstClass* ast);
static void DumpAstEnum(const SAstEnum* ast);
static void DumpAstArg(const SAstArg* ast);
static void DumpAstStat(const SAstStat* ast);
static void DumpAstStatBreakable(const SAstStatBreakable* ast);
static void DumpAstStatSkipable(const SAstStatSkipable* ast);
static void DumpAstStatFunc(const SAstStatFunc* ast);
static void DumpAstStatVar(const SAstStatVar* ast);
static void DumpAstStatConst(const SAstStatConst* ast);
static void DumpAstStatAlias(const SAstStatAlias* ast);
static void DumpAstStatClass(const SAstStatClass* ast);
static void DumpAstStatEnum(const SAstStatEnum* ast);
static void DumpAstStatIf(const SAstStatIf* ast);
static void DumpAstStatElIf(const SAstStatElIf* ast);
static void DumpAstStatSwitch(const SAstStatSwitch* ast);
static void DumpAstStatCase(const SAstStatCase* ast);
static void DumpAstStatWhile(const SAstStatWhile* ast);
static void DumpAstStatFor(const SAstStatFor* ast);
static void DumpAstStatTry(const SAstStatTry* ast);
static void DumpAstStatCatch(const SAstStatCatch* ast);
static void DumpAstStatThrow(const SAstStatThrow* ast);
static void DumpAstStatBlock(const SAstStatBlock* ast);
static void DumpAstStatRet(const SAstStatRet* ast);
static void DumpAstStatDo(const SAstStatDo* ast);
static void DumpAstStatBreak(const SAstStat* ast);
static void DumpAstStatSkip(const SAstStat* ast);
static void DumpAstStatAssert(const SAstStatAssert* ast);
static void DumpAstType(const SAstType* ast);
static void DumpAstTypeNullable(const SAstTypeNullable* ast);
static void DumpAstTypeArray(const SAstTypeArray* ast);
static void DumpAstTypeBit(const SAstTypeBit* ast);
static void DumpAstTypeFunc(const SAstTypeFunc* ast);
static void DumpAstTypeGen(const SAstTypeGen* ast);
static void DumpAstTypeDict(const SAstTypeDict* ast);
static void DumpAstTypePrim(const SAstTypePrim* ast);
static void DumpAstTypeUser(const SAstTypeUser* ast);
static void DumpAstTypeNull(const SAstTypeNull* ast);
static void DumpAstTypeEnumElement(const SAstTypeEnumElement* ast);
static void DumpAstExpr(const SAstExpr* ast);
static void DumpAstExpr1(const SAstExpr1* ast);
static void DumpAstExpr2(const SAstExpr2* ast);
static void DumpAstExpr3(const SAstExpr3* ast);
static void DumpAstExprNew(const SAstExprNew* ast);
static void DumpAstExprNewArray(const SAstExprNewArray* ast);
static void DumpAstExprAs(const SAstExprAs* ast);
static void DumpAstExprToBin(const SAstExprToBin* ast);
static void DumpAstExprFromBin(const SAstExprFromBin* ast);
static void DumpAstExprCall(const SAstExprCall* ast);
static void DumpAstExprArray(const SAstExprArray* ast);
static void DumpAstExprDot(const SAstExprDot* ast);
static void DumpAstExprValue(const SAstExprValue* ast);
static void DumpAstExprValueArray(const SAstExprValueArray* ast);
static void DumpAstExprRef(const SAstExpr* ast);
static void DumpScope(const Char* name, const SAst* ast);
static const void* DumpScopeChildren(const Char* key, const void* value, void* param);
static const Char* GetDefinitionFunc(int* len, const SAstFunc* ast);
static const Char* GetDefinitionAlias(int* len, const SAstAlias* ast);
static const Char* GetDefinitionClass(int* len, const SAstClass* ast);
static const Char* GetDefinitionEnum(int* len, const SAstEnum* ast);
static const Char* GetDefinitionArg(int* len, const SAstArg* ast);
static const Char* GetDefinitionStatIf(int* len, const SAstStatIf* ast);
static const Char* GetDefinitionStatSwitch(int* len, const SAstStatSwitch* ast);
static const Char* GetDefinitionStatWhile(int* len, const SAstStatWhile* ast);
static const Char* GetDefinitionStatFor(int* len, const SAstStatFor* ast);
static const Char* GetDefinitionStatTry(int* len, const SAstStatTry* ast);
static const Char* GetDefinitionStatBlock(int* len, const SAstStatBlock* ast);
static const Char* GetDefinitionTypeArray(int* len, const SAstTypeArray* ast);
static const Char* GetDefinitionTypeBit(int* len, const SAstTypeBit* ast);
static const Char* GetDefinitionTypeFunc(int* len, const SAstTypeFunc* ast);
static const Char* GetDefinitionTypeGen(int* len, const SAstTypeGen* ast);
static const Char* GetDefinitionTypeDict(int* len, const SAstTypeDict* ast);
static const Char* GetDefinitionTypePrim(int* len, const SAstTypePrim* ast);
static const Char* GetDefinitionTypeUser(int* len, const SAstTypeUser* ast);
static const Char* GetDefinitionExprValue(int* len, const SAstExprValue* ast);

Bool IsInt(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypePrim && ((SAstTypePrim*)type)->Kind == AstTypePrimKind_Int;
}

Bool IsFloat(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypePrim && ((SAstTypePrim*)type)->Kind == AstTypePrimKind_Float;
}

Bool IsChar(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypePrim && ((SAstTypePrim*)type)->Kind == AstTypePrimKind_Char;
}

Bool IsBool(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypePrim && ((SAstTypePrim*)type)->Kind == AstTypePrimKind_Bool;
}

Bool IsRef(const SAstType* type)
{
	EAstTypeId type_id = ((SAst*)type)->TypeId;
	return type_id == AstTypeId_TypeArray || type_id == AstTypeId_TypeGen || type_id == AstTypeId_TypeDict || IsClass(type);
}

Bool IsNullable(const SAstType* type)
{
	EAstTypeId type_id = ((SAst*)type)->TypeId;
	return (type_id & AstTypeId_TypeNullable) != 0 && !(((SAst*)type)->TypeId == AstTypeId_TypeUser && ((SAst*)type)->RefItem->TypeId == AstTypeId_Enum);
}

Bool IsClass(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypeUser && ((SAst*)type)->RefItem->TypeId == AstTypeId_Class;
}

Bool IsEnum(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypeUser && ((SAst*)type)->RefItem->TypeId == AstTypeId_Enum;
}

Bool IsStr(const SAstType* type)
{
	return ((SAst*)type)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)type)->ItemType)->TypeId == AstTypeId_TypePrim && ((SAstTypePrim*)((SAstTypeArray*)type)->ItemType)->Kind == AstTypePrimKind_Char;
}

void Dump1(const Char* path, const SAst* ast)
{
	Tab = 0;
	FilePtr = _wfopen(path, L"w, ccs=UTF-8");
	rewind(FilePtr);
	fwprintf(FilePtr, L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fwprintf(FilePtr, L"<Dump1>\n");
	Tab++;
	{
		DumpRecursion(ast);
		ASSERT(Tab == 1);
		DumpScope(L"Scope", ast);
	}
	Tab--;
	fwprintf(FilePtr, L"</Dump1>\n");
	fclose(FilePtr);
}

void GetTypeName(Char* buf, size_t* len, size_t size, const SAstType* ast)
{
	if (ast == NULL)
		return;
	switch (((SAst*)ast)->TypeId)
	{
		case AstTypeId_TypeArray:
			if (*len < size)
				*len += swprintf(buf + *len, size - *len, L"[]");
			GetTypeName(buf, len, size, ((const SAstTypeArray*)ast)->ItemType);
			break;
		case AstTypeId_TypeBit:
			if (*len < size)
				*len += swprintf(buf + *len, size - *len, L"bit%d", ((const SAstTypeBit*)ast)->Size * 8);
			break;
		case AstTypeId_TypeFunc:
			{
				const SAstTypeFunc* ast2 = (const SAstTypeFunc*)ast;
				if (*len < size)
					*len += swprintf(buf + *len, size - *len, L"func<(");
				SListNode* ptr = ast2->Args->Top;
				while (ptr != NULL)
				{
					const SAstTypeFuncArg* arg = (const SAstTypeFuncArg*)ptr->Data;
					if (ptr != ast2->Args->Top && *len < size)
						*len += swprintf(buf + *len, size - *len, L", ");
					if (arg->RefVar && *len < size)
						*len += swprintf(buf + *len, size - *len, L"&");
					GetTypeName(buf, len, size, arg->Arg);
					ptr = ptr->Next;
				}
				if (ast2->Ret == NULL)
				{
					if (*len < size)
						*len += swprintf(buf + *len, size - *len, L")>");
				}
				else
				{
					if (*len < size)
						*len += swprintf(buf + *len, size - *len, L"): ");
					GetTypeName(buf, len, size, ast2->Ret);
					if (*len < size)
						*len += swprintf(buf + *len, size - *len, L">");
				}
			}
			break;
		case AstTypeId_TypeGen:
			{
				const SAstTypeGen* ast2 = (const SAstTypeGen*)ast;
				if (*len < size)
				{
					switch (ast2->Kind)
					{
						case AstTypeGenKind_List:
							*len += swprintf(buf + *len, size - *len, L"list<");
							break;
						case AstTypeGenKind_Stack:
							*len += swprintf(buf + *len, size - *len, L"stack<");
							break;
						case AstTypeGenKind_Queue:
							*len += swprintf(buf + *len, size - *len, L"queue<");
							break;
						default:
							ASSERT(False);
							break;
					}
				}
				GetTypeName(buf, len, size, ast2->ItemType);
				if (*len < size)
					*len += swprintf(buf + *len, size - *len, L">");
			}
			break;
		case AstTypeId_TypeDict:
			{
				const SAstTypeDict* ast2 = (const SAstTypeDict*)ast;
				if (*len < size)
					*len += swprintf(buf + *len, size - *len, L"dict<");
				GetTypeName(buf, len, size, ast2->ItemTypeKey);
				if (*len < size)
					*len += swprintf(buf + *len, size - *len, L", ");
				GetTypeName(buf, len, size, ast2->ItemTypeValue);
				if (*len < size)
					*len += swprintf(buf + *len, size - *len, L">");
			}
			break;
		case AstTypeId_TypePrim:
			{
				const SAstTypePrim* ast2 = (const SAstTypePrim*)ast;
				if (*len < size)
				{
					switch (ast2->Kind)
					{
						case AstTypePrimKind_Int:
							*len += swprintf(buf + *len, size - *len, L"int");
							break;
						case AstTypePrimKind_Float:
							*len += swprintf(buf + *len, size - *len, L"float");
							break;
						case AstTypePrimKind_Char:
							*len += swprintf(buf + *len, size - *len, L"char");
							break;
						case AstTypePrimKind_Bool:
							*len += swprintf(buf + *len, size - *len, L"bool");
							break;
						default:
							ASSERT(False);
							break;
					}
				}
			}
			break;
		case AstTypeId_TypeUser:
			if (*len < size)
				*len += swprintf(buf + *len, size - *len, L"%s%s", ((const SAst*)ast)->RefName[0] == L'@' ? ((const SAst*)ast)->Pos->SrcName : L"", ((const SAst*)ast)->RefName);
			break;
		case AstTypeId_TypeEnumElement:
			if (*len < size)
				*len += swprintf(buf + *len, size - *len, L"(Unknown type)");
			break;
	}
}

static void PrintAst(const Char* tag, const SAst* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<%s>\n", tag);
	Tab++;
	{
		DumpRecursion(ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</%s>\n", tag);
}

static void PrintTab(void)
{
	int i;
	for (i = 0; i < Tab; i++)
		fwprintf(FilePtr, L"  ");
}

static void PrintAstList(const Char* tag, const SList* list)
{
	PrintTab(); fwprintf(FilePtr, L"<%s>\n", tag);
	Tab++;
	{
		SListNode* ptr = list->Top;
		while (ptr != NULL)
		{
			DumpRecursion((const SAst*)ptr->Data);
			ptr = ptr->Next;
		}
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</%s>\n", tag);
}

static void DumpRecursion(const SAst* ast)
{
	if (ast == NULL)
		return;
	switch (ast->TypeId)
	{
		case AstTypeId_Ast: DumpAst(ast); break;
		case AstTypeId_Root: DumpAstRoot((const SAstRoot*)ast); break;
		case AstTypeId_Func: DumpAstFunc((const SAstFunc*)ast); break;
		case AstTypeId_FuncRaw: DumpAstFuncRaw((const SAstFuncRaw*)ast); break;
		case AstTypeId_Var: DumpAstVar((const SAstVar*)ast); break;
		case AstTypeId_Const: DumpAstConst((const SAstConst*)ast); break;
		case AstTypeId_Alias: DumpAstAlias((const SAstAlias*)ast); break;
		case AstTypeId_Class: DumpAstClass((const SAstClass*)ast); break;
		case AstTypeId_Enum: DumpAstEnum((const SAstEnum*)ast); break;
		case AstTypeId_Arg: DumpAstArg((const SAstArg*)ast); break;
		case AstTypeId_Stat: ASSERT(False); break;
		case AstTypeId_StatBreakable: DumpAstStatBreakable((const SAstStatBreakable*)ast); break;
		case AstTypeId_StatSkipable: DumpAstStatSkipable((const SAstStatSkipable*)ast); break;
		case AstTypeId_StatEnd: ASSERT(False); break;
		case AstTypeId_StatFunc: DumpAstStatFunc((const SAstStatFunc*)ast); break;
		case AstTypeId_StatVar: DumpAstStatVar((const SAstStatVar*)ast); break;
		case AstTypeId_StatConst: DumpAstStatConst((const SAstStatConst*)ast); break;
		case AstTypeId_StatAlias: DumpAstStatAlias((const SAstStatAlias*)ast); break;
		case AstTypeId_StatClass: DumpAstStatClass((const SAstStatClass*)ast); break;
		case AstTypeId_StatEnum: DumpAstStatEnum((const SAstStatEnum*)ast); break;
		case AstTypeId_StatIf: DumpAstStatIf((const SAstStatIf*)ast); break;
		case AstTypeId_StatElIf: DumpAstStatElIf((const SAstStatElIf*)ast); break;
		case AstTypeId_StatElse: ASSERT(False); break;
		case AstTypeId_StatSwitch: DumpAstStatSwitch((const SAstStatSwitch*)ast); break;
		case AstTypeId_StatCase: DumpAstStatCase((const SAstStatCase*)ast); break;
		case AstTypeId_StatDefault: ASSERT(False); break;
		case AstTypeId_StatWhile: DumpAstStatWhile((const SAstStatWhile*)ast); break;
		case AstTypeId_StatFor: DumpAstStatFor((const SAstStatFor*)ast); break;
		case AstTypeId_StatTry: DumpAstStatTry((const SAstStatTry*)ast); break;
		case AstTypeId_StatCatch: DumpAstStatCatch((const SAstStatCatch*)ast); break;
		case AstTypeId_StatFinally: ASSERT(False); break;
		case AstTypeId_StatThrow: DumpAstStatThrow((const SAstStatThrow*)ast); break;
		case AstTypeId_StatBlock: DumpAstStatBlock((const SAstStatBlock*)ast); break;
		case AstTypeId_StatRet: DumpAstStatRet((const SAstStatRet*)ast); break;
		case AstTypeId_StatDo: DumpAstStatDo((const SAstStatDo*)ast); break;
		case AstTypeId_StatBreak: DumpAstStatBreak((const SAstStat*)ast); break;
		case AstTypeId_StatSkip: DumpAstStatSkip((const SAstStat*)ast); break;
		case AstTypeId_StatAssert: DumpAstStatAssert((const SAstStatAssert*)ast); break;
		case AstTypeId_Type: ASSERT(False); break;
		case AstTypeId_TypeNullable: ASSERT(False); break;
		case AstTypeId_TypeArray: DumpAstTypeArray((const SAstTypeArray*)ast); break;
		case AstTypeId_TypeBit: DumpAstTypeBit((const SAstTypeBit*)ast); break;
		case AstTypeId_TypeFunc: DumpAstTypeFunc((const SAstTypeFunc*)ast); break;
		case AstTypeId_TypeGen: DumpAstTypeGen((const SAstTypeGen*)ast); break;
		case AstTypeId_TypeDict: DumpAstTypeDict((const SAstTypeDict*)ast); break;
		case AstTypeId_TypePrim: DumpAstTypePrim((const SAstTypePrim*)ast); break;
		case AstTypeId_TypeUser: DumpAstTypeUser((const SAstTypeUser*)ast); break;
		case AstTypeId_TypeNull: DumpAstTypeNull((const SAstTypeNull*)ast); break;
		case AstTypeId_TypeEnumElement: DumpAstTypeEnumElement((const SAstTypeEnumElement*)ast); break;
		case AstTypeId_Expr: ASSERT(False); break;
		case AstTypeId_Expr1: DumpAstExpr1((const SAstExpr1*)ast); break;
		case AstTypeId_Expr2: DumpAstExpr2((const SAstExpr2*)ast); break;
		case AstTypeId_Expr3: DumpAstExpr3((const SAstExpr3*)ast); break;
		case AstTypeId_ExprNew: DumpAstExprNew((const SAstExprNew*)ast); break;
		case AstTypeId_ExprNewArray: DumpAstExprNewArray((const SAstExprNewArray*)ast); break;
		case AstTypeId_ExprAs: DumpAstExprAs((const SAstExprAs*)ast); break;
		case AstTypeId_ExprToBin: DumpAstExprToBin((const SAstExprToBin*)ast); break;
		case AstTypeId_ExprFromBin: DumpAstExprFromBin((const SAstExprFromBin*)ast); break;
		case AstTypeId_ExprCall: DumpAstExprCall((const SAstExprCall*)ast); break;
		case AstTypeId_ExprArray: DumpAstExprArray((const SAstExprArray*)ast); break;
		case AstTypeId_ExprDot: DumpAstExprDot((const SAstExprDot*)ast); break;
		case AstTypeId_ExprValue: DumpAstExprValue((const SAstExprValue*)ast); break;
		case AstTypeId_ExprValueArray: DumpAstExprValueArray((const SAstExprValueArray*)ast); break;
		case AstTypeId_ExprRef: DumpAstExprRef((const SAstExpr*)ast); break;
		default:
			ASSERT(False);
			break;
	}
}

static void DumpAst(const SAst* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Ast");
	fwprintf(FilePtr, L" Pos=\"%s: %d, %d\"", ast->Pos->SrcName, ast->Pos->Row, ast->Pos->Col);
	fwprintf(FilePtr, L" Name=\"%s\"", ast->Name == NULL ? L"(Null)" : ast->Name);
	fwprintf(FilePtr, L" ScopeParent=\"%s\"", ast->ScopeParent == NULL ? L"(Null)" : L"(Ptr)");
	fwprintf(FilePtr, L" ScopeChildren=\"%s\"", ast->ScopeChildren == NULL ? L"(Null)" : L"(Ptr)");
	fwprintf(FilePtr, L" ScopeRefedItems=\"%s\"", ast->ScopeRefedItems == NULL ? L"(Null)" : L"(Ptr)");
	fwprintf(FilePtr, L" RefName=\"%s\"", ast->RefName == NULL ? L"(Null)" : ast->RefName);
	fwprintf(FilePtr, L" RefItem=\"%s\"", ast->RefItem == NULL ? L"(Null)" : L"(Ptr)");
	fwprintf(FilePtr, L" AnalyzedCache=\"%s\"", ast->AnalyzedCache == NULL ? L"(Null)" : L"(Ptr)");
	fwprintf(FilePtr, L" Public=\"%s\"", ast->Public ? L"True" : L"False");
	fwprintf(FilePtr, L" />\n");
}

static void DumpAstRoot(const SAstRoot* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Root>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		PrintAstList(L"Root_Items", ast->Items);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Root>\n");
}

static void DumpAstFunc(const SAstFunc* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Func");
	fwprintf(FilePtr, L" DllName=\"%s\"", ast->DllName == NULL ? L"(Null)" : ast->DllName);
	fwprintf(FilePtr, L" DllFuncName=\"%s\"", ast->DllFuncName == NULL ? L"(Null)" : ast->DllFuncName);
	fwprintf(FilePtr, L" FuncAttr=\"0x%016I64X\"", (U64)ast->FuncAttr);
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		ASSERT(ast->AddrTop != NULL && *ast->AddrTop == -1);
		ASSERT(ast->RetPoint == NULL);
		PrintAstList(L"Func_Args", ast->Args);
		if (ast->Ret != NULL)
			PrintAst(L"Func_Ret", (const SAst*)ast->Ret);
		PrintAstList(L"Func_Stats", ast->Stats);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Func>\n");
}

static void DumpAstFuncRaw(const SAstFuncRaw* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<FuncRaw>\n");
	Tab++;
	{
		DumpAstFunc((const SAstFunc*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</FuncRaw>\n");
}

static void DumpAstVar(const SAstVar* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Var>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		PrintAst(L"Var_Var", (const SAst*)ast->Var);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Var>\n");
}

static void DumpAstConst(const SAstConst* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Const>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		PrintAst(L"Const_Var", (const SAst*)ast->Var);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Const>\n");
}

static void DumpAstAlias(const SAstAlias* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Alias>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		PrintAst(L"Alias_Type", (const SAst*)ast->Type);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Alias>\n");
}

static void DumpAstClass(const SAstClass* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Class");
	ASSERT(ast->Addr != NULL && *ast->Addr == -1);
	fwprintf(FilePtr, L" VarSize=\"%d\"", ast->VarSize);
	fwprintf(FilePtr, L" FuncSize=\"%d\"", ast->FuncSize);
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		PrintTab(); fwprintf(FilePtr, L"<Class_Items>\n");
		Tab++;
		{
			SListNode* ptr = ast->Items->Top;
			while (ptr != NULL)
			{
				const SAstClassItem* item = (const SAstClassItem*)ptr->Data;
				PrintTab(); fwprintf(FilePtr, L"<ClassItem");
				fwprintf(FilePtr, L" Public=\"%s\"", item->Public ? L"True" : L"False");
				fwprintf(FilePtr, L" Override=\"%s\"", item->Override ? L"True" : L"False");
				fwprintf(FilePtr, L" ParentItem=\"%s\"", item->ParentItem == NULL ? L"(Null)" : L"(Ptr)");
				fwprintf(FilePtr, L" />\n");
				ASSERT(item->Def != NULL);
				ASSERT(item->Addr == -1);
				ptr = ptr->Next;
			}
		}
		Tab--;
		PrintTab(); fwprintf(FilePtr, L"</Class_Items>\n");
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Class>\n");
}

static void DumpAstEnum(const SAstEnum* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Enum>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		PrintAstList(L"Enum_Items", ast->Items);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Enum>\n");
}

static void DumpAstArg(const SAstArg* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Arg");
	fwprintf(FilePtr, L" Kind=\"");
	switch (ast->Kind)
	{
		case AstArgKind_Unknown: fwprintf(FilePtr, L"Unknown"); break;
		case AstArgKind_Global: fwprintf(FilePtr, L"Global"); break;
		case AstArgKind_LocalArg: fwprintf(FilePtr, L"LocalArg"); break;
		case AstArgKind_LocalVar: fwprintf(FilePtr, L"LocalVar"); break;
		case AstArgKind_Const: fwprintf(FilePtr, L"Const"); break;
		case AstArgKind_Member: fwprintf(FilePtr, L"Member"); break;
		default:
			ASSERT(False);
			break;
	}
	fwprintf(FilePtr, L"\"");
	fwprintf(FilePtr, L" RefVar=\"%s\"", ast->RefVar ? L"True" : L"False");
	fwprintf(FilePtr, L">\n");
	ASSERT(ast->Addr != NULL && *ast->Addr == -1);
	Tab++;
	{
		DumpAst((const SAst*)ast);
		if (ast->Type != NULL)
			PrintAst(L"Arg_Type", (const SAst*)ast->Type);
		if (ast->Expr != NULL)
			PrintAst(L"Arg_Expr", (const SAst*)ast->Expr);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Arg>\n");
}

static void DumpAstStat(const SAstStat* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Stat>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Stat>\n");
}

static void DumpAstStatBreakable(const SAstStatBreakable* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatBreakable>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		if (ast->BlockVar != NULL)
			PrintAst(L"StatBreakable_BlockVar", (const SAst*)ast->BlockVar);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatBreakable>\n");
}

static void DumpAstStatSkipable(const SAstStatSkipable* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatSkipable>\n");
	ASSERT(ast->SkipPoint != NULL);
	Tab++;
	{
		DumpAstStatBreakable((const SAstStatBreakable*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatSkipable>\n");
}

static void DumpAstStatFunc(const SAstStatFunc* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatFunc>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatFunc_Def", (const SAst*)ast->Def);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatFunc>\n");
}

static void DumpAstStatVar(const SAstStatVar* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatVar>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatVar_Def", (const SAst*)ast->Def);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatVar>\n");
}

static void DumpAstStatConst(const SAstStatConst* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatConst>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatConst_Def", (const SAst*)ast->Def);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatConst>\n");
}

static void DumpAstStatAlias(const SAstStatAlias* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatAlias>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatAlias_Def", (const SAst*)ast->Def);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatAlias>\n");
}

static void DumpAstStatClass(const SAstStatClass* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatClass>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatClass_Def", (const SAst*)ast->Def);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatClass>\n");
}

static void DumpAstStatEnum(const SAstStatEnum* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatEnum>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatEnum_Def", (const SAst*)ast->Def);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatEnum>\n");
}

static void DumpAstStatIf(const SAstStatIf* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatIf>\n");
	Tab++;
	{
		DumpAstStatBreakable((const SAstStatBreakable*)ast);
		if (ast->Cond != NULL)
			PrintAst(L"StatIf_Cond", (const SAst*)ast->Cond);
		PrintAst(L"StatIf_StatBlock", (const SAst*)ast->StatBlock);
		PrintAstList(L"StatIf_ElIfs", ast->ElIfs);
		if (ast->ElseStatBlock != NULL)
			PrintAst(L"StatIf_ElseStatBlock", (const SAst*)ast->ElseStatBlock);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatIf>\n");
}

static void DumpAstStatElIf(const SAstStatElIf* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatElIf>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatElIf_Cond", (const SAst*)ast->Cond);
		PrintAst(L"StatElIf_StatBlock", (const SAst*)ast->StatBlock);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatElIf>\n");
}

static void DumpAstStatSwitch(const SAstStatSwitch* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatSwitch>\n");
	Tab++;
	{
		DumpAstStatBreakable((const SAstStatBreakable*)ast);
		PrintAst(L"StatSwitch_Cond", (const SAst*)ast->Cond);
		PrintAstList(L"StatSwitch_Cases", ast->Cases);
		if (ast->DefaultStatBlock != NULL)
			PrintAst(L"StatSwitch_DefaultStatBlock", (const SAst*)ast->DefaultStatBlock);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatSwitch>\n");
}

static void DumpAstStatCase(const SAstStatCase* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatCase>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintTab(); fwprintf(FilePtr, L"<StatCase_Conds>\n");
		Tab++;
		{
			SListNode* ptr = ast->Conds->Top;
			while (ptr != NULL)
			{
				SAstExpr** exprs = (SAstExpr**)ptr->Data;
				PrintTab(); fwprintf(FilePtr, L"<StatCase_1>\n");
				Tab++;
				{
					DumpRecursion((const SAst*)exprs[0]);
				}
				Tab--;
				PrintTab(); fwprintf(FilePtr, L"</StatCase_1>\n");
				if (exprs[1] != NULL)
				{
					PrintTab(); fwprintf(FilePtr, L"<StatCase_2>\n");
					Tab++;
					{
						DumpRecursion((const SAst*)exprs[1]);
					}
					Tab--;
					PrintTab(); fwprintf(FilePtr, L"</StatCase_2>\n");
				}
				ptr = ptr->Next;
			}
		}
		Tab--;
		PrintTab(); fwprintf(FilePtr, L"</StatCase_Conds>\n");
		PrintAst(L"StatCase_StatBlock", (const SAst*)ast->StatBlock);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatCase>\n");
}

static void DumpAstStatWhile(const SAstStatWhile* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatWhile");
	fwprintf(FilePtr, L" Skip=\"%s\"", ast->Skip ? L"True" : L"False");
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstStatSkipable((const SAstStatSkipable*)ast);
		PrintAst(L"StatWhile_Cond", (const SAst*)ast->Cond);
		PrintAstList(L"StatWhile_Stats", ast->Stats);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatWhile>\n");
}

static void DumpAstStatFor(const SAstStatFor* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatFor>\n");
	Tab++;
	{
		DumpAstStatSkipable((const SAstStatSkipable*)ast);
		PrintAst(L"StatFor_Start", (const SAst*)ast->Start);
		PrintAst(L"StatFor_Cond", (const SAst*)ast->Cond);
		PrintAst(L"StatFor_Step", (const SAst*)ast->Step);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatFor>\n");
}

static void DumpAstStatTry(const SAstStatTry* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatTry>\n");
	Tab++;
	{
		DumpAstStatBreakable((const SAstStatBreakable*)ast);
		PrintAst(L"StatTry_StatBlock", (const SAst*)ast->StatBlock);
		PrintAstList(L"StatTry_Catches", ast->Catches);
		if (ast->FinallyStatBlock != NULL)
			PrintAst(L"StatTry_FinallyStatBlock", (const SAst*)ast->FinallyStatBlock);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatTry>\n");
}

static void DumpAstStatCatch(const SAstStatCatch* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatCatch>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintTab(); fwprintf(FilePtr, L"<StatCatch_Conds>\n");
		Tab++;
		{
			SListNode* ptr = ast->Conds->Top;
			while (ptr != NULL)
			{
				SAstExpr** exprs = (SAstExpr**)ptr->Data;
				PrintTab(); fwprintf(FilePtr, L"<StatCatch_1>\n");
				Tab++;
				{
					DumpRecursion((const SAst*)exprs[0]);
				}
				Tab--;
				PrintTab(); fwprintf(FilePtr, L"</StatCatch_1>\n");
				PrintTab(); fwprintf(FilePtr, L"<StatCatch_2>\n");
				if (exprs[1] != NULL)
				{
					Tab++;
					{
						DumpRecursion((const SAst*)exprs[1]);
					}
					Tab--;
				}
				PrintTab(); fwprintf(FilePtr, L"</StatCatch_2>\n");
				ptr = ptr->Next;
			}
		}
		Tab--;
		PrintTab(); fwprintf(FilePtr, L"</StatCatch_Conds>\n");
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatCatch>\n");
}

static void DumpAstStatThrow(const SAstStatThrow* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatThrow>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatThrow_Code", (const SAst*)ast->Code);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatThrow>\n");
}

static void DumpAstStatBlock(const SAstStatBlock* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatBlock>\n");
	Tab++;
	{
		DumpAstStatBreakable((const SAstStatBreakable*)ast);
		PrintAstList(L"StatBlock_Stats", ast->Stats);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatBlock>\n");
}

static void DumpAstStatRet(const SAstStatRet* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatRet>\n");
	if (ast->Value != NULL)
	{
		Tab++;
		{
			DumpAstStat((const SAstStat*)ast);
			PrintAst(L"StatRet_Value", (const SAst*)ast->Value);
		}
		Tab--;
	}
	PrintTab(); fwprintf(FilePtr, L"</StatRet>\n");
}

static void DumpAstStatDo(const SAstStatDo* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatDo>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatDo_Expr", (const SAst*)ast->Expr);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatDo>\n");
}

static void DumpAstStatBreak(const SAstStat* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatBreak>\n");
	Tab++;
	{
		DumpAstStat(ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatBreak>\n");
}

static void DumpAstStatSkip(const SAstStat* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatSkip>\n");
	Tab++;
	{
		DumpAstStat(ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatSkip>\n");
}

static void DumpAstStatAssert(const SAstStatAssert* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<StatAssert>\n");
	Tab++;
	{
		DumpAstStat((const SAstStat*)ast);
		PrintAst(L"StatAssert_Cond", (const SAst*)ast->Cond);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</StatAssert>\n");
}

static void DumpAstType(const SAstType* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Type>\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Type>\n");
}

static void DumpAstTypeNullable(const SAstTypeNullable* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeNullable>\n");
	Tab++;
	{
		DumpAstType((const SAstType*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeNullable>\n");
}

static void DumpAstTypeArray(const SAstTypeArray* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeArray>\n");
	Tab++;
	{
		DumpAstTypeNullable((const SAstTypeNullable*)ast);
		PrintAst(L"TypeArray_ItemType", (const SAst*)ast->ItemType);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeArray>\n");
}

static void DumpAstTypeBit(const SAstTypeBit* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeBit");
	fwprintf(FilePtr, L" Size=\"%d\"", ast->Size);
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstType((const SAstType*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeBit>\n");
}

static void DumpAstTypeFunc(const SAstTypeFunc* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeFunc>\n");
	Tab++;
	{
		DumpAstTypeNullable((const SAstTypeNullable*)ast);
		PrintTab(); fwprintf(FilePtr, L"<TypeFunc_Args>\n");
		Tab++;
		{
			SListNode* ptr = ast->Args->Top;
			while (ptr != NULL)
			{
				SAstTypeFuncArg* arg = (SAstTypeFuncArg*)ptr->Data;
				PrintTab(); fwprintf(FilePtr, L"<TypeFunc_Arg");
				fwprintf(FilePtr, L" RefVar=\"%s\"", arg->RefVar ? L"True" : L"False");
				fwprintf(FilePtr, L">\n");
				Tab++;
				{
					DumpRecursion((const SAst*)arg->Arg);
				}
				Tab--;
				PrintTab(); fwprintf(FilePtr, L"</TypeFunc_Arg>\n");
				ptr = ptr->Next;
			}
		}
		Tab--;
		PrintTab(); fwprintf(FilePtr, L"</TypeFunc_Args>\n");
		if (ast->Ret != NULL)
			PrintAst(L"TypeFunc_Ret", (const SAst*)ast->Ret);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeFunc>\n");
}

static void DumpAstTypeGen(const SAstTypeGen* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeGen");
	{
		fwprintf(FilePtr, L" Kind=\"");
		switch (ast->Kind)
		{
			case AstTypeGenKind_List: fwprintf(FilePtr, L"List"); break;
			case AstTypeGenKind_Stack: fwprintf(FilePtr, L"Stack"); break;
			case AstTypeGenKind_Queue: fwprintf(FilePtr, L"Queue"); break;
			default:
				ASSERT(False);
				break;
		}
		fwprintf(FilePtr, L"\"");
	}
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstTypeNullable((const SAstTypeNullable*)ast);
		PrintAst(L"TypeGen_ItemType", (const SAst*)ast->ItemType);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeGen>\n");
}

static void DumpAstTypeDict(const SAstTypeDict* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeDict>\n");
	Tab++;
	{
		DumpAstTypeNullable((const SAstTypeNullable*)ast);
		PrintAst(L"TypeDict_ItemTypeKey", (const SAst*)ast->ItemTypeKey);
		PrintAst(L"TypeDict_ItemTypeValue", (const SAst*)ast->ItemTypeValue);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeDict>\n");
}

static void DumpAstTypePrim(const SAstTypePrim* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypePrim");
	{
		fwprintf(FilePtr, L" Kind=\"");
		switch (ast->Kind)
		{
			case AstTypePrimKind_Int: fwprintf(FilePtr, L"Int"); break;
			case AstTypePrimKind_Float: fwprintf(FilePtr, L"Float"); break;
			case AstTypePrimKind_Char: fwprintf(FilePtr, L"Char"); break;
			case AstTypePrimKind_Bool: fwprintf(FilePtr, L"Bool"); break;
			default:
				ASSERT(False);
				break;
		}
		fwprintf(FilePtr, L"\"");
	}
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstType((const SAstType*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypePrim>\n");
}

static void DumpAstTypeUser(const SAstTypeUser* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeUser>\n");
	Tab++;
	{
		DumpAstTypeNullable((const SAstTypeNullable*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeUser>\n");
}

static void DumpAstTypeNull(const SAstTypeNull* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeNull>\n");
	Tab++;
	{
		DumpAstType((const SAstType*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeNull>\n");
}

static void DumpAstTypeEnumElement(const SAstTypeEnumElement* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<TypeEnumElement>\n");
	Tab++;
	{
		DumpAstType((const SAstType*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</TypeEnumElement>\n");
}

static void DumpAstExpr(const SAstExpr* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Expr");
	{
		fwprintf(FilePtr, L" VarKind=\"");
		switch (ast->VarKind)
		{
			case AstExprVarKind_Unknown: fwprintf(FilePtr, L"Unknown"); break;
			case AstExprVarKind_Value: fwprintf(FilePtr, L"Value"); break;
			case AstExprVarKind_LocalVar: fwprintf(FilePtr, L"LocalVar"); break;
			case AstExprVarKind_GlobalVar: fwprintf(FilePtr, L"GlobalVar"); break;
			case AstExprVarKind_RefVar: fwprintf(FilePtr, L"RefVar"); break;
			default:
				ASSERT(False);
				break;
		}
		fwprintf(FilePtr, L"\"");
	}
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAst((const SAst*)ast);
		if (ast->Type != NULL)
			PrintAst(L"Expr_Type", (const SAst*)ast->Type);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Expr>\n");
}

static void DumpAstExpr1(const SAstExpr1* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Expr1");
	{
		fwprintf(FilePtr, L" Kind=\"");
		switch (ast->Kind)
		{
			case AstExpr1Kind_Plus: fwprintf(FilePtr, L"Plus"); break;
			case AstExpr1Kind_Minus: fwprintf(FilePtr, L"Minus"); break;
			case AstExpr1Kind_Not: fwprintf(FilePtr, L"Not"); break;
			case AstExpr1Kind_Copy: fwprintf(FilePtr, L"Copy"); break;
			case AstExpr1Kind_Len: fwprintf(FilePtr, L"Len"); break;
			default:
				ASSERT(False);
				break;
		}
		fwprintf(FilePtr, L"\"");
	}
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"Expr1_Child", (const SAst*)ast->Child);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Expr1>\n");
}

static void DumpAstExpr2(const SAstExpr2* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Expr2");
	{
		fwprintf(FilePtr, L" Kind=\"");
		switch (ast->Kind)
		{
			case AstExpr2Kind_Assign: fwprintf(FilePtr, L"Assign"); break;
			case AstExpr2Kind_AssignAdd: fwprintf(FilePtr, L"AssignAdd"); break;
			case AstExpr2Kind_AssignSub: fwprintf(FilePtr, L"AssignSub"); break;
			case AstExpr2Kind_AssignMul: fwprintf(FilePtr, L"AssignMul"); break;
			case AstExpr2Kind_AssignDiv: fwprintf(FilePtr, L"AssignDiv"); break;
			case AstExpr2Kind_AssignMod: fwprintf(FilePtr, L"AssignMod"); break;
			case AstExpr2Kind_AssignPow: fwprintf(FilePtr, L"AssignPow"); break;
			case AstExpr2Kind_AssignCat: fwprintf(FilePtr, L"AssignCat"); break;
			case AstExpr2Kind_Or: fwprintf(FilePtr, L"Or"); break;
			case AstExpr2Kind_And: fwprintf(FilePtr, L"And"); break;
			case AstExpr2Kind_LT: fwprintf(FilePtr, L"LT"); break;
			case AstExpr2Kind_GT: fwprintf(FilePtr, L"GT"); break;
			case AstExpr2Kind_LE: fwprintf(FilePtr, L"LE"); break;
			case AstExpr2Kind_GE: fwprintf(FilePtr, L"GE"); break;
			case AstExpr2Kind_Eq: fwprintf(FilePtr, L"Eq"); break;
			case AstExpr2Kind_NEq: fwprintf(FilePtr, L"NEq"); break;
			case AstExpr2Kind_EqRef: fwprintf(FilePtr, L"EqRef"); break;
			case AstExpr2Kind_NEqRef: fwprintf(FilePtr, L"NEqRef"); break;
			case AstExpr2Kind_Cat: fwprintf(FilePtr, L"Cat"); break;
			case AstExpr2Kind_Add: fwprintf(FilePtr, L"Add"); break;
			case AstExpr2Kind_Sub: fwprintf(FilePtr, L"Sub"); break;
			case AstExpr2Kind_Mul: fwprintf(FilePtr, L"Mul"); break;
			case AstExpr2Kind_Div: fwprintf(FilePtr, L"Div"); break;
			case AstExpr2Kind_Mod: fwprintf(FilePtr, L"Mod"); break;
			case AstExpr2Kind_Pow: fwprintf(FilePtr, L"Pow"); break;
			case AstExpr2Kind_Swap: fwprintf(FilePtr, L"Swap"); break;
			default:
				ASSERT(False);
				break;
		}
		fwprintf(FilePtr, L"\"");
	}
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"Expr2_Child1", (const SAst*)ast->Children[0]);
		PrintAst(L"Expr2_Child2", (const SAst*)ast->Children[1]);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Expr2>\n");
}

static void DumpAstExpr3(const SAstExpr3* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<Expr3>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"Expr3_Child1", (const SAst*)ast->Children[0]);
		PrintAst(L"Expr3_Child2", (const SAst*)ast->Children[1]);
		PrintAst(L"Expr3_Child3", (const SAst*)ast->Children[2]);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</Expr3>\n");
}

static void DumpAstExprNew(const SAstExprNew* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprNew>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprNew_ItemType", (const SAst*)ast->ItemType);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprNew>\n");
}

static void DumpAstExprNewArray(const SAstExprNewArray* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprNewArray>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAstList(L"ExprNewArray_Idces", ast->Idces);
		PrintAst(L"ExprNewArray_ItemType", (const SAst*)ast->ItemType);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprNewArray>\n");
}

static void DumpAstExprAs(const SAstExprAs* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprAs");
	{
		fwprintf(FilePtr, L" Kind=\"");
		switch (ast->Kind)
		{
			case AstExprAsKind_As: fwprintf(FilePtr, L"As"); break;
			case AstExprAsKind_Is: fwprintf(FilePtr, L"Is"); break;
			case AstExprAsKind_NIs: fwprintf(FilePtr, L"NIs"); break;
			default:
				ASSERT(False);
				break;
		}
		fwprintf(FilePtr, L"\"");
	}
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprAs_Child", (const SAst*)ast->Child);
		PrintAst(L"ExprAs_ChildType", (const SAst*)ast->ChildType);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprAs>\n");
}

static void DumpAstExprToBin(const SAstExprToBin* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprToBin>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprToBin_Child", (const SAst*)ast->Child);
		PrintAst(L"ExprToBin_ChildType", (const SAst*)ast->ChildType);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprToBin>\n");
}

static void DumpAstExprFromBin(const SAstExprFromBin* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprFromBin>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprFromBin_Child", (const SAst*)ast->Child);
		PrintAst(L"ExprFromBin_ChildType", (const SAst*)ast->ChildType);
		PrintAst(L"ExprFromBin_Offset", (const SAst*)ast->Offset);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprFromBin>\n");
}

static void DumpAstExprCall(const SAstExprCall* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprCall>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprCall_Func", (const SAst*)ast->Func);
		PrintTab(); fwprintf(FilePtr, L"<ExprCall_Args>\n");
		Tab++;
		{
			SListNode* ptr = ast->Args->Top;
			while (ptr != NULL)
			{
				SAstExprCallArg* arg = (SAstExprCallArg*)ptr->Data;
				PrintTab(); fwprintf(FilePtr, L"<ExprCall_Arg");
				fwprintf(FilePtr, L" RefVar=\"%s\"", arg->RefVar ? L"True" : L"False");
				fwprintf(FilePtr, L">\n");
				Tab++;
				{
					DumpRecursion((const SAst*)arg->Arg);
				}
				Tab--;
				PrintTab(); fwprintf(FilePtr, L"</ExprCall_Arg>\n");

				ptr = ptr->Next;
			}
		}
		Tab--;
		PrintTab(); fwprintf(FilePtr, L"</ExprCall_Args>\n");
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprCall>\n");
}

static void DumpAstExprArray(const SAstExprArray* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprArray>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprArray_Var", (const SAst*)ast->Var);
		PrintAst(L"ExprArray_Idx", (const SAst*)ast->Idx);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprArray>\n");
}

static void DumpAstExprDot(const SAstExprDot* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprDot");
	fwprintf(FilePtr, L" Member=\"%s\"", ast->Member);
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAst(L"ExprDot_Var", (const SAst*)ast->Var);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprDot>\n");
}

static void DumpAstExprValue(const SAstExprValue* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprValue");
	fwprintf(FilePtr, L" Value=\"0x%016I64X\"", *(U64*)ast->Value);
	fwprintf(FilePtr, L">\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprValue>\n");
}

static void DumpAstExprValueArray(const SAstExprValueArray* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprValueArray>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
		PrintAstList(L"ExprValueArray_Values", ast->Values);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprValueArray>\n");
}

static void DumpAstExprRef(const SAstExpr* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<ExprRef>\n");
	Tab++;
	{
		DumpAstExpr((const SAstExpr*)ast);
	}
	Tab--;
	PrintTab(); fwprintf(FilePtr, L"</ExprRef>\n");
}

static void DumpScope(const Char* name, const SAst* ast)
{
	PrintTab(); fwprintf(FilePtr, L"<%s", name);
	fwprintf(FilePtr, L" Name=\"%s\"", ast->Name == NULL ? L"(Null)" : ast->Name);
	if (ast->ScopeParent != NULL)
		fwprintf(FilePtr, L" Parent=\"%s\"", ast->ScopeParent->Name == NULL ? L"(Null)" : ast->ScopeParent->Name);
	fwprintf(FilePtr, L">\n");
	Tab++;
	if (ast->ScopeRefedItems != NULL)
	{
		PrintTab(); fwprintf(FilePtr, L"<Refed>\n");
		Tab++;
		{
			SListNode* ptr = ast->ScopeRefedItems->Top;
			while (ptr != NULL)
			{
				const SAst* ast2 = (const SAst*)ptr->Data;
				PrintTab(); fwprintf(FilePtr, L"<%s />\n", ast2->Name == NULL ? L"Null" : ast2->Name);
				ptr = ptr->Next;
			}
		}
		Tab--;
		PrintTab(); fwprintf(FilePtr, L"</Refed>\n");
	}
	Tab--;
	if (ast->ScopeChildren != NULL)
	{
		Tab++;
		{
			DictForEach(ast->ScopeChildren, DumpScopeChildren, NULL);
		}
		Tab--;
	}
	PrintTab(); fwprintf(FilePtr, L"</%s>\n", name);
}

static const void* DumpScopeChildren(const Char* key, const void* value, void* param)
{
	UNUSED(param);
	DumpScope(key, (const SAst*)value);
	return value;
}
