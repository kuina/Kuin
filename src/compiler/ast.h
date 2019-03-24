#pragma once

#include "..\common.h"

#include "asm.h"
#include "dict.h"
#include "list.h"
#include "pos.h"

struct SAstArg;
struct SAstStatBlock;
struct SAstType;
struct SAstExpr;

typedef enum EAstTypeId
{
	AstTypeId_Ast = 0x00,
	AstTypeId_Root = 0x01,
	AstTypeId_Func = 0x0100,
	AstTypeId_FuncRaw = AstTypeId_Func | 0x01,
	AstTypeId_Var = 0x02,
	AstTypeId_Const = 0x03,
	AstTypeId_Alias = 0x04,
	AstTypeId_Class = 0x05,
	AstTypeId_Enum = 0x06,
	AstTypeId_Arg = 0x07,
	AstTypeId_Stat = 0x0200,
	AstTypeId_StatBreakable = AstTypeId_Stat | 0x00010000,
	AstTypeId_StatSkipable = AstTypeId_StatBreakable | 0x01000000,
	AstTypeId_StatEnd = AstTypeId_Stat | 0x01,
	AstTypeId_StatFunc = AstTypeId_Stat | 0x02,
	AstTypeId_StatVar = AstTypeId_Stat | 0x03,
	AstTypeId_StatConst = AstTypeId_Stat | 0x04,
	AstTypeId_StatAlias = AstTypeId_Stat | 0x05,
	AstTypeId_StatClass = AstTypeId_Stat | 0x06,
	AstTypeId_StatEnum = AstTypeId_Stat | 0x07,
	AstTypeId_StatIf = AstTypeId_StatBreakable | 0x01,
	AstTypeId_StatElIf = AstTypeId_Stat | 0x08,
	AstTypeId_StatElse = AstTypeId_Stat | 0x09,
	AstTypeId_StatSwitch = AstTypeId_StatBreakable | 0x02,
	AstTypeId_StatCase = AstTypeId_Stat | 0x0a,
	AstTypeId_StatDefault = AstTypeId_Stat | 0x0b,
	AstTypeId_StatWhile = AstTypeId_StatSkipable | 0x01,
	AstTypeId_StatFor = AstTypeId_StatSkipable | 0x02,
	AstTypeId_StatTry = AstTypeId_StatBreakable | 0x03,
	AstTypeId_StatCatch = AstTypeId_Stat | 0x0c,
	AstTypeId_StatFinally = AstTypeId_Stat | 0x0d,
	AstTypeId_StatThrow = AstTypeId_Stat | 0x0e,
	AstTypeId_StatBlock = AstTypeId_StatBreakable | 0x04,
	AstTypeId_StatRet = AstTypeId_Stat | 0x0f,
	AstTypeId_StatDo = AstTypeId_Stat | 0x10,
	AstTypeId_StatBreak = AstTypeId_Stat | 0x11,
	AstTypeId_StatSkip = AstTypeId_Stat | 0x12,
	AstTypeId_StatAssert = AstTypeId_Stat | 0x13,
	AstTypeId_Type = 0x0400,
	AstTypeId_TypeNullable = AstTypeId_Type | 0x00010000,
	AstTypeId_TypeArray = AstTypeId_TypeNullable | 0x01,
	AstTypeId_TypeBit = AstTypeId_Type | 0x01,
	AstTypeId_TypeFunc = AstTypeId_TypeNullable | 0x02,
	AstTypeId_TypeGen = AstTypeId_TypeNullable | 0x03,
	AstTypeId_TypeDict = AstTypeId_TypeNullable | 0x04,
	AstTypeId_TypePrim = AstTypeId_Type | 0x02,
	AstTypeId_TypeUser = AstTypeId_TypeNullable | 0x05,
	AstTypeId_TypeNull = AstTypeId_Type | 0x03,
	AstTypeId_TypeEnumElement = AstTypeId_Type | 0x04,
	AstTypeId_Expr = 0x0800,
	AstTypeId_Expr1 = AstTypeId_Expr | 0x01,
	AstTypeId_Expr2 = AstTypeId_Expr | 0x02,
	AstTypeId_Expr3 = AstTypeId_Expr | 0x03,
	AstTypeId_ExprNew = AstTypeId_Expr | 0x04,
	AstTypeId_ExprNewArray = AstTypeId_Expr | 0x05,
	AstTypeId_ExprAs = AstTypeId_Expr | 0x06,
	AstTypeId_ExprToBin = AstTypeId_Expr | 0x07,
	AstTypeId_ExprFromBin = AstTypeId_Expr | 0x08,
	AstTypeId_ExprCall = AstTypeId_Expr | 0x09,
	AstTypeId_ExprArray = AstTypeId_Expr | 0x0a,
	AstTypeId_ExprDot = AstTypeId_Expr | 0x0b,
	AstTypeId_ExprValue = AstTypeId_Expr | 0x0c,
	AstTypeId_ExprValueArray = AstTypeId_Expr | 0x0d,
	AstTypeId_ExprRef = AstTypeId_Expr | 0x0e,
} EAstTypeId;

