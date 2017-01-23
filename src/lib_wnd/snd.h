#pragma once

#include "..\common.h"

EXPORT_CPP void _setMainVolume(double value);
EXPORT_CPP double _getMainVolume(void);
EXPORT_CPP SClass* _makeSnd(SClass* me_, const U8* path);
EXPORT_CPP void _sndPlay(SClass* me_, double startPos);
EXPORT_CPP void _sndPlayLoop(SClass* me_, double startPos, double loopPos);
EXPORT_CPP void _sndStop(void);
EXPORT_CPP Bool _sndPlaying(void);
EXPORT_CPP void _sndVolume(double value);
EXPORT_CPP void _sndPan(double value);
EXPORT_CPP void _sndFreq(double value);
EXPORT_CPP void _sndSetPos(double pos);
EXPORT_CPP double _sndGetPos(void);
EXPORT_CPP double _sndLen(void);

namespace Snd
{
	void Init();
	void Fin();
}
