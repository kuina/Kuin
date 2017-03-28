#include "file.h"

typedef struct SStream
{
	SClass Class;
	FILE* Handle;
	S64 DelimiterNum;
	Char* Delimiters;
} SStream;

static const U8 Newline[2] = { 0x0d, 0x0a };

static Char ReadUtf8(SStream* me_, Bool replace_delimiter);
static void WriteUtf8(SStream* me_, Char data);

EXPORT SClass* _makeReader(SClass* me_, const U8* path)
{
	SStream* me2 = (SStream*)me_;
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), L"rb");
	if (file_ptr == NULL)
		return NULL;
	me2->Handle = file_ptr;
	me2->DelimiterNum = 2;
	me2->Delimiters = (Char*)AllocMem(sizeof(Char) * 2);
	me2->Delimiters[0] = L' ';
	me2->Delimiters[1] = L',';
	return me_;
}

EXPORT SClass* _makeWriter(SClass* me_, const U8* path, Bool append)
{
	SStream* me2 = (SStream*)me_;
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), append ? L"ab" : L"wb");
	if (file_ptr == NULL)
		return NULL;
	me2->Handle = file_ptr;
	me2->DelimiterNum = 0;
	me2->Delimiters = NULL;
	return me_;
}

EXPORT void _streamDtor(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
	if (me2->Handle != NULL)
		fclose(me2->Handle);
	if (me2->Delimiters != NULL)
		FreeMem(me2->Delimiters);
}

EXPORT void _streamFin(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	fclose(me2->Handle);
	me2->Handle = NULL;
	if (me2->Delimiters != NULL)
	{
		FreeMem(me2->Delimiters);
		me2->Delimiters = NULL;
	}
}

EXPORT void _streamSetPos(SClass* me_, S64 origin, S64 pos)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
	ASSERT(0 <= origin && origin <= 2);
#endif
	if (_fseeki64(me2->Handle, pos, (int)origin))
		THROW(0x1000, L"");
}

EXPORT S64 _streamGetPos(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	return _ftelli64(me2->Handle);
}

EXPORT void _streamDelimiter(SClass* me_, const U8* delimiters)
{
	SStream* me2 = (SStream*)me_;
	S64 len = *(S64*)(delimiters + 0x08);
	S64 i;
	const Char* ptr = (const Char*)(delimiters + 0x10);
	FreeMem(me2->Delimiters);
	me2->DelimiterNum = len;
	me2->Delimiters = (Char*)AllocMem(sizeof(Char) * (size_t)len);
	for (i = 0; i < len; i++)
		me2->Delimiters[i] = ptr[i];
}

EXPORT void* _streamRead(SClass* me_, S64 size)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		U8* result = (U8*)AllocMem(0x10 + (size_t)size);
		size_t size2;
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = size;
		size2 = fread(result + 0x10, 1, (size_t)size, me2->Handle);
		if (size2 != (size_t)size)
		{
			FreeMem(result);
			return NULL;
		}
		return result;
	}
}

EXPORT Char _streamReadLetter(SClass* me_)
{
	return ReadUtf8((SStream*)me_, False);
}

EXPORT S64 _streamReadInt(SClass* me_)
{
	Char buf[33];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				ASSERT(False);
			break;
		}
		if (c == L'\0')
		{
			if (buf[0] == L'\0')
				continue;
			break;
		}
		if (ptr == 32)
			ASSERT(False);
		buf[ptr] = c;
		ptr++;
	}
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
			break;
		if (c != L'\0')
		{
			_fseeki64(((SStream*)me_)->Handle, -1, SEEK_CUR);
			break;
		}
	}
	buf[ptr] = L'\0';
	{
		S64 result;
		errno = 0;
		result = _wtoi64(buf);
		if (errno != 0)
			THROW(0x1000, L"");
		return result;
	}
}

EXPORT double _streamReadFloat(SClass* me_)
{
	Char buf[33];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				ASSERT(False);
			break;
		}
		if (c == L'\0')
		{
			if (buf[0] == L'\0')
				continue;
			break;
		}
		if (ptr == 32)
			ASSERT(False);
		buf[ptr] = c;
		ptr++;
	}
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
			break;
		if (c != L'\0')
		{
			_fseeki64(((SStream*)me_)->Handle, -1, SEEK_CUR);
			break;
		}
	}
	buf[ptr] = L'\0';
	{
		double result;
		errno = 0;
		result = _wtof(buf);
		if (errno != 0)
			THROW(0x1000, L"");
		return result;
	}
}

EXPORT Char _streamReadChar(SClass* me_)
{
	Char c = L'\0';
	for (; ; )
	{
		c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
			ASSERT(False);
		if (c != L'\0')
			break;
	}
	for (; ; )
	{
		Char c2 = ReadUtf8((SStream*)me_, True);
		if (c2 == WEOF)
			break;
		if (c2 != L'\0')
		{
			_fseeki64(((SStream*)me_)->Handle, -1, SEEK_CUR);
			break;
		}
	}
	return c;
}