typedef struct SAst
{
	EAstTypeId TypeId;
	const SPos* Pos;
	const Char* Name;
	const struct SAst* ScopeParent;
	SDict* ScopeChildren;
	SList* ScopeRefedItems;
	const Char* RefName;
	struct SAst* RefItem;
	struct SAst* AnalyzedCache;
	Bool Public;
} SAst;

typedef struct SAstRoot
{
	SAst Ast;
	SList* Items;
} SAstRoot;

typedef enum EFuncAttr
{
	FuncAttr_None = 0x00,
	FuncAttr_Callback = 0x01, // Conform to the x64 calling convention.
	FuncAttr_AnyType = 0x02, // Ignore type checking of 'me' and the add the type of it to the second argument.
	FuncAttr_Init = 0x04, // Pass necessary information for initialization.
	FuncAttr_TakeMe = 0x08, // The function receives a value of the same type as 'me' in the third argument.
	FuncAttr_RetMe = 0x10, // The function returns a value of the same type as 'me'.
	FuncAttr_TakeChild = 0x20, // The function receives a value of the type of elements of 'me' in the third argument.
	FuncAttr_RetChild = 0x40, // The function returns a value of the type of elements of 'me'.
	FuncAttr_TakeKeyValue = 0x80, // The function receives a value of the type of 'key' in the third argument and a value of the type of 'value' in the fourth.
	FuncAttr_RetArrayOfListChild = 0x0100, // The function returns an array of the type of list elements of 'me'.
	FuncAttr_MakeInstance = 0x0200, // Make an instance before calling the function.
	FuncAttr_Force = 0x0400, // Force to define a method that cannot be overridden.
	FuncAttr_ExitCode = 0x0800, // Set 'ExitCode'.
	FuncAttr_TakeKeyValueFunc = 0x1000, // The function receives a callback function that receives key-value pairs.
	FuncAttr_RetArrayOfDictKey = 0x2000,
	FuncAttr_RetArrayOfDictValue = 0x4000,
} EFuncAttr;

typedef struct SAstFunc
{
	SAst Ast;
	S64* AddrTop;
	S64 AddrBottom;
	S64 PosRowBottom;
	const Char* DllName;
	const Char* DllFuncName;
	EFuncAttr FuncAttr;
	SList* Args;
	struct SAstType* Ret;
	SList* Stats;
	SAsmLabel* RetPoint;
} SAstFunc;

typedef struct SAstFuncRaw
{
	SAstFunc AstFunc;
	SList* Asms;
	int ArgNum;
	SList* Header;
} SAstFuncRaw;

typedef struct SAstVar
{
	SAst Ast;
	struct SAstArg* Var;
} SAstVar;

