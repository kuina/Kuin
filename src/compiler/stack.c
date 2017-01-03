#include "stack.h"

#include "mem.h"

SStack* StackPush(SStack* top, const void* data)
{
	SStack* node = (SStack*)Alloc(sizeof(SStack));
	node->Data = data;
	node->Next = top;
	top = node;
	return top;
}

SStack* StackPop(SStack* top)
{
	ASSERT(top != NULL);
	top = top->Next;
	return top;
}

const void* StackPeek(const SStack* top)
{
	ASSERT(top != NULL);
	return top->Data;
}

Bool StackIsEmpty(const SStack* top)
{
	return top == NULL;
}
