#pragma once

#include "..\common.h"

EXPORT_CPP void _set2dCallback(void*(*callback)(int, void*, void*));
EXPORT_CPP void _render(S64 fps);
EXPORT_CPP S64 _cnt();
EXPORT_CPP S64 _screenWidth();
EXPORT_CPP S64 _screenHeight();
EXPORT_CPP void _depth(Bool test, Bool write);
EXPORT_CPP void _blend(S64 kind);
EXPORT_CPP void _sampler(S64 kind);
EXPORT_CPP void _clearColor(S64 color);
EXPORT_CPP void _autoClear(Bool enabled);
EXPORT_CPP void _clear();
EXPORT_CPP void _editPixels(const void* callback);
EXPORT_CPP Bool _capture(const U8* path);
EXPORT_CPP void _line(double x1, double y1, double x2, double y2, S64 color);
EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color);
EXPORT_CPP void _rect(double x, double y, double w, double h, S64 color);
EXPORT_CPP void _rectLine(double x, double y, double w, double h, S64 color);
EXPORT_CPP void _circle(double x, double y, double radiusX, double radiusY, S64 color);
EXPORT_CPP void _circleLine(double x, double y, double radiusX, double radiusY, S64 color);
EXPORT_CPP void _poly(const void* x, const void* y, const void* color);
EXPORT_CPP void _polyLine(const void* x, const void* y, const void* color);
EXPORT_CPP void _filterNone();
EXPORT_CPP void _filterMonotone(S64 color, double rate);
EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeTexArgb(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeTexEvenArgb(SClass* me_, double a, double r, double g, double b);
EXPORT_CPP SClass* _makeTexEvenColor(SClass* me_, S64 color);
EXPORT_CPP void _texDtor(SClass* me_);
EXPORT_CPP S64 _texWidth(SClass* me_);
EXPORT_CPP S64 _texHeight(SClass* me_);
EXPORT_CPP S64 _texImgWidth(SClass* me_);
EXPORT_CPP S64 _texImgHeight(SClass* me_);
EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double srcX, double srcY, double srcW, double srcH, S64 color);
EXPORT_CPP void _texDrawScale(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, S64 color);
EXPORT_CPP void _texDrawRot(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, double centerX, double centerY, double angle, S64 color);
EXPORT_CPP SClass* _makeFont(SClass* me_, const U8* fontName, S64 size, bool bold, bool italic, bool proportional, double advance);
EXPORT_CPP void _fontDtor(SClass* me_);
EXPORT_CPP void _fontDraw(SClass* me_, double dstX, double dstY, const U8* text, S64 color);
EXPORT_CPP void _fontDrawScale(SClass* me_, double dstX, double dstY, double dstScaleX, double dstScaleY, const U8* text, S64 color);
EXPORT_CPP double _fontMaxWidth(SClass* me_);
EXPORT_CPP double _fontMaxHeight(SClass* me_);
EXPORT_CPP double _fontCalcWidth(SClass* me_, const U8* text);
EXPORT_CPP void _fontCalcSize(SClass* me_, double* width, double* height, const U8* text);
EXPORT_CPP void _fontSetHeight(SClass* me_, double height);
EXPORT_CPP double _fontGetHeight(SClass* me_);
EXPORT_CPP void _fontAlign(SClass* me_, S64 horizontal, S64 vertical);
EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ);
EXPORT_CPP void _proj(double fovy, double aspectX, double aspectY, double nearZ, double farZ);
EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* path);
EXPORT_CPP void _objDtor(SClass* me_);
EXPORT_CPP SClass* _makeBox(SClass* me_);
EXPORT_CPP SClass* _makeSphere(SClass* me_);
EXPORT_CPP SClass* _makePlane(SClass* me_);
EXPORT_CPP void _objDraw(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal);
EXPORT_CPP void _objDrawToon(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal);
EXPORT_CPP void _objDrawFlat(SClass* me_, S64 element, double frame, SClass* diffuse);
EXPORT_CPP void _objDrawOutline(SClass* me_, S64 element, double frame, double width, S64 color);
EXPORT_CPP void _objDrawWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
EXPORT_CPP void _objDrawToonWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
EXPORT_CPP void _objMat(SClass* me_, const U8* mat, const U8* normMat);
EXPORT_CPP void _objPos(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ);
EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ, Bool fixUp);
EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ, Bool fixUp);
EXPORT_CPP void _ambLight(double topR, double topG, double topB, double bottomR, double bottomG, double bottomB);
EXPORT_CPP void _dirLight(double atX, double atY, double atZ, double r, double g, double b);
EXPORT_CPP S64 _argbToColor(double a, double r, double g, double b);
EXPORT_CPP void _colorToArgb(double* a, double* r, double* g, double* b, S64 color);
EXPORT_CPP void _particleDtor(SClass* me_);
EXPORT_CPP void _particleDraw(SClass* me_, SClass* tex);
EXPORT_CPP void _particleDraw2d(SClass* me_, SClass* tex);
EXPORT_CPP void _particleEmit(SClass* me_, double x, double y, double z, double velo_x, double velo_y, double velo_z, double size, double size_velo, double rot, double rot_velo);
EXPORT_CPP SClass* _makeParticle(SClass* me_, S64 life_span, S64 color1, S64 color2, double friction, double accel_x, double accel_y, double accel_z, double size_accel, double rot_accel);
EXPORT_CPP void _shadowDtor(SClass* me_);
EXPORT_CPP void _shadowBeginRecord(SClass* me_, double x, double y, double z, double radius);
EXPORT_CPP void _shadowEndRecord(SClass* me_);
EXPORT_CPP void _shadowAdd(SClass* me_, SClass* obj, S64 element, double frame);
EXPORT_CPP SClass* _makeShadow(SClass* me_, S64 width, S64 height);

