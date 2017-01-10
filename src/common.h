#pragma once

#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
#if defined(_DEBUG)
	#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#else
	#pragma comment(linker, "/nodefaultlib:libcmtd.lib")
#endif

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")

#define STRICT
#define _WIN32_DCOM
#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Windows.h>
#include <crtdbg.h>
#include <time.h>
#include <emmintrin.h> // 'SSE2'
#include <Shlwapi.h> // 'PathFileExists'

#define UNUSED(var) (void)(var)
#define EXPORT _declspec(dllexport)

#if defined(_DEBUG)
	#define ASSERT(cond) _ASSERTE((cond))
	#define STATIC_ASSERT(cond) static_assert((cond), "Static assertion failed.")
#else
	#define ASSERT(cond)
	#define STATIC_ASSERT(cond)
#endif

typedef unsigned char Bool;
typedef wchar_t Char;
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long long S64;
typedef __m128i S128;

static const Bool False = 0;
static const Bool True = 1;

typedef struct SClass
{
	U64 RefCnt;
	void* ClassTable;
	void(*Ctor)(struct SClass* me_);
	void(*Dtor)(struct SClass* me_);
	int(*Cmp)(const struct SClass* me_, const struct SClass* t);
	struct SClass*(*Copy)(const struct SClass* me_);
	U8*(*ToBin)(const struct SClass* me_);
	S64(*FromBin)(struct SClass* me_, U8* bin, S64 idx);
} SClass;

static const S64 DefaultRefCntFunc = 0; // Just before exiting the function, this is incremented for 'GcInstance'.
static const S64 DefaultRefCntOpe = 1; // For 'GcInstance'.

static void* DummyPtr = (void*)1i64; // An invalid pointer used to point to non-NULL.

extern void* Heap;
extern S64* HeapCnt;
extern S64 AppCode;
extern const Char* AppName;
extern HINSTANCE Instance;

void* AllocMem(size_t size);
void FreeMem(void* ptr);
void ThrowImpl(U32 code, const Char* msg);
void InitClass(SClass* class_, void(*ctor)(SClass* me_), void(*dtor)(SClass* me_));

#if defined(DBG)
#define THROW(code, msg) ThrowImpl((code), (msg))
#else
#define THROW(code, msg) ThrowImpl((code), NULL)
#endif
