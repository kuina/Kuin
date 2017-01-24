#pragma once

#include "..\common.h"

EXPORT_CPP void _setMainVolume(double value);
EXPORT_CPP double _getMainVolume();
EXPORT_CPP SClass* _makeSnd(SClass* me_, const U8* path);
EXPORT_CPP void _sndPlay(SClass* me_, double startPos);
EXPORT_CPP void _sndPlayLoop(SClass* me_, double startPos, double loopPos);
EXPORT_CPP void _sndStop();
EXPORT_CPP Bool _sndPlaying();
EXPORT_CPP void _sndVolume(double value);
EXPORT_CPP void _sndPan(double value);
EXPORT_CPP void _sndFreq(double value);
EXPORT_CPP void _sndSetPos(double pos);
EXPORT_CPP double _sndGetPos();
EXPORT_CPP double _sndLen();

namespace Snd
{
	void Init();
	void Fin();
}
