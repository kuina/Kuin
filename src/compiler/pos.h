#pragma once

#include "..\common.h"

typedef struct SPos
{
	const Char* SrcName;
	int Row;
	int Col;
} SPos;

const SPos* NewPos(const Char* src_name, int row, int col);
