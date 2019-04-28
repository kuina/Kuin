#include "cui.h"

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

EXPORT void* _input(void)
{
	Char stack_buf[STACK_STRING_BUF_SIZE];
	Char* buf = stack_buf;
	size_t buf_len = STACK_STRING_BUF_SIZE;
	size_t len = 0;
	for (; ; )
	{
		Char c = fgetwc(stdin);
		if (c == L'\n' || c == WEOF)
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
