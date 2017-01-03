#pragma once

#include "..\common.h"

typedef struct SStack
{
	const void* Data;
	struct SStack* Next;
} SStack;

SStack* StackPush(SStack* top, const void* data);
SStack* StackPop(SStack* top);
const void* StackPeek(const SStack* top);
Bool StackIsEmpty(const SStack* top);
