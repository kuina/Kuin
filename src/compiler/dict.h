#pragma once

#include "..\common.h"

typedef struct SDict
{
	const Char* Key;
	const void* Value;
	Bool Red;
	struct SDict* Left;
	struct SDict* Right;
} SDict;

typedef struct SDictI
{
	U64 Key;
	const void* Value;
	Bool Red;
	struct SDictI* Left;
	struct SDictI* Right;
} SDictI;

SDict* DictAdd(SDict* root, const Char* key, const void* value);
const void* DictSearch(const SDict* root, const Char* key);
void DictForEach(SDict* root, const void*(*func)(const Char* key, const void* value, void* param), void* param);
SDictI* DictIAdd(SDictI* root, U64 key, const void* value);
const void* DictISearch(const SDictI* root, U64 key);
void DictIForEach(SDictI* root, const void*(*func)(U64 key, const void* value, void* param), void* param);
