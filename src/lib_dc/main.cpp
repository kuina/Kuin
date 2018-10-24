#include "main.h"
#include <d2d1.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")

static ID2D1Factory* factory = NULL;

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
	ID2D1HwndRenderTarget* renderTarget;
};

EXPORT_CPP SClass* _makeDCFromWindowHandle(SClass* me_, S64 hwnd)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	HWND hwnd2 = (HWND)hwnd;

	RECT rect;
	GetClientRect(hwnd2, &rect);

	HRESULT hr = factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			hwnd2, D2D1::SizeU(rect.right, rect.bottom)),
		&me2->renderTarget);
	if (hr != S_OK) THROW(0x22345678);
	return me_;
}

/* Direct3D 10 -> Direct2D ‚Ö‚Ì•ÏŠ·‚ÍŒã‰ñ‚µ
EXPORT_CPP SClass* _makeDCFromDxgiSwapChain(SClass* me_, void* swapChain)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	IDXGISwapChain* sc2 = reinterpret_cast<IDXGISwapChain*>(swapChain);
	
}
*/

static inline D2D1::ColorF colorFromS64(S64 color) {
	union {
		S64 i;
		struct { S32 argb; S32 rest; } rgb;
		struct { S16 _1; S8 _2; U8 a; S32 _3; } a;
	} u;
	u.i = color;
	return D2D1::ColorF(u.rgb.argb, u.a.a / 255.0f);
}

EXPORT_CPP void _dcDtor(SClass* me_)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	me2->renderTarget->Release();
}

EXPORT_CPP void _dcBegin(SClass* me_)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	me2->renderTarget->BeginDraw();
}

EXPORT_CPP void _dcEnd(SClass* me_)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	me2->renderTarget->EndDraw();
}

EXPORT_CPP void _circleLine(
	SClass* me_, double x, double y, double rx, double ry, double strokeWidth, S64 color)
{
	SDeviceContext* me2 = reinterpret_cast<SDeviceContext*>(me_);
	ID2D1SolidColorBrush *brush;
	me2->renderTarget->CreateSolidColorBrush(colorFromS64(color), &brush);
	me2->renderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush, strokeWidth);
}