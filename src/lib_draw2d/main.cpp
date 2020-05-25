// LibDraw2d.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include <d2d1.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")

struct S2dBuf
{
	IDXGISurface* Surface;
	ID2D1RenderTarget* RenderTarget;
};

struct SWndBuf
{
	S2dBuf* Extra;
};

struct SBrush
{
	SClass Class;
	ID2D1Brush* Brush;
};

struct SBrushSolidColor
{
	SBrush Parent;
	ID2D1SolidColorBrush* BrushSolidColor;
};

struct SBrushLinearGradient
{
	SBrush Parent;
	ID2D1LinearGradientBrush* BrushLinearGradient;
};

struct SBrushRadialGradient
{
	SBrush Parent;
	ID2D1RadialGradientBrush* BrushRadialGradient;
};

struct SStrokeStyle
{
	SClass Class;
	ID2D1StrokeStyle* StrokeStyle;
};

struct SGeometry
{
	SClass Class;
	ID2D1Geometry* Geometry;
};

struct SGeometryPath
{
	SGeometry Parent;
	ID2D1PathGeometry* GeometryPath;
	ID2D1GeometrySink* Sink;
};

static ID2D1Factory* Factory = NULL;
static HMODULE D0001Handle = NULL;
static ID2D1RenderTarget* CurRenderTarget = NULL;
static Bool Opened;

static void* Callback2d(int kind, void* arg1, void* arg2);
static D2D1::ColorF ColorToColorF(S64 color);
static double Gamma(double value);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (!InitEnvVars(heap, heap_cnt, app_code, use_res_flags))
		return;

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory)))
		THROW(EXCPT_DEVICE_INIT_FAILED);

	D0001Handle = LoadLibrary(L"data/d0001.knd");
	if (D0001Handle == NULL)
		THROW(EXCPT_FILE_READ_FAILED);

	((void(*)(void*(*callback)(int, void*, void*)))GetProcAddress(D0001Handle, "_set2dCallback"))(Callback2d);
}

EXPORT_CPP void _fin()
{
	if (D0001Handle != NULL)
		FreeLibrary(D0001Handle);
	if (Factory != NULL)
		Factory->Release();
}

EXPORT_CPP void _line(double x1, double y1, double x2, double y2, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawLine(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2)), brush, static_cast<FLOAT>(stroke_width), NULL);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _rect(double x, double y, double width, double height, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillRectangle(D2D1::RectF(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + width), static_cast<FLOAT>(y + height)), brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _rectLine(double x, double y, double width, double height, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawRectangle(D2D1::RectF(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + width), static_cast<FLOAT>(y + height)), brush, static_cast<FLOAT>(stroke_width), NULL);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _circle(double x, double y, double radius_x, double radius_y, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)), static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _circleLine(double x, double y, double radius_x, double radius_y, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)), static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), brush, static_cast<FLOAT>(stroke_width), NULL);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _roundRect(double x, double y, double width, double height, double radius_x, double radius_y, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + width), static_cast<FLOAT>(y + height)), static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _roundRectLine(double x, double y, double width, double height, double radius_x, double radius_y, double stroke_width, S64 color)
{
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + width), static_cast<FLOAT>(y + height)), static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), brush, static_cast<FLOAT>(stroke_width), NULL);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color)
{
	ID2D1SolidColorBrush* brush = NULL;
	ID2D1PathGeometry* geometry = NULL;
	ID2D1GeometrySink* sink = NULL;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	Factory->CreatePathGeometry(&geometry);
	geometry->Open(&sink);
	sink->BeginFigure(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2)));
	sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x3), static_cast<FLOAT>(y3)));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	CurRenderTarget->FillGeometry(geometry, brush);
	sink->Release();
	geometry->Release();
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP SClass* _makeBrushSolidColor(SClass* me_, S64 color)
{
	SBrushSolidColor* me2 = reinterpret_cast<SBrushSolidColor*>(me_);
	D2D1_COLOR_F color_ = ColorToColorF(color);
	CurRenderTarget->CreateSolidColorBrush(color_, &me2->BrushSolidColor);
	me2->BrushSolidColor->SetOpacity(color_.a);
	me2->BrushSolidColor->QueryInterface(IID_ID2D1Brush, (void**)&me2->Parent.Brush);
	return me_;
}

EXPORT_CPP void _brushSolidColorDtor(SClass* me_)
{
	SBrushSolidColor* me2 = reinterpret_cast<SBrushSolidColor*>(me_);
	me2->Parent.Brush->Release();
	me2->BrushSolidColor->Release();
}

