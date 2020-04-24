#include "file.h"

typedef struct SReader
{
	SClass Class;
	void* Handle;
	S64 DelimiterNum;
	Char* Delimiters;
	S64 FileSize;
} SReader;

typedef struct SWriter
{
	SClass Class;
	FILE* Handle;
} SWriter;

static const U8 Newline[2] = { 0x0d, 0x0a };

static Char ReadUtf8(SReader* me_, Bool replace_delimiter, int* char_cnt);
static void WriteUtf8(SWriter* me_, Char data);
static void NormPath(Char* path, Bool dir);
static void NormPathBackSlash(Char* path, Bool dir);
static Bool ForEachDirRecursion(const Char* path, Bool recursion, void* callback, void* data);
static Bool DelDirRecursion(const Char* path);
static Bool CopyDirRecursion(const Char* dst, const Char* src);
static void SkipLastChars(SReader* reader);

void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

EXPORT SClass* _makeReader(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SReader* me2 = (SReader*)me_;
	me2->Handle = OpenFileStream((Char*)(path + 0x10));
	if (me2->Handle == NULL)
		return NULL;
	me2->DelimiterNum = 3;
	me2->Delimiters = (Char*)AllocMem(sizeof(Char) * 3);
	me2->Delimiters[0] = L'\n';
	me2->Delimiters[1] = L' ';
	me2->Delimiters[2] = L',';
	{
		SeekFileStream(me2->Handle, 0, SEEK_END);
		me2->FileSize = TellFileStream(me2->Handle);
		SeekFileStream(me2->Handle, 0, SEEK_SET);
	}
	return me_;
}

EXPORT SClass* _makeWriter(SClass* me_, const U8* path, Bool append)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SWriter* me2 = (SWriter*)me_;
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), append ? L"ab" : L"wb");
	if (file_ptr == NULL)
		return NULL;
	me2->Handle = file_ptr;
	return me_;
}

EXPORT void _readerDtor(SClass* me_)
{
	SReader* me2 = (SReader*)me_;
	if (me2->Handle != NULL)
		CloseFileStream(me2->Handle);
	if (me2->Delimiters != NULL)
		FreeMem(me2->Delimiters);
}

EXPORT void _readerFin(SClass* me_)
{
	SReader* me2 = (SReader*)me_;
	if (me2->Handle == NULL)
		return;
	CloseFileStream(me2->Handle);
	me2->Handle = NULL;
	if (me2->Delimiters != NULL)
	{
		FreeMem(me2->Delimiters);
		me2->Delimiters = NULL;
	}
}