// Assembly functions.
extern "C" void* Call0Asm(void* func);
extern "C" void* Call1Asm(void* arg1, void* func);
extern "C" void* Call2Asm(void* arg1, void* arg2, void* func);
extern "C" void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

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
	void* MakeDrawBuf(int tex_width, int tex_height, int split, HWND wnd, void* old, Bool editable);
	void FinDrawBuf(void* wnd_buf);
	void ActiveDrawBuf(void* wnd_buf);
	void Viewport(double x, double y, double w, double h);
	void ResetViewport();
	void* MakeShaderBuf(EShaderKind kind, size_t size, const void* bin, size_t const_buf_size, int layout_num, const ELayoutType* layout_types, const Char** layout_semantics);
	void FinShaderBuf(void* shader_buf);
	void ConstBuf(void* shader_buf, const void* data);
	void VertexBuf(void* vertex_buf);
	void* MakeVertexBuf(size_t vertex_size, const void* vertices, size_t vertex_line_size, size_t idx_size, const U32* idces);
	void FinVertexBuf(void* vertex_buf);
	void Identity(double mat[4][4]);
	void IdentityFloat(float mat[4][4]);
	double Normalize(double vec[3]);
	double Dot(const double a[3], const double b[3]);
	void Cross(double out[3], const double a[3], const double b[3]);
	void MulMat(double out[4][4], const double a[4][4], const double b[4][4]);
	void SetProjViewMat(float out[4][4], const double proj[4][4], const double view[4][4]);
	HFONT ToFontHandle(SClass* font);
	void ColorToArgb(double* a, double* r, double* g, double* b, S64 color);
	S64 ArgbToColor(double a, double r, double g, double b);
	double Gamma(double value);
	double Degamma(double value);
	U8* AdjustTexSize(U8* argb, int* width, int* height);
	void SetJointMat(const void* element, double frame, float(*joint)[4][4]);
	SClass* MakeTexImpl(SClass* me_, const U8* path, Bool as_argb);
	void Clear();
}
