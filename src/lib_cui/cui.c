#include "cui.h"

#include "main.h"

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
		if (len > 1 && buf[len - 1] == L'\n')
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

EXPORT void* _cmdLine(void)
{
	int num;
	Char** cmds = CommandLineToArgvW(GetCommandLine(), &num);
	ASSERT(num >= 1);
	{
		int i;
		void** ptr;
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char*) * (size_t)(num - 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)(num - 1);
		ptr = (void**)(result + 0x10);
		for (i = 1; i < num; i++)
		{
			size_t len = wcslen(cmds[i]);
			U8* item = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
			((S64*)item)[0] = 1;
			((S64*)item)[1] = (S64)len;
			memcpy(item + 0x10, cmds[i], sizeof(Char) * (len + 1));
			*ptr = item;
			ptr++;
		}
		return result;
	}
}