EXPORT void _readerSetPos(SClass* me_, S64 origin, S64 pos)
{
	THROWDBG(origin < 0 || 2 < origin, EXCPT_DBG_ARG_OUT_DOMAIN);
	SReader* me2 = (SReader*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	if (!SeekFileStream(me2->Handle, pos, origin))
		THROW(EXCPT_INVALID_DATA_FMT);
}

EXPORT S64 _readerGetPos(SClass* me_)
{
	SReader* me2 = (SReader*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	return TellFileStream(me2->Handle);
}

EXPORT void _readerDelimiter(SClass* me_, const U8* delimiters)
{
	SReader* me2 = (SReader*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	THROWDBG(delimiters == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(delimiters + 0x08);
	S64 i;
	const Char* ptr = (const Char*)(delimiters + 0x10);
	FreeMem(me2->Delimiters);
	me2->DelimiterNum = len;
	me2->Delimiters = (Char*)AllocMem(sizeof(Char) * (size_t)len);
	for (i = 0; i < len; i++)
		me2->Delimiters[i] = ptr[i];
}

EXPORT void* _readerRead(SClass* me_, S64 size)
{
	SReader* me2 = (SReader*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	{
		U8* result = (U8*)AllocMem(0x10 + (size_t)size);
		size_t size2;
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = size;
		size2 = ReadFileStream(me2->Handle, (size_t)size, result + 0x10);
		if (size2 != (size_t)size)
		{
			FreeMem(result);
			THROW(EXCPT_INVALID_DATA_FMT);
			return NULL;
		}
		return result;
	}
}

EXPORT Char _readerReadLetter(SClass* me_)
{
	Char result = ReadUtf8((SReader*)me_, False, NULL);
	if (result == WEOF)
		THROW(EXCPT_INVALID_DATA_FMT);
	return result;
}

EXPORT S64 _readerReadInt(SClass* me_)
{
	Char buf[33];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SReader*)me_, True, NULL);
		if (c == L'\r')
			continue;
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				THROW(EXCPT_INVALID_DATA_FMT);
			break;
		}
		if (c == L'\0')
		{
			if (buf[0] == L'\0')
				continue;
			break;
		}
		if (ptr == 32)
			THROW(EXCPT_INVALID_DATA_FMT);
		buf[ptr] = c;
		ptr++;
	}
	SkipLastChars((SReader*)me_);
	buf[ptr] = L'\0';
	{
		S64 result;
		Char* ptr2;
		result = wcstoll(buf, &ptr2, 10);
		if (*ptr2 != L'\0')
			THROW(EXCPT_INVALID_DATA_FMT);
		return result;
	}
}

EXPORT double _readerReadFloat(SClass* me_)
{
	Char buf[33];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SReader*)me_, True, NULL);
		if (c == L'\r')
			continue;
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				THROW(EXCPT_INVALID_DATA_FMT);
			break;
		}
		if (c == L'\0')
		{
			if (buf[0] == L'\0')
				continue;
			break;
		}
		if (ptr == 32)
			THROW(EXCPT_INVALID_DATA_FMT);
		buf[ptr] = c;
		ptr++;
	}
	SkipLastChars((SReader*)me_);
	buf[ptr] = L'\0';
	{
		double result;
		Char* ptr2;
		result = wcstod(buf, &ptr2);
		if (*ptr2 != L'\0')
			THROW(EXCPT_INVALID_DATA_FMT);
		return result;
	}
}

EXPORT Char _readerReadChar(SClass* me_)
{
	Char c = L'\0';
	for (; ; )
	{
		c = ReadUtf8((SReader*)me_, True, NULL);
		if (c == L'\r')
			continue;
		if (c == WEOF)
			THROW(EXCPT_INVALID_DATA_FMT);
		if (c != L'\0')
			break;
	}
	SkipLastChars((SReader*)me_);
	return c;
}

EXPORT void* _readerReadStr(SClass* me_)
{
	Char stack_buf[STACK_STRING_BUF_SIZE];
	Char* buf = stack_buf;
	size_t buf_len = STACK_STRING_BUF_SIZE;
	size_t len = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SReader*)me_, True, NULL);
		if (c == L'\r')
			continue;
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				THROW(EXCPT_INVALID_DATA_FMT);
			break;
		}
		if (c == L'\0')
		{
			if (buf[0] == L'\0')
				continue;
			break;
		}
		buf[len] = c;
		len++;
		if (len == buf_len)
		{
			buf_len += STACK_STRING_BUF_SIZE;
			Char* tmp = (Char*)ReAllocMem(buf == stack_buf ? NULL : buf, sizeof(Char) * buf_len);
			if (tmp == NULL)
			{
				if (buf != stack_buf)
					FreeMem(buf);
				return NULL;
			}
			if (buf == stack_buf)
				memcpy(tmp, buf, sizeof(Char) * len);
			buf = tmp;
		}
	}
	buf[len] = L'\0';
	SkipLastChars((SReader*)me_);
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * ((size_t)len + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)len;
		memcpy(result + 0x10, buf, sizeof(Char) * (len + 1));
		if (buf != stack_buf)
			FreeMem(buf);
		return result;
	}
}

