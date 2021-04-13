#include "draw2d.h"

#include <d2d1.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")

struct S2dBuf
{
	IDXGISurface* Surface;
	ID2D1RenderTarget* RenderTarget;
	double Scale;
};

struct SWndBuf
{
	S2dBuf* Extra;
};

static ID2D1Factory* Factory = nullptr;
static HMODULE D0001Handle = nullptr;
static ID2D1RenderTarget* CurRenderTarget = nullptr;
static double CurScale = 1.0;
static Bool Opened;

static void* Callback2d(int kind, void* arg1, void* arg2);
static D2D1::ColorF ColorToColorF(S64 color);
static double Gamma(double value);

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory)))
		THROW(0xe9170009);

	D0001Handle = LoadLibrary(L"data/d0001.knd");
	if (D0001Handle == nullptr)
		THROW(0xe9170009);

	((void(*)(void* (*callback)(int, void*, void*)))GetProcAddress(D0001Handle, "_set2dCallback"))(Callback2d);
}

EXPORT_CPP void _fin()
{
	if (D0001Handle != nullptr)
		FreeLibrary(D0001Handle);
	if (Factory != nullptr)
		Factory->Release();
}

EXPORT_CPP void _circle(double x, double y, double radius_x, double radius_y, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(static_cast<FLOAT>(x * CurScale), static_cast<FLOAT>(y * CurScale)), static_cast<FLOAT>(radius_x * CurScale), static_cast<FLOAT>(radius_y * CurScale)), brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _circleLine(double x, double y, double radius_x, double radius_y, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(static_cast<FLOAT>(x * CurScale), static_cast<FLOAT>(y * CurScale)), static_cast<FLOAT>(radius_x * CurScale), static_cast<FLOAT>(radius_y * CurScale)), brush, static_cast<FLOAT>(stroke_width * CurScale), nullptr);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _line(double x1, double y1, double x2, double y2, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawLine(D2D1::Point2F(static_cast<FLOAT>(x1 * CurScale), static_cast<FLOAT>(y1 * CurScale)), D2D1::Point2F(static_cast<FLOAT>(x2 * CurScale), static_cast<FLOAT>(y2 * CurScale)), brush, static_cast<FLOAT>(stroke_width * CurScale), nullptr);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _rect(double x, double y, double width, double height, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillRectangle(D2D1::RectF(static_cast<FLOAT>(x * CurScale), static_cast<FLOAT>(y * CurScale), static_cast<FLOAT>((x + width) * CurScale), static_cast<FLOAT>((y + height) * CurScale)), brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _rectLine(double x, double y, double width, double height, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawRectangle(D2D1::RectF(static_cast<FLOAT>(x * CurScale), static_cast<FLOAT>(y * CurScale), static_cast<FLOAT>((x + width) * CurScale), static_cast<FLOAT>((y + height) * CurScale)), brush, static_cast<FLOAT>(stroke_width * CurScale), nullptr);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _roundRect(double x, double y, double width, double height, double radius_x, double radius_y, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(static_cast<FLOAT>(x * CurScale), static_cast<FLOAT>(y * CurScale), static_cast<FLOAT>((x + width) * CurScale), static_cast<FLOAT>((y + height) * CurScale)), static_cast<FLOAT>(radius_x * CurScale), static_cast<FLOAT>(radius_y * CurScale)), brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _roundRectLine(double x, double y, double width, double height, double radius_x, double radius_y, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(static_cast<FLOAT>(x * CurScale), static_cast<FLOAT>(y * CurScale), static_cast<FLOAT>((x + width) * CurScale), static_cast<FLOAT>((y + height) * CurScale)), static_cast<FLOAT>(radius_x * CurScale), static_cast<FLOAT>(radius_y * CurScale)), brush, static_cast<FLOAT>(stroke_width * CurScale), nullptr);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color)
{
	ID2D1SolidColorBrush* brush = nullptr;
	ID2D1PathGeometry* geometry = nullptr;
	ID2D1GeometrySink* sink = nullptr;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	Factory->CreatePathGeometry(&geometry);
	geometry->Open(&sink);
	sink->BeginFigure(D2D1::Point2F(static_cast<FLOAT>(x1 * CurScale), static_cast<FLOAT>(y1 * CurScale)), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x2 * CurScale), static_cast<FLOAT>(y2 * CurScale)));
	sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x3 * CurScale), static_cast<FLOAT>(y3 * CurScale)));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	CurRenderTarget->FillGeometry(geometry, brush);
	sink->Release();
	geometry->Release();
	brush->Release();
	CurRenderTarget->Flush();
}

static void* Callback2d(int kind, void* arg1, void* arg2)
{
	switch (kind)
	{
		case 0: // Initialize 'S2dBuf'.
			{
				void** arg2s = static_cast<void**>(arg2);
				SWndBuf* wnd_buf = static_cast<SWndBuf*>(arg1);
				wnd_buf->Extra = static_cast<S2dBuf*>(AllocMem(sizeof(S2dBuf)));
				memset(wnd_buf->Extra, 0, sizeof(S2dBuf));

				wnd_buf->Extra->Surface = static_cast<IDXGISurface*>(arg2s[0]);
				wnd_buf->Extra->Scale = *static_cast<double*>(arg2s[1]);
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
				if (FAILED(Factory->CreateDxgiSurfaceRenderTarget(wnd_buf->Extra->Surface, &props, &wnd_buf->Extra->RenderTarget)))
					THROW(0xe9170009);
			}
			return nullptr;
		case 1: // Finalize 'S2dBuf'.
			{
				SWndBuf* wnd_buf = static_cast<SWndBuf*>(arg1);
				if (wnd_buf->Extra->Surface != nullptr)
					wnd_buf->Extra->Surface->Release();
				FreeMem(wnd_buf->Extra);
			}
			return nullptr;
		case 2: // 'ActiveDrawBuf'
			{
				if (Opened)
					CurRenderTarget->EndDraw();
				S2dBuf* extra = static_cast<SWndBuf*>(arg1)->Extra;
				CurRenderTarget = extra->RenderTarget;
				CurScale = extra->Scale;
				CurRenderTarget->BeginDraw();
				Opened = True;
			}
			return nullptr;
		case 3: // Write the buffer back.
			if (Opened)
				CurRenderTarget->EndDraw();
			CurRenderTarget->BeginDraw();
			Opened = True;
			return nullptr;
	}
	return nullptr;
}

static D2D1::ColorF ColorToColorF(S64 color)
{
	THROWDBG(color < 0 || 0xffffffff < color, 0xe9170006);
	const FLOAT a = static_cast<FLOAT>(static_cast<double>((color >> 24) & 0xff) / 255.0);
	const FLOAT r = static_cast<FLOAT>(Gamma(static_cast<double>((color >> 16) & 0xff) / 255.0));
	const FLOAT g = static_cast<FLOAT>(Gamma(static_cast<double>((color >> 8) & 0xff) / 255.0));
	const FLOAT b = static_cast<FLOAT>(Gamma(static_cast<double>(color & 0xff) / 255.0));
	return D2D1::ColorF(r, g, b, a);
}

static double Gamma(double value)
{
	return value * (value * (value * 0.305306011 + 0.682171111) + 0.012522878);
}
