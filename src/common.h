#pragma once

#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
#if defined(_DEBUG)
	#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#else
	#pragma comment(linker, "/nodefaultlib:libcmtd.lib")
#endif

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "imm32.lib")

#define STRICT
#define _WIN32_DCOM
#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <WinSock2.h>
#include <Windows.h>
#include <crtdbg.h>
#include <time.h>
#include <emmintrin.h> // 'SSE2'
#include <Shlwapi.h> // 'PathFileExists'
#include <Shlobj.h> // 'SHGetSpecialFolderPath'

#define UNUSED(var) (void)(var)
#define EXPORT _declspec(dllexport)
#define EXPORT_CPP extern "C" _declspec(dllexport)
#define KUIN_MAX_PATH (512)
#define STACK_STRING_BUF_SIZE (1024) // Default size of string buffer on stack.

#define EXCPT_ACCESS_VIOLATION (0xc0000005)
#define EXCPT_DBG_ASSERT_FAILED (0xe9170000)
#define EXCPT_CLASS_CAST_FAILED (0xe9170001)
#define EXCPT_DBG_ARRAY_IDX_OUT_OF_RANGE (0xe9170002)
#define EXCPT_DBG_INT_OVERFLOW (0xe9170003)
#define EXCPT_INVALID_CMP (0xe9170004)
#define EXCPT_DBG_ARG_OUT_DOMAIN (0xe9170006)
#define EXCPT_FILE_READ_FAILED (0xe9170007)
#define EXCPT_INVALID_DATA_FMT (0xe9170008)
#define EXCPT_DEVICE_INIT_FAILED (0xe9170009)
#define EXCPT_DBG_INOPERABLE_STATE (0xe917000a)

#if defined(_DEBUG)
	#define ASSERT(cond) _ASSERTE((cond))
	#define STATIC_ASSERT(cond) static_assert((cond), "Static assertion failed.")
#else
	#define ASSERT(cond) {}
	#define STATIC_ASSERT(cond)
#endif

#if _MSC_VER >= 1900
	#define VS2015
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
} SClass;

static const S64 DefaultRefCntFunc = 0; // Just before exiting the function, this is incremented for 'GcInstance'.
static const S64 DefaultRefCntOpe = 1; // For 'GcInstance'.

static const void* DummyPtr = (void*)1i64; // An invalid pointer used to point to non-NULL.

typedef enum EUseResFlagsKind
{
	UseResFlagsKind_Draw_Circle = 1,
	UseResFlagsKind_Draw_FilterMonotone = 2,
	UseResFlagsKind_Draw_Particle = 3,
	UseResFlagsKind_Draw_Poly = 4,
	UseResFlagsKind_Draw_ObjDraw = 5,
	UseResFlagsKind_Draw_ObjDrawOutline = 6,
} EUseResFlagsKind;
#define USE_RES_FLAGS_LEN (1)

/*
	Uncommitted libraries:
		d0000.knd	common
		d0001.knd	wnd
		d0002.knd	cui
		d0003.knd	math
		d0004.knd	net
		d0005.knd	draw2d
		d0917.knd	compiler

	Committed additional libraries:
		d1000.knd	ogg
		d1001.knd	zip
		d1002.knd	regex(boost)
		d1003.knd	xml
		d1004.knd	game
		d1005.knd	sql
		d1006.knd	math_boost
*/

typedef struct SEnvVars
{
	void* Heap;
#if defined(_DEBUG)
	S64* HeapCnt;
#endif
	S64 AppCode;
	const U8* UseResFlags;
	Char ResRoot[KUIN_MAX_PATH];
} SEnvVars;
extern SEnvVars EnvVars;

Bool InitEnvVars(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
void* AllocMem(size_t size);
void* ReAllocMem(void* ptr, size_t size);
void FreeMem(void* ptr);
void ThrowImpl(U32 code);
void* LoadFileAll(const Char* path, size_t* size);
void* OpenFileStream(const Char* path);
void CloseFileStream(void* handle);
size_t ReadFileStream(void* handle, size_t size, void* buf);
Bool SeekFileStream(void* handle, S64 offset, S64 origin);
S64 TellFileStream(void* handle);
Bool StrCmpIgnoreCase(const Char* a, const Char* b);
U8 SwapEndianU8(U8 n);
U16 SwapEndianU16(U16 n);
U32 SwapEndianU32(U32 n);
U64 SwapEndianU64(U64 n);
Bool IsPowerOf2(U64 n);
U32 MakeSeed(U32 key);
U32 XorShift(U32* seed);
U64 XorShift64(U32* seed);
S64 XorShiftInt(U32* seed, S64 min, S64 max);
double XorShiftFloat(U32* seed, double min, double max);
char* Utf16ToUtf8(const U8* str);
U8* Utf8ToUtf16(const char* str);
Bool IsResUsed(EUseResFlagsKind kind);

#define THROW(code) ThrowImpl(code)
#if defined(DBG)
#define THROWDBG(condition, code) if (condition) ThrowImpl(code)
#else
#define THROWDBG(condition, code)
#endif