EXPORT void* _readerReadLine(SClass* me_)
{
	Char stack_buf[STACK_STRING_BUF_SIZE];
	Char* buf = stack_buf;
	size_t buf_len = STACK_STRING_BUF_SIZE;
	size_t len = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SReader*)me_, False, NULL);
		if (c == L'\r')
			continue;
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				THROW(EXCPT_INVALID_DATA_FMT);
			break;
		}
		if (c == L'\n')
			break;
		buf[len] = c;
		len++;
		if (len == buf_len)
		{
			buf_len += STACK_STRING_BUF_SIZE;
			Char* tmp = (Char*)ReAllocMem(buf == stack_buf ? NULL : buf, sizeof(Char) * buf_len);
			if (tmp == NULL)
			{
				if (buf != stack_buf)
					FreeMem(buf);
				return NULL;
			}
			if (buf == stack_buf)
				memcpy(tmp, buf, sizeof(Char) * len);
			buf = tmp;
		}
	}
	buf[len] = L'\0';
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * ((size_t)len + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)len;
		memcpy(result + 0x10, buf, sizeof(Char) * (len + 1));
		if (buf != stack_buf)
			FreeMem(buf);
		return result;
	}
}

EXPORT S64 _readerFileSize(SClass* me_)
{
	SReader* me2 = (SReader*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	return me2->FileSize;
}

EXPORT Bool _readerTerm(SClass* me_)
{
	SReader* me2 = (SReader*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	{
		U8 buf;
		Bool result = ReadFileStream(me2->Handle, 1, &buf) == 0;
		if (!result)
			SeekFileStream(me2->Handle, -1, SEEK_CUR);
		return result;
	}
}

EXPORT void _writerDtor(SClass* me_)
{
	SWriter* me2 = (SWriter*)me_;
	if (me2->Handle != NULL)
		fclose(me2->Handle);
}

EXPORT void _writerFin(SClass* me_)
{
	SWriter* me2 = (SWriter*)me_;
	if (me2->Handle == NULL)
		return;
	fclose(me2->Handle);
	me2->Handle = NULL;
}

EXPORT void _writerSetPos(SClass* me_, S64 origin, S64 pos)
{
	THROWDBG(origin < 0 || 2 < origin, EXCPT_DBG_ARG_OUT_DOMAIN);
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	if (_fseeki64(me2->Handle, pos, (int)origin))
		THROW(EXCPT_INVALID_DATA_FMT);
}

EXPORT S64 _writerGetPos(SClass* me_)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	return _ftelli64(me2->Handle);
}

EXPORT void _writerWrite(SClass* me_, void* bin)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	THROWDBG(bin == NULL, EXCPT_ACCESS_VIOLATION);
	{
		U8* bin2 = (U8*)bin;
		fwrite(bin2 + 0x10, 1, (size_t)*(S64*)(bin2 + 0x08), me2->Handle);
	}
}

EXPORT void _writerWriteInt(SClass* me_, S64 n)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	Char str[33];
	int len = swprintf(str, 33, L"%I64d", n);
	int i;
	for (i = 0; i < len; i++)
		WriteUtf8(me2, str[i]);
}

EXPORT void _writerWriteFloat(SClass* me_, double n)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	Char str[33];
	int len = swprintf(str, 33, L"%g", n);
	int i;
	for (i = 0; i < len; i++)
		WriteUtf8(me2, str[i]);
}

EXPORT void _writerWriteChar(SClass* me_, Char n)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	WriteUtf8(me2, n);
}

EXPORT void _writerWriteStr(SClass* me_, const U8* n)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	THROWDBG(n == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* ptr = (const Char*)(n + 0x10);
	while (*ptr != L'\0')
	{
		WriteUtf8(me2, *ptr);
		ptr++;
	}
}

EXPORT void _writerFlush(SClass* me_)
{
	SWriter* me2 = (SWriter*)me_;
	THROWDBG(me2->Handle == NULL, EXCPT_DBG_INOPERABLE_STATE);
	fflush(me2->Handle);
}

EXPORT Bool _makeDir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(S64*)(path + 0x08) > KUIN_MAX_PATH, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (!DelDirRecursion((const Char*)(path + 0x10)))
		return False;
	{
		Char path2[KUIN_MAX_PATH + 1];
		if (GetFullPathName((const Char*)(path + 0x10), KUIN_MAX_PATH, path2, NULL) == 0)
			return False;
		return SHCreateDirectory(NULL, path2) == ERROR_SUCCESS;
	}
}

EXPORT Bool _forEachDir(const U8* path, Bool recursion, void* callback, void* data)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(callback == NULL, EXCPT_ACCESS_VIOLATION);
	return ForEachDirRecursion((const Char*)(path + 0x10), recursion, callback, data);
}

