#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _shadowAdd(SClass* me_, SClass* obj, S64 element, double frame);
EXPORT_CPP void _shadowBeginRecord(SClass* me_, double x, double y, double z, double radius);
EXPORT_CPP void _shadowEndRecord(SClass* me_);
EXPORT_CPP void _shadowFin(SClass* me_);
EXPORT_CPP SClass* _makeShadow(SClass* me_, S64 width, S64 height);
