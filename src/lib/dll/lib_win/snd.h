#pragma once

#include "..\common.h"

EXPORT_CPP void _sndInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT_CPP void _sndFin();
EXPORT_CPP void _sndFin2(SClass* me_);
EXPORT_CPP void _sndFreq(SClass* me_, double value);
EXPORT_CPP double _sndGetPos(SClass* me_);
EXPORT_CPP double _sndLen(SClass* me_);
EXPORT_CPP void _sndPan(SClass* me_, double value);
EXPORT_CPP void _sndPlay(SClass* me_);
EXPORT_CPP Bool _sndPlaying(SClass* me_);
EXPORT_CPP void _sndPlayLoop(SClass* me_);
EXPORT_CPP void _sndSetPos(SClass* me_, double value);
EXPORT_CPP void _sndStop(SClass* me_);
EXPORT_CPP void _sndVolume(SClass* me_, double value);
EXPORT_CPP double _getMainVolume();
EXPORT_CPP SClass* _makeSnd(SClass* me_, const U8* data, const U8* path);
EXPORT_CPP void _setMainVolume(double value);