EXPORT Bool _existPath(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	return PathFileExists((const Char*)(path + 0x10)) != 0;
}

EXPORT Bool _delDir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	return DelDirRecursion((const Char*)(path + 0x10)) != 0;
}

EXPORT Bool _delFile(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	if (!PathFileExists((const Char*)(path + 0x10)))
		return True;
	return DeleteFile((const Char*)(path + 0x10)) != 0;
}

EXPORT Bool _copyDir(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	return CopyDirRecursion((const Char*)(dst + 0x10), (const Char*)(src + 0x10)) != 0;
}

EXPORT Bool _copyFile(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	return CopyFile((const Char*)(src + 0x10), (const Char*)(dst + 0x10), FALSE) != 0;
}

EXPORT Bool _moveDir(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	if (MoveFileEx((const Char*)(src + 0x10), (const Char*)(dst + 0x10), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING) == 0)
	{
		if (CopyDirRecursion((const Char*)(dst + 0x10), (const Char*)(src + 0x10)) == 0)
			return False;
		if (DelDirRecursion((const Char*)(src + 0x10)) == 0)
			return False;
	}
	return True;
}

EXPORT Bool _moveFile(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	return MoveFileEx((const Char*)(src + 0x10), (const Char*)(dst + 0x10), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING) != 0;
}