typedef struct SAstConst
{
	SAst Ast;
	struct SAstArg* Var;
} SAstConst;

typedef struct SAstAlias
{
	SAst Ast;
	struct SAstType* Type;
} SAstAlias;

typedef struct SAstClassItem
{
	Bool Public;
	Bool Override;
	SAst* Def;
	struct SAstClassItem* ParentItem;
	S64 Addr;
} SAstClassItem;

typedef struct SAstClass
{
	SAst Ast;
	S64* Addr;
	int VarSize;
	int FuncSize;
	SList* Items;
	Bool IndirectCreation;
} SAstClass;

typedef struct SAstEnum
{
	SAst Ast;
	SList* Items;
} SAstEnum;

typedef enum EAstArgKind
{
	AstArgKind_Unknown,
	AstArgKind_Global,
	AstArgKind_LocalArg,
	AstArgKind_LocalVar,
	AstArgKind_Const,
	AstArgKind_Member,
} EAstArgKind;

typedef struct SAstArg
{
	SAst Ast;
	S64* Addr;
	EAstArgKind Kind;
	Bool RefVar;
	struct SAstType* Type;
	struct SAstExpr* Expr;
} SAstArg;

typedef struct SAstStat
{
	SAst Ast;
	SAsm* AsmTop;
	SAsm* AsmBottom;
	S64 PosRowBottom;
} SAstStat;

typedef struct SAstStatBreakable
{
	SAstStat AstStat;
	SAstArg* BlockVar;
	SAsmLabel* BreakPoint;
} SAstStatBreakable;

typedef struct SAstStatSkipable
{
	SAstStatBreakable AstStatBreakable;
	SAsmLabel* SkipPoint;
} SAstStatSkipable;

typedef struct SAstStatFunc
{
	SAstStat AstStat;
	SAstFunc* Def;
} SAstStatFunc;

typedef struct SAstStatVar
{
	SAstStat AstStat;
	SAstVar* Def;
} SAstStatVar;

typedef struct SAstStatConst
{
	SAstStat AstStat;
	SAstConst* Def;
} SAstStatConst;

typedef struct SAstStatAlias
{
	SAstStat AstStat;
	SAstAlias* Def;
} SAstStatAlias;

typedef struct SAstStatClass
{
	SAstStat AstStat;
	SAstClass* Def;
} SAstStatClass;

typedef struct SAstStatEnum
{
	SAstStat AstStat;
	SAstEnum* Def;
} SAstStatEnum;

typedef struct SAstStatIf
{
	SAstStatBreakable AstStatBreakable;
	struct SAstExpr* Cond;
	struct SAstStatBlock* StatBlock;
	SList* ElIfs;
	struct SAstStatBlock* ElseStatBlock;
} SAstStatIf;

typedef struct SAstStatElIf
{
	SAstStat AstStat;
	struct SAstExpr* Cond;
	struct SAstStatBlock* StatBlock;
} SAstStatElIf;

typedef struct SAstStatSwitch
{
	SAstStatBreakable AstStatBreakable;
	struct SAstExpr* Cond;
	SList* Cases;
	struct SAstStatBlock* DefaultStatBlock;
} SAstStatSwitch;

typedef struct SAstStatCase
{
	SAstStat AstStat;
	SList* Conds;
	struct SAstStatBlock* StatBlock;
} SAstStatCase;

typedef struct SAstStatWhile
{
	SAstStatSkipable AstStatSkipable;
	struct SAstExpr* Cond;
	Bool Skip;
	SList* Stats;
} SAstStatWhile;

typedef struct SAstStatFor
{
	SAstStatSkipable AstStatSkipable;
	struct SAstExpr* Start;
	struct SAstExpr* Cond;
	struct SAstExpr* Step;
	SList* Stats;
} SAstStatFor;

typedef struct SAstStatTry
{
	SAstStatBreakable AstStatBreakable;
	struct SAstStatBlock* StatBlock;
	SList* Catches;
	struct SAstStatBlock* FinallyStatBlock;
} SAstStatTry;

