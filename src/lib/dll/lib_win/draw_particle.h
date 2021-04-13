#pragma once

#include "..\common.h"
#include "draw_common.h"

EXPORT_CPP void _particleDraw(SClass* me_, SClass* tex);
EXPORT_CPP void _particleDraw2d(SClass* me_, SClass* tex);
EXPORT_CPP void _particleEmit(SClass* me_, double x, double y, double z, double velo_x, double velo_y, double velo_z, double size, double size_velo, double rot, double rot_velo);
EXPORT_CPP void _particleFin(SClass* me_);
EXPORT_CPP SClass* _makeParticle(SClass* me_, S64 life_span, S64 color1, S64 color2, double friction, double accel_x, double accel_y, double accel_z, double size_accel, double rot_accel);
