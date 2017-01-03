#include "pos.h"

#include "mem.h"

const SPos* NewPos(const Char* src_name, int row, int col)
{
	SPos* pos = (SPos*)Alloc(sizeof(SPos));
	pos->SrcName = src_name;
	pos->Row = row;
	pos->Col = col;
	return pos;
}
