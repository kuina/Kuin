#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _objDraw(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal);
EXPORT_CPP void _objDrawFlat(SClass* me_, S64 element, double frame, SClass* diffuse);
EXPORT_CPP void _objDrawOutline(SClass* me_, S64 element, double frame, double width, S64 color);
EXPORT_CPP void _objDrawToon(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal);
EXPORT_CPP void _objDrawToonWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
EXPORT_CPP void _objDrawWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
EXPORT_CPP void _objFin(SClass* me_);
EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ, Bool fixUp);
EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ, Bool fixUp);
EXPORT_CPP void _objMat(SClass* me_, const U8* mat, const U8* normMat);
EXPORT_CPP void _objPos(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ);
EXPORT_CPP SClass* _makeBox(SClass* me_);
EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* data);
EXPORT_CPP SClass* _makePlane(SClass* me_);
EXPORT_CPP SClass* _makeSphere(SClass* me_);
