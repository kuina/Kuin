#include "dbg.h"

EXPORT void _dbgPrint(const U8* str)
{
#if defined(DBG)
	if (str == NULL)
		OutputDebugString(L"dbg!(null)");
	else
	{
		S64 len = ((S64*)str)[1];
		Char* str2 = (Char*)malloc(sizeof(Char) * (4 + len + 1));
		str2[0] = L'd';
		str2[1] = L'b';
		str2[2] = L'g';
		str2[3] = L'!';
		memcpy(&str2[4], str + 0x10, sizeof(Char) * (len + 1));
		OutputDebugString(str2);
		free(str2);
	}
#else
	UNUSED(str);
#endif
}