typedef struct SAstStatCatch
{
	SAstStat AstStat;
	SList* Conds;
	struct SAstStatBlock* StatBlock;
} SAstStatCatch;

typedef struct SAstStatThrow
{
	SAstStat AstStat;
	struct SAstExpr* Code;
} SAstStatThrow;

typedef struct SAstStatBlock
{
	SAstStatBreakable AstStatBreakable;
	SList* Stats;
} SAstStatBlock;

typedef struct SAstStatRet
{
	SAstStat AstStat;
	struct SAstExpr* Value;
} SAstStatRet;

typedef struct SAstStatDo
{
	SAstStat AstStat;
	struct SAstExpr* Expr;
} SAstStatDo;

typedef struct SAstStatAssert
{
	SAstStat AstStat;
	struct SAstExpr* Cond;
} SAstStatAssert;

typedef struct SAstType
{
	SAst Ast;
} SAstType;

typedef struct SAstTypeNullable
{
	SAstType AstType;
} SAstTypeNullable;

typedef struct SAstTypeArray
{
	SAstTypeNullable AstTypeNullable;
	SAstType* ItemType;
} SAstTypeArray;

typedef struct SAstTypeBit
{
	SAstType AstType;
	int Size;
} SAstTypeBit;

typedef struct SAstTypeFuncArg
{
	SAstType* Arg;
	Bool RefVar;
} SAstTypeFuncArg;

typedef struct SAstTypeFunc
{
	SAstTypeNullable AstTypeNullable;
	EFuncAttr FuncAttr;
	SList* Args;
	SAstType* Ret;
} SAstTypeFunc;

typedef enum EAstTypeGenKind
{
	AstTypeGenKind_List,
	AstTypeGenKind_Stack,
	AstTypeGenKind_Queue,
} EAstTypeGenKind;

typedef struct SAstTypeGen
{
	SAstTypeNullable AstTypeNullable;
	EAstTypeGenKind Kind;
	SAstType* ItemType;
} SAstTypeGen;

typedef struct SAstTypeDict
{
	SAstTypeNullable AstTypeNullable;
	SAstType* ItemTypeKey;
	SAstType* ItemTypeValue;
} SAstTypeDict;

typedef enum EAstTypePrimKind
{
	AstTypePrimKind_Int,
	AstTypePrimKind_Float,
	AstTypePrimKind_Char,
	AstTypePrimKind_Bool,
} EAstTypePrimKind;

typedef struct SAstTypePrim
{
	SAstType AstType;
	EAstTypePrimKind Kind;
} SAstTypePrim;

typedef struct SAstTypeUser
{
	SAstTypeNullable AstTypeNullable;
} SAstTypeUser;

typedef struct SAstTypeNull
{
	SAstType AstType;
} SAstTypeNull;

typedef struct SAstTypeEnumElement
{
	SAstType AstType;
} SAstTypeEnumElement;

typedef enum EAstExprVarKind
{
	AstExprVarKind_Unknown,
	AstExprVarKind_Value,
	AstExprVarKind_LocalVar,
	AstExprVarKind_GlobalVar,
	AstExprVarKind_RefVar,
} EAstExprVarKind;

typedef struct SAstExpr
{
	SAst Ast;
	SAstType* Type;
	EAstExprVarKind VarKind;
} SAstExpr;

typedef enum EAstExpr1Kind
{
	AstExpr1Kind_Plus,
	AstExpr1Kind_Minus,
	AstExpr1Kind_Not,
	AstExpr1Kind_Copy,
	AstExpr1Kind_Len,
} EAstExpr1Kind;

typedef struct SAstExpr1
{
	SAstExpr AstExpr;
	EAstExpr1Kind Kind;
	SAstExpr* Child;
} SAstExpr1;

