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
	Char buf[INPUT_SIZE];
	if (fgetws(buf, INPUT_SIZE, stdin) == NULL)
		buf[0] = L'\0';
	{
		size_t len = wcslen(buf);
		U8* result;
		if (len > 0 && buf[len - 1] == L'\n')
		{
			buf[len - 1] = L'\0';
			len--;
		}
		result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		memcpy(result + 0x10, buf, sizeof(Char) * (len + 1));
		return result;
	}
}