EXPORT_CPP SClass* _makeBrushLinearGradient(SClass* me_, double x1, double y1, double x2, double y2, void* color_position, void* color)
{
	SBrushLinearGradient* me2 = reinterpret_cast<SBrushLinearGradient*>(me_);
	S64 len_pos = static_cast<S64*>(color_position)[1];
	S64 len_color = static_cast<S64*>(color)[1];
	THROWDBG(len_pos != len_color, EXCPT_DBG_ARG_OUT_DOMAIN);
	ID2D1GradientStopCollection* gradient_stop_collection = NULL;
	D2D1_GRADIENT_STOP* gradient_stops = static_cast<D2D1_GRADIENT_STOP*>(AllocMem(sizeof(D2D1_GRADIENT_STOP) * len_pos));
	for (S64 i = 0; i < len_pos; i++)
	{
		gradient_stops[i].color = ColorToColorF(static_cast<S64*>(color)[i + 2]);
		gradient_stops[i].position = static_cast<FLOAT>(static_cast<double*>(color_position)[i + 2]);
	}
	CurRenderTarget->CreateGradientStopCollection(gradient_stops, static_cast<UINT32>(len_pos), &gradient_stop_collection);
	FreeMem(gradient_stops);
	CurRenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2))), gradient_stop_collection, &me2->BrushLinearGradient);
	gradient_stop_collection->Release();
	me2->BrushLinearGradient->QueryInterface(IID_ID2D1Brush, (void**)&me2->Parent.Brush);
	return me_;
}

EXPORT_CPP void _brushLinearGradientDtor(SClass* me_)
{
	SBrushLinearGradient* me2 = reinterpret_cast<SBrushLinearGradient*>(me_);
	me2->Parent.Brush->Release();
	me2->BrushLinearGradient->Release();
}

EXPORT_CPP SClass* _makeBrushRadialGradient(SClass* me_, double left, double top, double right, double bottom, void* color_position, void* color)
{
	SBrushRadialGradient* me2 = reinterpret_cast<SBrushRadialGradient*>(me_);
	S64 lenPos = static_cast<S64*>(color_position)[1];
	S64 lenColor = static_cast<S64*>(color)[1];
	THROWDBG(lenPos != lenColor, 0xe9170006);
	ID2D1GradientStopCollection* gradient_stop_collection = NULL;
	D2D1_GRADIENT_STOP* gradient_stops = static_cast<D2D1_GRADIENT_STOP*>(AllocMem(sizeof(D2D1_GRADIENT_STOP) * lenPos));
	for (S64 i = 0; i < lenPos; i++)
	{
		gradient_stops[i].color = ColorToColorF(static_cast<S64*>(color)[i + 2]);
		gradient_stops[i].position = static_cast<FLOAT>(static_cast<double*>(color_position)[i + 2]);
	}
	CurRenderTarget->CreateGradientStopCollection(gradient_stops, static_cast<UINT32>(lenPos), &gradient_stop_collection);
	FreeMem(gradient_stops);
	CurRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(static_cast<FLOAT>(0.5 * (right + left)), static_cast<FLOAT>(0.5 * (top + bottom))), D2D1::Point2F(0.0f, 0.0f), static_cast<FLOAT>(right - left), static_cast<FLOAT>(bottom - top)), gradient_stop_collection, &me2->BrushRadialGradient);
	gradient_stop_collection->Release();
	me2->BrushRadialGradient->QueryInterface(IID_ID2D1Brush, (void**)&me2->Parent.Brush);
	return me_;
}

EXPORT_CPP void _brushRadialGradientSetGradientOriginOffset(SClass* me_, double offsetX, double offsetY)
{
	SBrushRadialGradient* me2 = reinterpret_cast<SBrushRadialGradient*>(me_);
	me2->BrushRadialGradient->SetGradientOriginOffset(D2D1::Point2F(static_cast<FLOAT>(offsetX), static_cast<FLOAT>(offsetY)));
}

EXPORT_CPP void _brushRadialGradientDtor(SClass* me_)
{
	SBrushRadialGradient* me2 = reinterpret_cast<SBrushRadialGradient*>(me_);
	me2->Parent.Brush->Release();
	me2->BrushRadialGradient->Release();
}

EXPORT_CPP void _brushLine(SClass* me_, double x1, double y1, double x2, double y2, double stroke_width, SClass* stroke_style)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	SStrokeStyle* strokeStyle2 = reinterpret_cast<SStrokeStyle*>(stroke_style);
	CurRenderTarget->DrawLine(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2)), me2->Brush, static_cast<FLOAT>(stroke_width), stroke_style != NULL ? strokeStyle2->StrokeStyle : NULL);
	CurRenderTarget->Flush();
}

