#include "dbg.h"

#if defined(DBG)
static int DbgCode = 0;
#endif

EXPORT void _dbgPrint(const U8* str)
{
#if defined(DBG)
	if (str == NULL)
		OutputDebugString(L"(null)");
	else
	{
		S64 len = ((S64*)str)[1];
		Char* str2 = (Char*)malloc(sizeof(Char) * (5 + len + 1));
		str2[0] = L'd';
		str2[1] = L'b';
		str2[2] = L'g';
		str2[3] = (Char)(L'0' + DbgCode);
		str2[4] = L'!';
		memcpy(&str2[5], str + 0x10, sizeof(Char) * (len + 1));
		OutputDebugString(str2);
		free(str2);
		DbgCode = (DbgCode + 1) % 10;
	}
#else
	UNUSED(str);
#endif
}
