#pragma once

#include "..\common.h"

EXPORT_CPP void _render();
EXPORT_CPP void _viewport(double x, double y, double w, double h);
EXPORT_CPP void _resetViewport();
EXPORT_CPP void _depth(Bool test, Bool write);
EXPORT_CPP void _blend(S64 kind);
EXPORT_CPP void _sampler(S64 kind);
EXPORT_CPP void _clearColor(double r, double g, double b);
EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, double r, double g, double b, double a);
EXPORT_CPP void _rect(double x, double y, double w, double h, double r, double g, double b, double a);
EXPORT_CPP void _circle(double x, double y, double radiusX, double radiusY, double r, double g, double b, double a);
EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* path);
EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double srcX, double srcY, double srcW, double srcH);
EXPORT_CPP void _texDrawScale(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH);
EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeBox(SClass* me_, double w, double h, double d, double r, double g, double b, double a);

/*
EXPORT_CPP void _viewport(double left, double top, double width, double height);
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
*/

namespace Draw
{
	enum EShaderKind
	{
		ShaderKind_Vs,
		ShaderKind_Gs,
		ShaderKind_Ps,
	};

	enum ELayoutType
	{
		LayoutType_Int1,
		LayoutType_Int2,
		LayoutType_Int4,
		LayoutType_Float1,
		LayoutType_Float2,
		LayoutType_Float3,
		LayoutType_Float4,
	};

	void Init();
	void Fin();
	void* MakeWndBuf(int width, int height, HWND wnd);
	void FinWndBuf(void* wnd_buf);
	void* MakeShaderBuf(EShaderKind kind, size_t size, const void* bin, size_t const_buf_size, int layout_num, const ELayoutType* layout_types, const Char** layout_semantics);
	void FinShaderBuf(void* shader_buf);
	void ConstBuf(void* shader_buf, const void* data);
	void VertexBuf(void* vertex_buf);
	void* MakeVertexBuf(size_t vertex_size, const void* vertices, size_t vertex_line_size, size_t idx_size, const U16* idces);
	void FinVertexBuf(void* vertex_buf);
}
