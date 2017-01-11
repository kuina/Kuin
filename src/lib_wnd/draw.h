#pragma once

#include "..\common.h"

EXPORT_CPP void _flip();
EXPORT_CPP void _clear();
EXPORT_CPP void _viewport(double left, double top, double width, double height);
EXPORT_CPP void _resetViewport();
EXPORT_CPP void _zBuf(S64 zBuf);
EXPORT_CPP void _blend(S64 blend);
EXPORT_CPP void _rect(double left, double top, double width, double height, double r, double g, double b, double a);
EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* path);
EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH);
EXPORT_CPP void _texDrawRot(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, double centerX, double centerY, double angle);
EXPORT_CPP double _texWidth(SClass* me_);
EXPORT_CPP double _texHeight(SClass* me_);
EXPORT_CPP SClass* _makeFont(SClass* me_, const U8* path);
EXPORT_CPP void _fontDraw(SClass* me_, const U8* str, double x, double y, double r, double g, double b, double a);
EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* path);
EXPORT_CPP void _objDraw(SClass* me_, SClass* tex, SClass* normTex, S64 group, double anim);
EXPORT_CPP void _objReset(SClass* me_);
EXPORT_CPP void _objScale(SClass* me_, double x, double y, double z);
EXPORT_CPP void _objRotX(SClass* me_, double angle);
EXPORT_CPP void _objRotY(SClass* me_, double angle);
EXPORT_CPP void _objRotZ(SClass* me_, double angle);
EXPORT_CPP void _objTrans(SClass* me_, double x, double y, double z);
EXPORT_CPP void _objSrt(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ);
EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ);
EXPORT_CPP void _objLookFix(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ);
EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ);
EXPORT_CPP void _objLookCameraFix(SClass* me_, double x, double y, double z, double upX, double upY, double upZ);
EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ);
EXPORT_CPP void _proj(double fovy, double minZ, double maxZ);
EXPORT_CPP void _ambLight(double topR, double topG, double topB, double bottomR, double bottomG, double bottomB);
EXPORT_CPP void _dirLight(double atX, double atY, double atZ, double r, double g, double b);
void InitDraw();
void FinDraw();
void* MakeWndBuf(int width, int height, HWND wnd);
void FinWndBuf(void* wnd_buf);
