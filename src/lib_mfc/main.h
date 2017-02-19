#pragma once

#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN
#endif
#include <SDKDDKVer.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <afxwin.h>
#include <afxext.h>
#ifndef _AFX_NO_OLE_SUPPORT
	#include <afxole.h>
	#include <afxodlgs.h>
	#include <afxdisp.h>
#endif
#ifndef _AFX_NO_DB_SUPPORT
	#include <afxdb.h>
#endif
#ifndef _AFX_NO_DAO_SUPPORT
	#include <afxdao.h>
#endif
#ifndef _AFX_NO_OLE_SUPPORT
	#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
	#include <afxcmn.h>
#endif

#define EXPORT_CPP extern "C" _declspec(dllexport)

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

EXPORT_CPP void _init();
EXPORT_CPP void _fin();
EXPORT_CPP void _dummy();
EXPORT_CPP Bool _act();
EXPORT_CPP void* _makeWnd();
EXPORT_CPP HWND _getHwnd(void* ptr);

class CKuinBackground : public CFrameWnd
{
public:
	DECLARE_MESSAGE_MAP()
};

class CKuinWnd : public CFrameWnd
{
public:
	virtual void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

class Clib_mfcApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
};
