#pragma once

#include "..\common.h"

#include "ast.h"
#include "dict.h"
#include "list.h"
#include "option.h"

typedef struct SPackAsm
{
	SList* Asms;
	SList* ReadonlyData;
	SList* DLLImport;
	SList* ExcptTables;
	SDict* WritableData;
	SList* ResEntries;
	int ResIconNum;
	U8 (*ResIconHeaderBins)[14];
	int* ResIconBinSize;
	U8** ResIconBins;
	SList* RefValueList;
	SDictI* FuncAddrs;
	S64* ExcptFunc;
	SList* ClassTables;
	S64 AppCode;
	const Char* AppName;
} SPackAsm;

typedef struct SReadonlyData
{
	Bool Align128;
	int BufSize;
	const U8* Buf;
	S64* Addr;
} SReadonlyData;

typedef struct SDLLImportFunc
{
	int FuncNameSize;
	const U8* FuncName;
	S64* Addr;
} SDLLImportFunc;

typedef struct SDLLImport
{
	int DLLNameSize;
	const U8* DllName;
	S64 Addr;
	SList* Funcs;
} SDLLImport;

typedef struct SWritableData
{
	S64* Addr;
	int Size;
} SWritableData;

typedef struct SResEntry
{
	int Value;
	struct SList* Children;
	S64 Addr;
} SResEntry;

typedef struct SClassTable
{
	S64* Addr;
	S64* Parent;
	SAstClass* Class;
} SClassTable;

typedef struct SExcptTableTry
{
	SAsmLabel* Begin;
	SAsmLabel* End;
	S64* CatchFunc;
} SExcptTableTry;

typedef struct SExcptTable
{
	SAsmLabel* Begin;
	SAsmLabel* End;
	SAsmLabel* PostPrologue;
	SList* TryScopes;
	int StackSize;
	S64 Addr;
} SExcptTable;

void Assemble(SPackAsm* pack_asm, const SAstFunc* entry, const SOption* option, SDict* dlls, S64 app_code, const Char* app_name);