EXPORT_CPP void _brushRect(SClass* me_, double x, double y, double width, double height)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	CurRenderTarget->FillRectangle(D2D1::RectF(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + width), static_cast<FLOAT>(y + height)), me2->Brush);
}

EXPORT_CPP void _brushRectLine(SClass* me_, double x, double y, double width, double height, double stroke_width, SClass* stroke_style)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	SStrokeStyle* strokeStyle2 = reinterpret_cast<SStrokeStyle*>(stroke_style);
	CurRenderTarget->DrawRectangle(D2D1::RectF(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + width), static_cast<FLOAT>(y + height)), me2->Brush, static_cast<FLOAT>(stroke_width), stroke_style != NULL ? strokeStyle2->StrokeStyle : NULL);
}

EXPORT_CPP void _brushCircle(SClass* me_, double x, double y, double radius_x, double radius_y)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	CurRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)), static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), me2->Brush);
}

EXPORT_CPP void _brushCircleLine(SClass* me_, double x, double y, double radius_x, double radius_y, double stroke_width, SClass* stroke_style)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	SStrokeStyle* strokeStyle2 = reinterpret_cast<SStrokeStyle*>(stroke_style);
	CurRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)), static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), me2->Brush, static_cast<FLOAT>(stroke_width), stroke_style != NULL ? strokeStyle2->StrokeStyle : NULL);
}

EXPORT_CPP void _brushTri(SClass* me_, double x1, double y1, double x2, double y2, double x3, double y3)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	ID2D1PathGeometry* geometry = NULL;
	ID2D1GeometrySink* sink = NULL;
	Factory->CreatePathGeometry(&geometry);
	geometry->Open(&sink);
	sink->BeginFigure(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2)));
	sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x3), static_cast<FLOAT>(y3)));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	CurRenderTarget->FillGeometry(geometry, me2->Brush);
	sink->Release();
	geometry->Release();
}

EXPORT_CPP void _brushDraw(SClass* me_, SClass* geometry)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	SGeometry* geometry2 = reinterpret_cast<SGeometry*>(geometry);
	CurRenderTarget->FillGeometry(geometry2->Geometry, me2->Brush);
}

EXPORT_CPP void _brushDrawLine(SClass* me_, SClass* geometry, double stroke_width, SClass* stroke_style)
{
	SBrush* me2 = reinterpret_cast<SBrush*>(me_);
	SGeometry* geometry2 = reinterpret_cast<SGeometry*>(geometry);
	SStrokeStyle* stroke_style2 = reinterpret_cast<SStrokeStyle*>(stroke_style);
	CurRenderTarget->DrawGeometry(geometry2->Geometry, me2->Brush, static_cast<FLOAT>(stroke_width), stroke_style2 == NULL ? NULL : stroke_style2->StrokeStyle);
}

EXPORT_CPP SClass* _makeStrokeStyle(SClass *me_, S64 startCap, S64 endCap, S64 dashCap, S64 lineJoin, double miterLimit, S64 dashStyle, double dashOffset, void *dash)
{
	SStrokeStyle* me2 = reinterpret_cast<SStrokeStyle*>(me_);
	D2D1_STROKE_STYLE_PROPERTIES prop;
	prop.startCap = static_cast<D2D1_CAP_STYLE>(startCap);
	prop.endCap = static_cast<D2D1_CAP_STYLE>(endCap);
	prop.dashCap = static_cast<D2D1_CAP_STYLE>(dashCap);
	prop.lineJoin = static_cast<D2D1_LINE_JOIN>(lineJoin);
	prop.miterLimit = static_cast<FLOAT>(miterLimit);
	prop.dashStyle = static_cast<D2D1_DASH_STYLE>(dashStyle);
	prop.dashOffset = static_cast<FLOAT>(dashOffset);
	if (dash == NULL) {
		Factory->CreateStrokeStyle(prop, NULL, 0, &me2->StrokeStyle);
	}
	else {
		S64 lenDash = reinterpret_cast<S64*>(dash)[1];
		float* dashes = (float*)AllocMem(lenDash * sizeof(float));
		double* dash_ = reinterpret_cast<double*>(dash);
		for (S64 i = 0; i < lenDash; i++)
			dashes[i] = static_cast<FLOAT>(dash_[i + 2]);
		Factory->CreateStrokeStyle(prop, dashes, static_cast<UINT32>(lenDash), &me2->StrokeStyle);
		FreeMem(dashes);
	}
	return me_;
}

EXPORT_CPP void _strokeStyleDtor(SClass* me_)
{
	SStrokeStyle* me2 = reinterpret_cast<SStrokeStyle*>(me_);
	me2->StrokeStyle->Release();
}

