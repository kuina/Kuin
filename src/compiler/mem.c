#include "mem.h"

#define MEM_BUF_MAX (2)
#define MEM_SIZE (32 * 1024 * 1024)

typedef struct SMemList
{
	void* Mem;
	struct SMemList* Next;
} SMemList;

static SMemList* TopMem[MEM_BUF_MAX] = { NULL };
static SMemList* BottomMem[MEM_BUF_MAX] = { NULL };
static SMemList* CurMem = NULL;
static void* CurMemBuf = NULL;
static SMemList* CurBottom = NULL;

void* Alloc(size_t size)
{
	if ((U8*)CurMemBuf + size >= (U8*)CurMem->Mem + MEM_SIZE)
	{
		if (CurMem == CurBottom)
		{
			void* mem = malloc(MEM_SIZE);
			SMemList* node = (SMemList*)malloc(sizeof(SMemList));
			node->Mem = mem;
			node->Next = NULL;
			CurBottom->Next = node;
			CurBottom = node;
			CurMem = CurBottom;
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

void InitAllocator(S64 mem_buf_num)
{
	ASSERT(TopMem[0] == NULL && mem_buf_num > 0);
	S64 i;
	for (i = 0; i < MEM_BUF_MAX; i++)
	{
		if (i < mem_buf_num)
		{
			void* mem = malloc(MEM_SIZE);
			SMemList* node = (SMemList*)malloc(sizeof(SMemList));
			node->Mem = mem;
			node->Next = NULL;
			TopMem[i] = node;
			BottomMem[i] = node;
		}
		else
		{
			TopMem[i] = NULL;
			BottomMem[i] = NULL;
		}
	}
	CurMem = TopMem[0];
	CurBottom = BottomMem[0];
	CurMemBuf = CurMem->Mem;
}

void FinAllocator(void)
{
	ASSERT(TopMem[0] != NULL);
	S64 i;
	for (i = 0; i < MEM_BUF_MAX; i++)
	{
		if (TopMem[i] != NULL)
		{
			SMemList* ptr = TopMem[i];
			while (ptr != NULL)
			{
				SMemList* ptr2 = ptr;
				free(ptr->Mem);
				ptr = ptr->Next;
				free(ptr2);
			}
			TopMem[i] = NULL;
		}
		BottomMem[i] = NULL;
	}
	CurMem = NULL;
	CurMemBuf = NULL;
	CurBottom = NULL;
}

void ResetAllocator(S64 idx)
{
	ASSERT(TopMem[0] != NULL && 0 <= idx && idx < MEM_BUF_MAX && TopMem[idx] != NULL);
	CurMem = TopMem[idx];
	CurMemBuf = CurMem->Mem;
	CurBottom = BottomMem[idx];
}
