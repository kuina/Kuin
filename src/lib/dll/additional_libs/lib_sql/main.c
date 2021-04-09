// LibSql.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "sqlite3.h"

typedef struct SSql
{
	SClass Class;
	sqlite3* Db;
	sqlite3_stmt* Statement;
	int Result;
} SSql;

static void Reset(SSql* me);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);
}

EXPORT Bool _sqlApply(SClass* me_)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);

	if (sqlite3_step(me2->Statement) != SQLITE_DONE)
	{
		Reset(me2);
		return False;
	}
	return True;
}

EXPORT void* _sqlErrMsg(SClass* me_)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	const Char* str = sqlite3_errmsg16(me2->Db);
	size_t len = wcslen(str);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)len;
	memcpy(result + 0x10, str, sizeof(Char) * (len + 1));
	return result;
}

EXPORT Bool _sqlExec(SClass* me_, const void* cmd)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	Bool success = False;
	for (; ; )
	{
		Reset(me2);
		if (sqlite3_prepare16(me2->Db, (const U8*)cmd + 0x10, (int)(*(const S64*)((const U8*)cmd + 0x08) * sizeof(Char)), &me2->Statement, NULL) != SQLITE_OK)
			break;
		int result = sqlite3_step(me2->Statement);
		if (result != SQLITE_DONE && result != SQLITE_ROW)
			break;
		me2->Result = result;

		success = True;
		break;
	}
	if (!success)
	{
		Reset(me2);
		return False;
	}
	return True;
}

EXPORT void _sqlFin(SClass* me_)
{
	SSql* me2 = (SSql*)me_;
	if (me2->Statement != NULL)
	{
		sqlite3_finalize(me2->Statement);
		me2->Statement = NULL;
	}
	if (me2->Db != NULL)
	{
		sqlite3_close(me2->Db);
		me2->Db = NULL;
	}
}

EXPORT void* _sqlGetBlob(SClass* me_, S64 col)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	size_t len = sqlite3_column_bytes(me2->Statement, (int)col);
	const S8* blob = sqlite3_column_blob(me2->Statement, (int)col);
	if (blob == NULL)
		return NULL;
	U8* result = (U8*)AllocMem(0x10 + sizeof(S8) * len);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)len;
	memcpy(result + 0x10, blob, sizeof(S8) * len);
	return result;
}

EXPORT double _sqlGetFloat(SClass* me_, S64 col)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	return sqlite3_column_double(me2->Statement, (int)col);
}

EXPORT S64 _sqlGetInt(SClass* me_, S64 col)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	return sqlite3_column_int64(me2->Statement, (int)col);
}

EXPORT void* _sqlGetStr(SClass* me_, S64 col)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	const Char* str = sqlite3_column_text16(me2->Statement, (int)col);
	if (str == NULL)
		return NULL;
	size_t len = wcslen(str);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)len;
	memcpy(result + 0x10, str, sizeof(Char) * (len + 1));
	return result;
}

EXPORT void _sqlNext(SClass* me_)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	sqlite3_step(me2->Statement);
}

EXPORT Bool _sqlPrepare(SClass* me_, const void* cmd)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	Reset(me2);
	int result = sqlite3_prepare16_v2(me2->Db, (const U8*)cmd + 0x10, (int)(*(const S64*)((const U8*)cmd + 0x08) * sizeof(Char)), &me2->Statement, NULL);
	if (result == SQLITE_OK)
	{
		me2->Result = result;
		return True;
	}

	Reset(me2);
	return False;
}

EXPORT Bool _sqlSetBlob(SClass* me_, S64 idx, const void* value)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	int result = sqlite3_bind_blob(me2->Statement, (int)idx + 1, (const U8*)value + 0x10, (int)(*(const S64*)((const U8*)value + 0x08) * sizeof(S8)), SQLITE_TRANSIENT);
	return result == SQLITE_OK;
}

EXPORT Bool _sqlSetFloat(SClass* me_, S64 idx, double value)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	int result = sqlite3_bind_double(me2->Statement, (int)idx + 1, value);
	return result == SQLITE_OK;
}

EXPORT Bool _sqlSetInt(SClass* me_, S64 idx, S64 value)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	int result = sqlite3_bind_int64(me2->Statement, (int)idx + 1, value);
	return result == SQLITE_OK;
}

EXPORT Bool _sqlSetStr(SClass* me_, S64 idx, const void* value)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	int result = sqlite3_bind_text16(me2->Statement, (int)idx + 1, (const U8*)value + 0x10, (int)(*(const S64*)((const U8*)value + 0x08) * sizeof(Char)), SQLITE_TRANSIENT);
	return result == SQLITE_OK;
}

EXPORT Bool _sqlTerm(SClass* me_)
{
	SSql* me2 = (SSql*)me_;
	THROWDBG(me2->Db == NULL, 0xe917000a);
	return me2->Result != SQLITE_ROW;
}

EXPORT SClass* _makeSql(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SSql* me2 = (SSql*)me_;
	me2->Db = NULL;
	me2->Statement = NULL;
	me2->Result = SQLITE_DONE;
	if (sqlite3_open16((const Char*)(path + 0x10), &me2->Db) != SQLITE_OK)
		return NULL;
	return me_;
}

static void Reset(SSql* me)
{
	if (me->Statement != NULL)
	{
		sqlite3_finalize(me->Statement);
		me->Statement = NULL;
	}
	me->Result = SQLITE_DONE;
}
