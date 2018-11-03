#include "main.h"
#include <d2d1.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")

// 下記の警告を無効化(描画ライブラリでfloatを使うのは当然で、Kuinではdoubleを扱うため)
// warning C4244: '引数': 'double' から 'FLOAT' への変換です。データが失われる可能性があります。
#pragma warning( disable : 4244 )

static ID2D1Factory* factory = NULL;

static inline D2D1::ColorF colorFromS64(S64 color);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	HRESULT res;
	UNUSED(hinst);
	UNUSED(reserved);

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		res = D2D1CreateFactory(
			D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&factory);
		if (res != S_OK) {
			THROW(0x12345678);
		}
		break;
	case DLL_PROCESS_DETACH:
		if (factory) factory->Release();
		break;
	}
	return TRUE;
}

struct SDeviceContext
{
	SClass Class;
	ID2D1RenderTarget* renderTarget;
};

struct SWindowDeviceContext
{
	SDeviceContext parent;
	ID2D1HwndRenderTarget* renderTarget;
};

struct SGdiDeviceContext
{
	SDeviceContext parent;
	ID2D1DCRenderTarget* renderTarget;
	HWND hwnd;
};

EXPORT_CPP SClass* _makeWndDC(SClass* me_, S64 hwnd)
{
	SWindowDeviceContext* me2 = reinterpret_cast<SWindowDeviceContext*>(me_);
	HWND hwnd2 = (HWND)hwnd;

	RECT rect;
	GetClientRect(hwnd2, &rect);

	HRESULT hr = factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			hwnd2, D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top)),
		&me2->renderTarget);
	if (hr != S_OK) THROW(0x22345678);

	hr = me2->renderTarget->QueryInterface(
		__uuidof(ID2D1RenderTarget), (LPVOID*)&me2->parent.renderTarget);
	if (hr != S_OK) THROW(0x22345678);

	return me_;
}

EXPORT_CPP SClass* _makeGdiDC(SClass* me_, S64 hwnd)
{
	SGdiDeviceContext* me2 = reinterpret_cast<SGdiDeviceContext*>(me_);
	me2->hwnd = (HWND)hwnd;

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_PREMULTIPLIED),
		0,
		0,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT);

	HRESULT hr = factory->CreateDCRenderTarget(&props, &me2->renderTarget);
	if (hr != S_OK) THROW(0x32345678);

	hr = me2->renderTarget->QueryInterface(
		__uuidof(ID2D1RenderTarget), (LPVOID*)&me2->parent.renderTarget);
	if (hr != S_OK) THROW(0x32345678);

	return me_;
}

// TODO: Direct3D 10 -> Direct2D への変換

EXPORT_CPP void _wndDCDtor(SClass* me_)
{
	SWindowDeviceContext* me2 = reinterpret_cast<SWindowDeviceContext*>(me_);
	me2->parent.renderTarget->Release();
	me2->renderTarget->Release();
}

EXPORT_CPP void _gdiDCDtor(SClass* me_)
{
	SGdiDeviceContext* me2 = reinterpret_cast<SGdiDeviceContext*>(me_);
	me2->parent.renderTarget->Release();
	me2->renderTarget->Release();
}

EXPORT_CPP void _dcBegin(SClass* me_)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	me2->renderTarget->BeginDraw();
}

EXPORT_CPP void _gdiDCBegin(SClass* me_)
{
	SGdiDeviceContext* me2 = reinterpret_cast<SGdiDeviceContext*>(me_);
	RECT rc;
	GetClientRect(me2->hwnd, &rc);
	me2->renderTarget->BindDC(GetDC(me2->hwnd), &rc);
	me2->renderTarget->BeginDraw();
}

EXPORT_CPP void _dcEnd(SClass* me_)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	me2->renderTarget->EndDraw();
}

EXPORT_CPP void _line(
	SClass* me_, double x1, double y1, double x2, double y2, double strokeWidth, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush;
	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	me2->renderTarget->DrawLine(
		D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth, NULL);
	brush->Release();
}

EXPORT_CPP void _rect(
	SClass* me_, double x, double y, double w, double h, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush;
	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	me2->renderTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush);
	brush->Release();
}

EXPORT_CPP void _rectLine(
	SClass* me_, double x, double y, double w, double h, double strokeWidth, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush;
	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	me2->renderTarget->DrawRectangle(
		D2D1::RectF(x, y, x + w, y + h), brush, strokeWidth, NULL);
	brush->Release();
}

EXPORT_CPP void _circle(
	SClass* me_, double x, double y, double rx, double ry, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush;
	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	me2->renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush);
	brush->Release();
}

EXPORT_CPP void _circleLine(
	SClass* me_, double x, double y, double rx, double ry, double strokeWidth, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush;
	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	me2->renderTarget->DrawEllipse(
		D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush, strokeWidth, NULL);
	brush->Release();
}

EXPORT_CPP void _tri(
	SClass* me_, double x1, double y1, double x2, double y2, double x3, double y3, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush = NULL;
	ID2D1PathGeometry *geometry = NULL;
	ID2D1GeometrySink *sink = NULL;

	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	factory->CreatePathGeometry(&geometry);
	geometry->Open(&sink);
	sink->BeginFigure(D2D1::Point2F(x1, y1), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F(x2, y2));
	sink->AddLine(D2D1::Point2F(x3, y3));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();

	me2->renderTarget->FillGeometry(geometry, brush);

	brush->Release();
	geometry->Release();
	sink->Release();
}

static inline D2D1::ColorF colorFromS64(S64 color) {
	union {
		S64 i;
		struct { S32 argb; S32 rest; } rgb;
		struct { S16 _1; S8 _2; U8 a; S32 _3; } a;
	} u;
	u.i = color;
	return D2D1::ColorF(u.rgb.argb, u.a.a / 255.0f);
}