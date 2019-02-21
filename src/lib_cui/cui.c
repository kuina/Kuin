#include "cui.h"

#define INPUT_SIZE (1024)

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
	Char *buf = NULL;
	size_t len = 0;
	size_t pos = 0;
	for (; ; )
	{
		if (pos == len)
		{
			Char *tmp;
			len += INPUT_SIZE;
			tmp = (Char *)realloc(buf, sizeof(Char) * len);
			if (tmp)
			{
				buf = tmp;
			}
			else
			{
				// TODO: Add exception.
				free(buf);
				return NULL;
			}
		}
		{
			Char c = fgetwc(stdin);
			if (c == L'\n' || c == WEOF)
				break;
			buf[pos] = c;
		}
		pos++;
	}
	buf[pos] = L'\0';
	len = pos + 1;
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		memcpy(result + 0x10, buf, sizeof(Char) * (len + 1));
		free(buf);
		return result;
	}
}
