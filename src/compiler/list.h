#pragma once

#include "..\common.h"

typedef struct SListNode
{
	const void* Data;
	struct SListNode* Prev;
	struct SListNode* Next;
} SListNode;

typedef struct SList
{
	SListNode* Top;
	SListNode* Bottom;
	int Len;
} SList;

SList* ListNew(void);
void ListAdd(SList* set, const void* data);
void ListIns(SList* set, SListNode* ptr, const void* data);
void ListDel(SList* set, SListNode** ptr);
const void** ListToArray(const SList* set);