EXPORT_CPP SClass* _makeGeometryPath(SClass* me_)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	Factory->CreatePathGeometry(&me2->GeometryPath);
	me2->GeometryPath->QueryInterface(IID_ID2D1Geometry, (void**)&me2->Parent.Geometry);
	me2->Sink = NULL;
	return me_;
}

EXPORT_CPP void _geometryPathDtor(SClass* me_)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	if (me2->Sink != NULL)
		me2->Sink->Release();
	me2->Parent.Geometry->Release();
	me2->GeometryPath->Release();
}

EXPORT_CPP void _geometryPathOpen(SClass* me_)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->GeometryPath->Open(&me2->Sink);
}

EXPORT_CPP void _geometryPathClose(SClass* me_)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->Close();
}

EXPORT_CPP void _geometryPathOpenFigure(SClass* me_, double x, double y, Bool filled)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->BeginFigure(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)), filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
}

EXPORT_CPP void _geometryPathCloseFigure(SClass* me_, Bool closed_path)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->EndFigure(closed_path ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
}

EXPORT_CPP void _geometryPathAddArc(SClass* me_, double x, double y, double radius_x, double radius_y, double angle, Bool ccw, Bool large_arc)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)), D2D1::SizeF(static_cast<FLOAT>(radius_x), static_cast<FLOAT>(radius_y)), static_cast<FLOAT>(angle), ccw ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE : D2D1_SWEEP_DIRECTION_CLOCKWISE, large_arc ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
}

EXPORT_CPP void _geometryPathAddBezier(SClass* me_, double x1, double y1, double x2, double y2, double x3, double y3)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2)), D2D1::Point2F(static_cast<FLOAT>(x3), static_cast<FLOAT>(y3))));
}

EXPORT_CPP void _geometryPathAddLine(SClass* me_, double x, double y)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->AddLine(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)));
}

EXPORT_CPP void _geometryPathAddQuadraticBezier(SClass* me_, double x1, double y1, double x2, double y2)
{
	SGeometryPath* me2 = reinterpret_cast<SGeometryPath*>(me_);
	me2->Sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)), D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2))));
}

EXPORT_CPP void _geometryDraw(SClass* me_, S64 color)
{
	SGeometry* me2 = reinterpret_cast<SGeometry*>(me_);
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->FillGeometry(me2->Geometry, brush);
	brush->Release();
	CurRenderTarget->Flush();
}

EXPORT_CPP void _geometryDrawLine(SClass* me_, double stroke_width, S64 color)
{
	SGeometry* me2 = reinterpret_cast<SGeometry*>(me_);
	ID2D1SolidColorBrush* brush;
	CurRenderTarget->CreateSolidColorBrush(ColorToColorF(color), &brush);
	CurRenderTarget->DrawGeometry(me2->Geometry, brush, static_cast<FLOAT>(stroke_width));
	brush->Release();
	CurRenderTarget->Flush();
}

static void* Callback2d(int kind, void* arg1, void* arg2)
{
	switch (kind)
	{
		case 0: // Initialize 'S2dBuf'.
			{
				SWndBuf* wnd_buf = static_cast<SWndBuf*>(arg1);
				wnd_buf->Extra = static_cast<S2dBuf*>(AllocMem(sizeof(S2dBuf)));
				memset(wnd_buf->Extra, 0, sizeof(S2dBuf));

				wnd_buf->Extra->Surface = static_cast<IDXGISurface*>(arg2);
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
				if (FAILED(Factory->CreateDxgiSurfaceRenderTarget(wnd_buf->Extra->Surface, &props, &wnd_buf->Extra->RenderTarget)))
					THROW(EXCPT_DEVICE_INIT_FAILED);
			}
			return NULL;
		case 1: // Finalize 'S2dBuf'.
			{
				SWndBuf* wnd_buf = static_cast<SWndBuf*>(arg1);
				if (wnd_buf->Extra->Surface != NULL)
					wnd_buf->Extra->Surface->Release();
				FreeMem(wnd_buf->Extra);
			}
			return NULL;
		case 2: // 'ActiveDrawBuf'
			if (Opened)
				CurRenderTarget->EndDraw();
			CurRenderTarget = static_cast<SWndBuf*>(arg1)->Extra->RenderTarget;
			CurRenderTarget->BeginDraw();
			Opened = True;
			return NULL;
		case 3: // Write the buffer back.
			if (Opened)
				CurRenderTarget->EndDraw();
			CurRenderTarget->BeginDraw();
			Opened = True;
			return NULL;
	}
	return NULL;
}

static D2D1::ColorF ColorToColorF(S64 color)
{
	THROWDBG(color < 0 || 0xffffffff < color, EXCPT_DBG_ARG_OUT_DOMAIN);
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
