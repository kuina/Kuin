#include "dbg.h"

EXPORT void _dbgPrint(const U8* str)
{
	OutputDebugString(str == NULL ? L"(null)" : (const Char*)(str + 0x10));
}