EXPORT void* _dir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* path2 = (const Char*)(path + 0x10);
	size_t len = wcslen(path2);
	U8* result;
	const Char* ptr = path2 + len;
	while (ptr != path2 && *ptr != L'\\' && *ptr != L'/')
		ptr--;
	if (ptr == path2)
	{
		result = (U8*)AllocMem(0x10 + sizeof(Char) * 3);
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = 2;
		wcscpy((Char*)(result + 0x10), L"./");
	}
	else
	{
		size_t len2 = ptr - path2 + 1;
		size_t i;
		Char* str;
		result = (U8*)AllocMem(0x10 + sizeof(Char) * (len2 + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = len2;
		str = (Char*)(result + 0x10);
		for (i = 0; i < len2; i++)
			str[i] = path2[i] == L'\\' ? L'/' : path2[i];
		str[len2] = L'\0';
	}
	return result;
}

EXPORT void* _ext(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* path2 = (const Char*)(path + 0x10);
	size_t len = wcslen(path2);
	U8* result;
	const Char* ptr = path2 + len;
	const Char* ptr2 = ptr;
	while (ptr != path2 && *ptr != L'\\' && *ptr != L'/' && *ptr != L'.')
		ptr--;
	if (ptr == path2 || *ptr != L'.')
	{
		result = (U8*)AllocMem(0x10 + sizeof(Char) * 1);
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = 0;
		*(Char*)(result + 0x10) = L'\0';
	}
	else
	{
		ptr++;
		{
			size_t len2 = ptr2 - ptr;
			size_t i;
			Char* str;
			result = (U8*)AllocMem(0x10 + sizeof(Char) * (len2 + 1));
			*(S64*)(result + 0x00) = DefaultRefCntFunc;
			*(S64*)(result + 0x08) = len2;
			str = (Char*)(result + 0x10);
			for (i = 0; i < len2; i++)
				str[i] = ptr[i];
			str[len2] = L'\0';
		}
	}
	return result;
}

EXPORT void* _fileName(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* path2 = (const Char*)(path + 0x10);
	size_t len = wcslen(path2);
	U8* result;
	const Char* ptr = path2 + len;
	const Char* ptr2 = ptr;
	while (ptr != path2 && *ptr != L'\\' && *ptr != L'/')
		ptr--;
	if (ptr == path2)
		return (void*)path;
	ptr++;
	{
		size_t len2 = ptr2 - ptr;
		size_t i;
		Char* str;
		result = (U8*)AllocMem(0x10 + sizeof(Char) * (len2 + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = len2;
		str = (Char*)(result + 0x10);
		for (i = 0; i < len2; i++)
			str[i] = ptr[i];
		str[len2] = L'\0';
	}
	return result;
}

EXPORT void* _fullPath(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	Char path2[KUIN_MAX_PATH + 2];
	Char* file_name_pos;
	if (GetFullPathName((const Char*)(path + 0x10), KUIN_MAX_PATH, path2, &file_name_pos) == 0)
		return NULL;
	NormPath(path2, file_name_pos == NULL);
	{
		size_t len = wcslen(path2);
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)len;
		wcscpy((Char*)(result + 0x10), path2);
		return result;
	}
}

EXPORT void* _delExt(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* path2 = (const Char*)(path + 0x10);
	size_t len = wcslen(path2);
	U8* result;
	const Char* ptr = path2 + len;
	while (ptr != path2 && *ptr != L'\\' && *ptr != L'/' && *ptr != L'.')
		ptr--;
	if (ptr == path2 || *ptr != L'.')
		return (void*)path;
	{
		size_t len2 = ptr - path2;
		size_t i;
		Char* str;
		result = (U8*)AllocMem(0x10 + sizeof(Char) * (len2 + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = len2;
		str = (Char*)(result + 0x10);
		for (i = 0; i < len2; i++)
			str[i] = path2[i] == L'\\' ? L'/' : path2[i];
		str[len2] = L'\0';
	}
	return result;
}

EXPORT void* _tmpFile(void)
{
	// TODO:
	return NULL;
}

EXPORT void* _sysDir(S64 kind)
{
	Char path[KUIN_MAX_PATH + 2];
	if (!SHGetSpecialFolderPath(NULL, path, (int)kind, TRUE))
		return NULL;
	NormPath(path, True);
	{
		size_t len = wcslen(path);
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)len;
		wcscpy((Char*)(result + 0x10), path);
		return result;
	}
}

EXPORT void* _exeDir(void)
{
	Char path[KUIN_MAX_PATH + 1];
#if defined(DBG)
	wcscpy(path, EnvVars.ResRoot);
#else
	if (!GetModuleFileName(NULL, path, KUIN_MAX_PATH))
		return NULL;
#endif
	{
		size_t len = wcslen(path);
		U8* result;
		const Char* ptr = path + len;
		while (ptr != path && *ptr != L'\\' && *ptr != L'/')
			ptr--;
		if (ptr == path)
			return NULL;
		{
			size_t len2 = ptr - path + 1;
			size_t i;
			Char* str;
			result = (U8*)AllocMem(0x10 + sizeof(Char) * (len2 + 1));
			*(S64*)(result + 0x00) = DefaultRefCntFunc;
			*(S64*)(result + 0x08) = len2;
			str = (Char*)(result + 0x10);
			for (i = 0; i < len2; i++)
				str[i] = path[i] == L'\\' ? L'/' : path[i];
			str[len2] = L'\0';
		}
		return result;
	}
}

EXPORT S64 _fileSize(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	S64 result;
	FILE* file_ptr = _wfopen((const Char*)(path + 0x10), L"rb");
	if (file_ptr == NULL)
		THROW(EXCPT_FILE_READ_FAILED);
	_fseeki64(file_ptr, 0, SEEK_END);
	result = _ftelli64(file_ptr);
	fclose(file_ptr);
	return result;
}

EXPORT void _setCurDir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SetCurrentDirectory((const Char*)(path + 0x10));
}

EXPORT void* _getCurDir(void)
{
	Char path[KUIN_MAX_PATH + 2];
	if (GetCurrentDirectory(KUIN_MAX_PATH + 1, path) == 0)
		return NULL;
	NormPath(path, True);
	{
		size_t len = wcslen(path);
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)len;
		wcscpy((Char*)(result + 0x10), path);
		return result;
	}
}

static Char ReadUtf8(SReader* me_, Bool replace_delimiter, int* char_cnt)
{
	U8 c;
	int len;
	U64 u;
	Char u2;
	for (; ; )
	{
		if (ReadFileStream(me_->Handle, 1, &c) == 0)
		{
			if (char_cnt != NULL)
				*char_cnt = 0;
			return WEOF;
		}
		if ((c & 0xc0) == 0x80)
			continue;
		if ((c & 0x80) == 0x00)
			len = 0;
		else if ((c & 0xe0) == 0xc0)
		{
			len = 1;
			c &= 0x1f;
		}
		else if ((c & 0xf0) == 0xe0)
		{
			len = 2;
			c &= 0x0f;
		}
		else if ((c & 0xf8) == 0xf0)
		{
			len = 3;
			c &= 0x07;
		}
		else if ((c & 0xfc) == 0xf8)
		{
			len = 4;
			c &= 0x03;
		}
		else if ((c & 0xfe) == 0xfc)
		{
			len = 5;
			c &= 0x01;
		}
		else
			continue; // TODO:
		break;
	}
	u = (U64)c;
	{
		int i;
		for (i = 0; i < len; i++)
		{
			if (ReadFileStream(me_->Handle, 1, &c) == 0 || (c & 0xc0) != 0x80)
				THROW(EXCPT_INVALID_DATA_FMT);
			u = (u << 6) | (c & 0x3f);
		}
	}
	if (char_cnt != NULL)
		*char_cnt = 1 + len;
	if (0x00010000 <= u && u <= 0x0010ffff)
		u = 0x20;
	u2 = (Char)u;
	if (!replace_delimiter)
		return u2;
	if (u2 == L'\0')
		return L'\0';
	{
		S64 i;
		for (i = 0; i < me_->DelimiterNum; i++)
		{
			if (u2 == me_->Delimiters[i] || u2 == L'\r' && me_->Delimiters[i] == L'\n')
				return L'\0';
		}
	}
	return u2;
}

static void WriteUtf8(SWriter* me_, Char data)
{
	U64 u;
	size_t size;
	if ((data >> 7) == 0)
	{
		u = data;
		size = 1;
	}
	else
	{
		u = (0x80 | (data & 0x3f)) << 8;
		data >>= 6;
		if ((data >> 5) == 0)
		{
			u |= 0xc0 | data;
			size = 2;
		}
		else
		{
			u = (u | 0x80 | (data & 0x3f)) << 8;
			data >>= 6;
			if ((data >> 4) == 0)
			{
				u |= 0xe0 | data;
				size = 3;
			}
			else
			{
				u = (u | 0x80 | (data & 0x3f)) << 8;
				data >>= 6;
				if ((data >> 3) == 0)
				{
					u |= 0xf0 | data;
					size = 4;
				}
				else
				{
					u = (u | 0x80 | (data & 0x3f)) << 8;
					data >>= 6;
					if ((data >> 2) == 0)
					{
						u |= 0xf8 | data;
						size = 5;
					}
					else
					{
						u = (u | 0x80 | (data & 0x3f)) << 8;
						data >>= 6;
						if ((data >> 1) == 0)
						{
							u |= 0xfc | data;
							size = 6;
						}
						else
							return; // TODO:
					}
				}
			}
		}
	}
	if (size == 1 && u == 0x0a)
		fwrite(&Newline, 1, sizeof(Newline), me_->Handle);
	else
		fwrite(&u, 1, size, me_->Handle);
}

static void NormPath(Char* path, Bool dir)
{
	Char* ptr = path;
	if (*ptr == L'\0')
		return;
	do
	{
		if (*ptr == L'\\')
			*ptr = L'/';
		ptr++;
	} while (*ptr != L'\0');
	if (dir && ptr[-1] != L'/')
	{
		ptr[0] = L'/';
		ptr[1] = L'\0';
	}
}

static void NormPathBackSlash(Char* path, Bool dir)
{
	Char* ptr = path;
	if (*ptr == L'\0')
		return;
	do
	{
		if (*ptr == L'/')
			*ptr = L'\\';
		ptr++;
	} while (*ptr != L'\0');
	if (dir && ptr[-1] != L'\\')
	{
		ptr[0] = L'\\';
		ptr[1] = L'\0';
	}
}

static Bool ForEachDirRecursion(const Char* path, Bool recursion, void* callback, void* data)
{
	Char path2[KUIN_MAX_PATH + 1];
	if (wcslen(path) > KUIN_MAX_PATH)
		return False;
	if (!PathFileExists(path))
		return False;
	{
		size_t len = wcslen(path);
		U8* path3 = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)path3)[0] = 2;
		((S64*)path3)[1] = len;
		memcpy(path3 + 0x10, path, sizeof(Char) * (len + 1));
		if (data != NULL)
			(*(S64*)data)++;
		Bool result = (Bool)(S64)Call3Asm((void*)path3, (void*)(S64)True, data, callback);
		(*(S64*)path3)--;
		if (*(S64*)path3 == 0)
			FreeMem(path3);
		if (!result)
			return False;
	}
	wcscpy(path2, path);
	wcscat(path2, L"*");
	{
		WIN32_FIND_DATA find_data;
		HANDLE handle = FindFirstFile(path2, &find_data);
		if (handle == INVALID_HANDLE_VALUE)
			return False;
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;
			{
				wcscpy(path2, path);
				wcscat(path2, find_data.cFileName);
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					if (recursion)
					{
						wcscat(path2, L"/");
						if (!ForEachDirRecursion(path2, recursion, callback, data))
						{
							FindClose(handle);
							return False;
						}
					}
				}
				else
				{
					size_t len = wcslen(path2);
					U8* path3 = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
					((S64*)path3)[0] = 2;
					((S64*)path3)[1] = len;
					memcpy(path3 + 0x10, path2, sizeof(Char) * (len + 1));
					if (data != NULL)
						(*(S64*)data)++;
					Bool result = (Bool)(S64)Call3Asm((void*)path3, (void*)(S64)False, data, callback);
					(*(S64*)path3)--;
					if (*(S64*)path3 == 0)
						FreeMem(path3);
					if (!result)
						return False;
				}
			}
		} while (FindNextFile(handle, &find_data));
		FindClose(handle);
	}
	return True;
}