EXPORT void* _streamReadStr(SClass* me_)
{
	Char buf[1025];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
		{
			if (buf[0] == L'\0')
				ASSERT(False);
			break;
		}
		if (c == L'\0')
		{
			if (buf[0] == L'\0')
				continue;
			break;
		}
		buf[ptr] = c;
		ptr++;
		if (ptr == 1024)
			break;
	}
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, True);
		if (c == WEOF)
			break;
		if (c != L'\0')
		{
			_fseeki64(((SStream*)me_)->Handle, -1, SEEK_CUR);
			break;
		}
	}
	buf[ptr] = L'\0';
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * ((size_t)ptr + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)ptr;
		wcscpy((Char*)(result + 0x10), buf);
		return result;
	}
}

EXPORT void* _streamReadLine(SClass* me_)
{
	Char buf[1025];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadUtf8((SStream*)me_, False);
		if (c == L'\r')
			continue;
		if (c == WEOF || c == L'\n')
			break;
		buf[ptr] = c;
		ptr++;
		if (ptr == 1024)
			break;
	}
	buf[ptr] = L'\0';
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * ((size_t)ptr + 1));
		*(S64*)(result + 0x00) = DefaultRefCntFunc;
		*(S64*)(result + 0x08) = (S64)ptr;
		wcscpy((Char*)(result + 0x10), buf);
		return result;
	}
}

EXPORT void _streamWrite(SClass* me_, void* bin)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		U8* bin2 = (U8*)bin;
		fwrite(bin2 + 0x10, 1, (size_t)*(S64*)(bin2 + 0x08), me2->Handle);
	}
}

EXPORT void _streamWriteInt(SClass* me_, S64 n)
{
	Char str[33];
	int len = swprintf(str, 33, L"%I64d", n);
	int i;
	for (i = 0; i < len; i++)
		WriteUtf8((SStream*)me_, str[i]);
}

EXPORT void _streamWriteFloat(SClass* me_, double n)
{
	Char str[33];
	int len = swprintf(str, 33, L"%g", n);
	int i;
	for (i = 0; i < len; i++)
		WriteUtf8((SStream*)me_, str[i]);
}

EXPORT void _streamWriteChar(SClass* me_, Char n)
{
	WriteUtf8((SStream*)me_, n);
}

EXPORT void _streamWriteStr(SClass* me_, const U8* n)
{
	const Char* ptr = (const Char*)(n + 0x10);
	while (*ptr != L'\0')
	{
		WriteUtf8((SStream*)me_, *ptr);
		ptr++;
	}
}

EXPORT S64 _streamFileSize(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		S64 current = _ftelli64(me2->Handle);
		S64 result;
		_fseeki64(me2->Handle, 0, SEEK_END);
		result = _ftelli64(me2->Handle);
		_fseeki64(me2->Handle, current, SEEK_SET);
		return result;
	}
}

EXPORT Bool _streamTerm(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		Bool result = fgetc(me2->Handle) == EOF;
		if (!result)
			_fseeki64(me2->Handle, -1, SEEK_CUR);
		return result;
	}
}

EXPORT Bool _makeDir(const U8* path, Bool clear)
{
	// TODO:
	return False;
}

EXPORT void _foreachDir(const U8* path, Bool recursive, void* func)
{
	// TODO:
}

EXPORT Bool _exist(const U8* path)
{
	// TODO: '\\' or '/'.
	return PathFileExists((const Char*)(path + 0x10)) != 0;
}

EXPORT Bool _delFile(const U8* path)
{
	// TODO:
	return False;
}

EXPORT Bool _copyFile(const U8* dst, const U8* src, Bool overwrite)
{
	// TODO:
	return False;
}

EXPORT Bool _moveFile(const U8* dst, const U8* src, Bool overwrite)
{
	// TODO:
	return False;
}

EXPORT const U8* _ext(const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT const U8* _fileName(const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT const U8* _fullPath(const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT const U8* _delExt(const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT const U8* _tmpFile(void)
{
	// TODO:
	return NULL;
}

EXPORT S64 _fileSize(const U8* path)
{
	// TODO: Test.
	S64 result;
	FILE* file_ptr = _wfopen((const Char*)(path + 0x10), L"rb");
	if (file_ptr == NULL)
		THROW(0x1000, L"");
	_fseeki64(file_ptr, 0, SEEK_END);
	result = _ftelli64(file_ptr);
	fclose(file_ptr);
	return result;
}

static Char ReadUtf8(SStream* me_, Bool replace_delimiter)
{
	U8 c;
	int len;
	U64 u;
	Char u2;
	for (; ; )
	{
		int c2 = fgetc(me_->Handle);
		if (c2 == EOF)
			return WEOF;
		c = (U8)c2;
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
			if (fread(&c, 1, 1, me_->Handle) != 1 || (c & 0xc0) != 0x80)
				ASSERT(False);
			u = (u << 6) | (c & 0x3f);
		}
	}
	if (0x00010000 <= u && u <= 0x0010ffff)
		u = 0x20;
	u2 = (Char)u;
	if (!replace_delimiter)
		return u2;
	if (u2 == L'\0' || u2 == L'\r' || u2 == L'\n')
		return L'\0';
	{
		S64 i;
		for (i = 0; i < me_->DelimiterNum; i++)
		{
			if (u2 == me_->Delimiters[i])
				return L'\0';
		}
	}
	return u2;
}

static void WriteUtf8(SStream* me_, Char data)
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