typedef enum EAstExpr2Kind
{
	AstExpr2Kind_Assign,
	AstExpr2Kind_AssignAdd,
	AstExpr2Kind_AssignSub,
	AstExpr2Kind_AssignMul,
	AstExpr2Kind_AssignDiv,
	AstExpr2Kind_AssignMod,
	AstExpr2Kind_AssignPow,
	AstExpr2Kind_AssignCat,
	AstExpr2Kind_Or,
	AstExpr2Kind_And,
	AstExpr2Kind_LT,
	AstExpr2Kind_GT,
	AstExpr2Kind_LE,
	AstExpr2Kind_GE,
	AstExpr2Kind_Eq,
	AstExpr2Kind_NEq,
	AstExpr2Kind_EqRef,
	AstExpr2Kind_NEqRef,
	AstExpr2Kind_Cat,
	AstExpr2Kind_Add,
	AstExpr2Kind_Sub,
	AstExpr2Kind_Mul,
	AstExpr2Kind_Div,
	AstExpr2Kind_Mod,
	AstExpr2Kind_Pow,
	AstExpr2Kind_Swap,
} EAstExpr2Kind;

typedef struct SAstExpr2
{
	SAstExpr AstExpr;
	EAstExpr2Kind Kind;
	SAstExpr* Children[2];
} SAstExpr2;

typedef struct SAstExpr3
{
	SAstExpr AstExpr;
	SAstExpr* Children[3];
} SAstExpr3;

typedef struct SAstExprNew
{
	SAstExpr AstExpr;
	SAstType* ItemType;
	Bool AutoCreated;
} SAstExprNew;

typedef struct SAstExprNewArray
{
	SAstExpr AstExpr;
	SList* Idces;
	SAstType* ItemType;
} SAstExprNewArray;

typedef enum EAstExprAsKind
{
	AstExprAsKind_As,
	AstExprAsKind_Is,
	AstExprAsKind_NIs,
} EAstExprAsKind;

typedef struct SAstExprAs
{
	SAstExpr AstExpr;
	EAstExprAsKind Kind;
	SAstExpr* Child;
	SAstType* ChildType;
} SAstExprAs;

typedef struct SAstExprToBin
{
	SAstExpr AstExpr;
	SAstExpr* Child;
	SAstType* ChildType;
} SAstExprToBin;

typedef struct SAstExprFromBin
{
	SAstExpr AstExpr;
	SAstExpr* Child;
	SAstType* ChildType;
	SAstExpr* Offset;
} SAstExprFromBin;

typedef struct SAstExprCallArg
{
	SAstExpr* Arg;
	Bool RefVar;
	Bool SkipVar;
} SAstExprCallArg;

typedef struct SAstExprCall
{
	SAstExpr AstExpr;
	SAstExpr* Func;
	SList* Args;
} SAstExprCall;

typedef struct SAstExprArray
{
	SAstExpr AstExpr;
	SAstExpr* Var;
	SAstExpr* Idx;
} SAstExprArray;

typedef struct SAstExprDot
{
	SAstExpr AstExpr;
	SAstExpr* Var;
	const Char* Member;
	SAstClassItem* ClassItem; // For caching to store class references.
} SAstExprDot;

typedef struct SAstExprValue
{
	SAstExpr AstExpr;
	U8 Value[8];
} SAstExprValue;

typedef struct SAstExprValueArray
{
	SAstExpr AstExpr;
	SList* Values;
} SAstExprValueArray;

Bool IsInt(const SAstType* type);
Bool IsFloat(const SAstType* type);
Bool IsChar(const SAstType* type);
Bool IsBool(const SAstType* type);
Bool IsRef(const SAstType* type);
Bool IsNullable(const SAstType* type);
Bool IsClass(const SAstType* type);
Bool IsEnum(const SAstType* type);
Bool IsStr(const SAstType* type);
void Dump1(const Char* path, const SAst* ast);
void GetTypeName(Char* buf, size_t* len, size_t size, const SAstType* ast);
