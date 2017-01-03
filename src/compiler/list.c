#include "list.h"

#include "mem.h"

SList* ListNew(void)
{
	SList* set = (SList*)Alloc(sizeof(SList));
	set->Top = NULL;
	set->Bottom = NULL;
	set->Len = 0;
	return set;
}

void ListAdd(SList* set, const void* data)
{
	ASSERT(set != NULL);
	{
		SListNode* node = (SListNode*)Alloc(sizeof(SListNode));
		node->Data = data;
		node->Next = NULL;
		if (set->Top == NULL)
		{
			node->Prev = NULL;
			set->Top = node;
			set->Bottom = node;
		}
		else
		{
			node->Prev = set->Bottom;
			set->Bottom->Next = node;
			set->Bottom = node;
		}
	}
	set->Len++;
}

void ListIns(SList* set, SListNode* ptr, const void* data)
{
	if (ptr == NULL)
	{
		ListAdd(set, data);
		return;
	}
	ASSERT(set != NULL);
	{
		SListNode* node = (SListNode*)Alloc(sizeof(SListNode));
		node->Data = data;
		if (ptr->Prev == NULL)
			set->Top = node;
		else
			ptr->Prev->Next = node;
		node->Next = ptr;
		node->Prev = ptr->Prev;
		ptr->Prev = node;
	}
	set->Len++;
}

void ListDel(SList* set, SListNode** ptr)
{
	ASSERT(set != NULL);
	ASSERT(*ptr != NULL);
	{
		SListNode* next = (*ptr)->Next;
		if ((*ptr)->Prev == NULL)
			set->Top = (*ptr)->Next;
		else
			(*ptr)->Prev->Next = (*ptr)->Next;
		if ((*ptr)->Next == NULL)
			set->Bottom = (*ptr)->Prev;
		else
			(*ptr)->Next->Prev = (*ptr)->Prev;
		(*ptr) = next;
	}
	set->Len--;
}

const void** ListToArray(const SList* set)
{
	ASSERT(set != NULL);
	{
		int cnt = 0;
		{
			const SListNode* node = set->Top;
			while (node != NULL)
			{
				cnt++;
				node = node->Next;
			}
		}
		if (cnt == 0)
			return NULL;
		{
			const void** array = (void**)Alloc(sizeof(void*) * (size_t)cnt);
			const SListNode* node = set->Top;
			int i;
			for (i = 0; i < cnt; i++)
			{
				array[i] = node->Data;
				node = node->Next;
			}
			return array;
		}
	}
}