static Bool DelDirRecursion(const Char* path)
{
	Char path2[KUIN_MAX_PATH + 1];
	if (wcslen(path) > KUIN_MAX_PATH)
		return False;
	if (!PathFileExists(path))
		return True;
	wcscpy(path2, path);
	wcscat(path2, L"*");
	{
		WIN32_FIND_DATA find_data;
		HANDLE handle = FindFirstFile(path2, &find_data);
		if (handle == INVALID_HANDLE_VALUE)
			return False;
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;
			{
				wcscpy(path2, path);
				wcscat(path2, find_data.cFileName);
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					wcscat(path2, L"/");
					if (!DelDirRecursion(path2))
					{
						FindClose(handle);
						return False;
					}
				}
				else
				{
					if (DeleteFile(path2) == 0)
					{
						FindClose(handle);
						return False;
					}
				}
			}
		} while (FindNextFile(handle, &find_data));
		FindClose(handle);
	}
	return RemoveDirectory(path) != 0;
}

static Bool CopyDirRecursion(const Char* dst, const Char* src)
{
	Char src2[KUIN_MAX_PATH + 1];
	Char dst2[KUIN_MAX_PATH + 1];
	if (wcslen(src) > KUIN_MAX_PATH)
		return False;
	if (!PathFileExists(src))
		return False;
	CreateDirectory(dst, NULL);
	wcscpy(src2, src);
	wcscat(src2, L"*");
	{
		WIN32_FIND_DATA find_data;
		HANDLE handle = FindFirstFile(src2, &find_data);
		if (handle == INVALID_HANDLE_VALUE)
			return False;
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;
			{
				wcscpy(src2, src);
				wcscat(src2, find_data.cFileName);
				wcscpy(dst2, dst);
				wcscat(dst2, find_data.cFileName);
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					wcscat(src2, L"/");
					wcscat(dst2, L"/");
					if (!CopyDirRecursion(dst2, src2))
					{
						FindClose(handle);
						return False;
					}
				}
				else
				{
					if (!CopyFile(src2, dst2, FALSE))
					{
						FindClose(handle);
						return False;
					}
				}
			}
		} while (FindNextFile(handle, &find_data));
		FindClose(handle);
	}
	return True;
}

static void SkipLastChars(SReader* reader)
{
	for (; ; )
	{
		int char_len;
		Char c = ReadUtf8(reader, True, &char_len);
		if (c == L'\r')
			continue;
		if (c == WEOF)
			break;
		if (c != L'\0')
		{
			SeekFileStream(reader->Handle, (S64)-char_len, SEEK_CUR);
			break;
		}
	}
}
