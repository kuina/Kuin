#include "draw_primitive.h"

EXPORT_CPP void _circle(double x, double y, double radiusX, double radiusY, S64 color)
{
	if (radiusX == 0.0 || radiusY == 0.0)
		return;
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;
	if (radiusX < 0.0)
		radiusX = -radiusX;
	if (radiusY < 0.0)
		radiusY = -radiusY;
	{
		float const_buf_vs[4] =
		{
			static_cast<float>(x) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(radiusX) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(radiusY) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
		};
		float const_buf_ps[8] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
			static_cast<float>(min(radiusX, radiusY)),
			0.0f,
			0.0f,
			0.0f
		};
		ConstBuf(ShaderBufs[ShaderBuf_CircleVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_CirclePs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_CircleVertex]);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _circleLine(double x, double y, double radiusX, double radiusY, S64 color)
{
	if (radiusX == 0.0 || radiusY == 0.0)
		return;
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;
	if (radiusX < 0.0)
		radiusX = -radiusX;
	if (radiusY < 0.0)
		radiusY = -radiusY;
	{
		float const_buf_vs[4] =
		{
			static_cast<float>(x) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(radiusX) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(radiusY) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
		};
		float const_buf_ps[8] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
			static_cast<float>(min(radiusX, radiusY)),
			static_cast<float>(1.5f),
			0.0f,
			0.0f
		};
		ConstBuf(ShaderBufs[ShaderBuf_CircleVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_CircleLinePs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_CircleVertex]);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _line(double x1, double y1, double x2, double y2, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;
	{
		float const_buf_vs[4] =
		{
			static_cast<float>(x1) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y1) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(x2 - x1) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(y2 - y1) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		ConstBuf(ShaderBufs[ShaderBuf_RectVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_LineVertex]);
	}
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	Device->DrawIndexed(2, 0, 0);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

EXPORT_CPP void _rect(double x, double y, double w, double h, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;
	if (w < 0.0)
	{
		x += w;
		w = -w;
	}
	if (h < 0.0)
	{
		y += h;
		h = -h;
	}
	{
		float const_buf_vs[4] =
		{
			static_cast<float>(x) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(w) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(h) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		ConstBuf(ShaderBufs[ShaderBuf_RectVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_RectVertex]);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _rectLine(double x, double y, double w, double h, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;
	if (w < 0.0)
	{
		x += w;
		w = -w;
	}
	if (h < 0.0)
	{
		y += h;
		h = -h;
	}
	{
		float const_buf_vs[4] =
		{
			static_cast<float>(x) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(w) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(h) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		ConstBuf(ShaderBufs[ShaderBuf_RectVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_RectLineVertex]);
	}
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Device->DrawIndexed(5, 0, 0);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;
	if ((x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1) < 0.0)
	{
		double tmp;
		tmp = x2;
		x2 = x3;
		x3 = tmp;
		tmp = y2;
		y2 = y3;
		y3 = tmp;
	}
	{
		float const_buf_vs[8] =
		{
			static_cast<float>(x1) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y1) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(x2) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y2) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(x3) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(y3) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		ConstBuf(ShaderBufs[ShaderBuf_TriVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_TriVertex]);
	}
	Device->DrawIndexed(3, 0, 0);
}
