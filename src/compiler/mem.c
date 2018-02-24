#include "mem.h"

#define MEM_SIZE (32 * 1024 * 1024)

typedef struct SMemList
{
	void* Mem;
	struct SMemList* Next;
} SMemList;

static SMemList* TopMem = NULL;
static SMemList* BottomMem = NULL;
static SMemList* CurMem = NULL;
static void* CurMemBuf = NULL;

void* Alloc(size_t size)
{
	if ((U8*)CurMemBuf + size >= (U8*)CurMem->Mem + MEM_SIZE)
	{
		if (CurMem == BottomMem)
		{
			void* mem = malloc(MEM_SIZE);
			SMemList* node = (SMemList*)malloc(sizeof(SMemList));
			node->Mem = mem;
			node->Next = NULL;
			BottomMem->Next = node;
			BottomMem = node;
			CurMem = BottomMem;
		}
		else
			CurMem = CurMem->Next;
		CurMemBuf = CurMem->Mem;
	}
	{
		void* result = CurMemBuf;
		CurMemBuf = ((U8*)CurMemBuf) + size;
		return result;
	}
}

void InitAllocator(void)
{
	ASSERT(TopMem == NULL);
	{
		void* mem = malloc(MEM_SIZE);
		SMemList* node = (SMemList*)malloc(sizeof(SMemList));
		node->Mem = mem;
		node->Next = NULL;
		TopMem = node;
		BottomMem = node;
		CurMem = node;
		CurMemBuf = CurMem->Mem;
	}
}

void FinAllocator(void)
{
	ASSERT(TopMem != NULL);
	{
		SMemList* ptr = TopMem;
		while (ptr != NULL)
		{
			SMemList* ptr2 = ptr;
			free(ptr->Mem);
			ptr = ptr->Next;
			free(ptr2);
		}
		TopMem = NULL;
		BottomMem = NULL;
		CurMem = NULL;
		CurMemBuf = NULL;
	}
}

void ResetAllocator(void)
{
	ASSERT(TopMem != NULL);
	CurMem = TopMem;
	CurMemBuf = CurMem->Mem;
}
