#include "cui.h"

static S64 DelimiterNum;
static Char* Delimiters;

static Char ReadIo(void);

EXPORT void _delimiter(const U8* delimiters)
{
	THROWDBG(delimiters == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(delimiters + 0x08);
	S64 i;
	const Char* ptr = (const Char*)(delimiters + 0x10);
	FreeMem(Delimiters);
	DelimiterNum = len;
	Delimiters = (Char*)AllocMem(sizeof(Char) * (size_t)len);
	for (i = 0; i < len; i++)
		Delimiters[i] = ptr[i];
}

EXPORT void _print(const U8* str)
{
	const Char* str2;
	if (str == NULL)
		str2 = L"(null)";
	else
		str2 = (const Char*)(str + 0x10);
	fputws(str2, stdout); // '_putws' is not used because it carries a new line at the end.
#if defined(_DEBUG)
	OutputDebugString(str2);
#endif
}

EXPORT void _flush(void)
{
	fflush(stdout);
}

EXPORT Char _inputLetter(void)
{
	Char c = fgetwc(stdin);
	if (c == WEOF)
		THROW(EXCPT_INVALID_DATA_FMT);
	return c;
}

EXPORT S64 _inputInt(void)
{
	Char buf[33];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadIo();
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

EXPORT double _inputFloat(void)
{
	Char buf[33];
	int ptr = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadIo();
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

EXPORT Char _inputChar(void)
{
	Char c = L'\0';
	for (; ; )
	{
		c = ReadIo();
		if (c == L'\r')
			continue;
		if (c == WEOF)
			THROW(EXCPT_INVALID_DATA_FMT);
		if (c != L'\0')
			break;
	}
	return c;
}

EXPORT void* _inputStr(void)
{
	Char stack_buf[STACK_STRING_BUF_SIZE];
	Char* buf = stack_buf;
	size_t buf_len = STACK_STRING_BUF_SIZE;
	size_t len = 0;
	buf[0] = L'\0';
	for (; ; )
	{
		Char c = ReadIo();
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

EXPORT void* _input(void)
{
	Char stack_buf[STACK_STRING_BUF_SIZE];
	Char* buf = stack_buf;
	size_t buf_len = STACK_STRING_BUF_SIZE;
	size_t len = 0;
	for (; ; )
	{
		Char c = fgetwc(stdin);
		if (c == L'\r')
			continue;
		if (c == WEOF)
			break;
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
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		memcpy(result + 0x10, buf, sizeof(Char) * (len + 1));
		if (buf != stack_buf)
			FreeMem(buf);
		return result;
	}
}

void InitCui(void)
{
	DelimiterNum = 3;
	Delimiters = (Char*)AllocMem(sizeof(Char) * 3);
	Delimiters[0] = L'\n';
	Delimiters[1] = L' ';
	Delimiters[2] = L',';
}

void FinCui(void)
{
	FreeMem(Delimiters);
#if defined(DBG)
	system("pause");
#endif
}

static Char ReadIo(void)
{
	Char c = fgetwc(stdin);
	if (c == L'\0')
		return L'\0';
	{
		S64 i;
		for (i = 0; i < DelimiterNum; i++)
		{
			if (c == Delimiters[i] || c == L'\r' && Delimiters[i] == L'\n')
				return L'\0';
		}
	}
	return c;
}
