#pragma once

/*
	Uncommitted libraries:
		d0000.knd lib_common
		d0001.knd lib_win
		d0004.knd lib_net
		d0005.knd lib_draw2d

	Committed additional libraries:
		d1000.knd lib_ogg
		d1001.knd lib_zip
		d1002.knd lib_regex
		d1003.knd lib_xml
		d1004.knd lib_game
		d1005.knd lib_sql
		d1006.knd lib_math_boost
*/

#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
#if defined(_DEBUG)
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#else
#pragma comment(linker, "/nodefaultlib:libcmtd.lib")
#endif

#pragma comment(lib, "shlwapi.lib")

#define STRICT
#define _WIN32_DCOM
#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCKAPI_

#include <crtdbg.h> // '_ASSERTE'
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Shlwapi.h> // 'PathFileExists'
#include <Windows.h>

#define UNUSED(var) (void)(var)
#define EXPORT _declspec(dllexport)
#define EXPORT_CPP extern "C" _declspec(dllexport)
#define KUIN_MAX_PATH (512)

#define EXCPT_ACCESS_VIOLATION (0xc0000005)

#if defined(_DEBUG)
#define ASSERT(cond) _ASSERTE((cond))
#define STATIC_ASSERT(cond) static_assert((cond), "Static assertion failed.")
#else
#define ASSERT(cond) {}
#define STATIC_ASSERT(cond)
#endif

#define THROW(code) ThrowImpl(code)
#if defined(DBG)
#define THROWDBG(condition, code) if (condition) ThrowImpl(code)
#else
#define THROWDBG(condition, code)
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

static const Bool False = 0;
static const Bool True = 1;

typedef struct SClass
{
	U64 RefCnt;
	void* ClassTable;
} SClass;

static const S64 DefaultRefCntFunc = 0; // Just before exiting the function, this is incremented for 'GcInstance'.
static const S64 DefaultRefCntOpe = 1; // For 'GcInstance'.

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

typedef enum EUseResFlagsKind
{
	UseResFlagsKind_Draw_Circle = 1,
	UseResFlagsKind_Draw_FilterMonotone = 2,
	UseResFlagsKind_Draw_Particle = 3,
	UseResFlagsKind_Draw_ObjDraw = 5,
	UseResFlagsKind_Draw_ObjDrawOutline = 6,
} EUseResFlagsKind;
#define USE_RES_FLAGS_LEN (1)

void InitEnvVars(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
void* AllocMem(size_t size);
void FreeMem(void* ptr);
void ThrowImpl(U32 code);
Bool IsResUsed(EUseResFlagsKind kind);

// Assembly functions.
#ifdef __cplusplus
extern "C" void* ToBinClassAsm(const void* me_);
extern "C" void* FromBinClassAsm(const void* me_, const U8* bin, S64* idx);
extern "C" int CmpClassAsm(const void* me_, const void* target);
extern "C" void DtorClassAsm(void* me_);
extern "C" void* CopyClassAsm(const void* me_);
extern "C" void* Call0Asm(void* func);
extern "C" void* Call1Asm(void* arg1, void* func);
extern "C" void* Call2Asm(void* arg1, void* arg2, void* func);
extern "C" void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);
#else
void* ToBinClassAsm(const void* me_);
void* FromBinClassAsm(const void* me_, const U8 * bin, S64 * idx);
int CmpClassAsm(const void* me_, const void* target);
void DtorClassAsm(void* me_);
void* CopyClassAsm(const void* me_);
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);
#endif
