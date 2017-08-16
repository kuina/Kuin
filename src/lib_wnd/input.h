#pragma once

#include "..\common.h"

EXPORT_CPP S64 _pad(S64 idx, S64 btn);
EXPORT_CPP void _setCfg(S64 idx, S64 btn, S64 newBtn);
EXPORT_CPP S64 _getCfg(S64 idx, S64 btn);
EXPORT_CPP void _lockCfg(Bool enabled);
EXPORT_CPP void _setCfgKey(S64 idx, S64 btn, const U8* keys);
EXPORT_CPP Bool _key(S64 key);

namespace Input
{
	void Init();
	void Fin();
	void Update();
}
