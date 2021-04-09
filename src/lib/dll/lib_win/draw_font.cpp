#include "draw_font.h"

static double CalcFontLineWidth(SFont* font, const Char* text);
static double CalcFontLineHeight(SFont* font, const Char* text);
static int SearchFromCache(SFont* me, int cell_num_width, int cell_num, Char c);

EXPORT_CPP void _fontAlign(SClass* me_, S64 horizontal, S64 vertical)
{
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	me2->AlignHorizontal = static_cast<U8>(horizontal);
	me2->AlignVertical = static_cast<U8>(vertical);
}

EXPORT_CPP void _fontCalcSize(SClass* me_, double* width, double* height, const U8* text)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	S64 len = *reinterpret_cast<const S64*>(text + 0x08);

	*width = 0.0;
	*height = 0.0;
	double x = 0.0;
	double y = 0.0;
	const Char* ptr = reinterpret_cast<const Char*>(text + 0x10);
	if (me2->Proportional)
	{
		HGDIOBJ old_font = SelectObject(me2->Dc, static_cast<HGDIOBJ>(me2->Font));
		for (S64 i = 0; i < len; i++)
		{
			Char c = *ptr;
			switch (c)
			{
				case L'\n':
					x = 0.0;
					y += me2->Height;
					ptr++;
					continue;
				case L'\t':
					c = L' ';
					break;
			}
			SIZE size;
			GetTextExtentPoint32(me2->Dc, &c, 1, &size);
			x += me2->Advance + static_cast<double>(size.cx);
			if (*width < x)
				*width = x;
			if (*height < y + me2->Height)
				*height = y + me2->Height;
			ptr++;
		}
		SelectObject(me2->Dc, old_font);
	}
	else
	{
		for (S64 i = 0; i < len; i++)
		{
			Char c = *ptr;
			switch (c)
			{
				case L'\n':
					x = 0.0;
					y += me2->Height;
					ptr++;
					continue;
				case L'\t':
					c = L' ';
					break;
			}
			x += me2->Advance;
			if (*width < x)
				*width = x;
			if (*height < y + me2->Height)
				*height = y + me2->Height;
			ptr++;
		}
	}
}

EXPORT_CPP double _fontCalcWidth(SClass* me_, const U8* text)
{
	double width;
	double height;
	_fontCalcSize(me_, &width, &height, text);
	return width;
}

EXPORT_CPP void _fontDraw(SClass* me_, double dstX, double dstY, const U8* text, S64 color)
{
	_fontDrawScale(me_, dstX, dstY, 1.0, 1.0, text, color);
}

EXPORT_CPP void _fontDrawScale(SClass* me_, double dstX, double dstY, double dstScaleX, double dstScaleY, const U8* text, S64 color)
{
	THROWDBG(text == nullptr, EXCPT_ACCESS_VIOLATION);
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;

	SFont* me2 = reinterpret_cast<SFont*>(me_);
	S64 len = *reinterpret_cast<const S64*>(text + 0x08);
	const Char* ptr = reinterpret_cast<const Char*>(text + 0x10);
	int cell_num_width = FontBitmapSize / me2->CellWidth;
	int cell_num = cell_num_width * (FontBitmapSize / me2->CellHeight);

	me2->Cnt++;
	if (me2->Cnt == 0)
	{
		for (int i = 0; i < cell_num; i++)
			me2->CntMap[i] = 0;
	}

	double x = dstX;
	switch (me2->AlignHorizontal)
	{
		case 1: // 'center'
			x -= CalcFontLineWidth(me2, ptr) / 2.0 * dstScaleX;
			break;
		case 2: // 'right'
			x -= CalcFontLineWidth(me2, ptr) * dstScaleX;
			break;
	}

	double y = dstY;
	switch (me2->AlignVertical)
	{
		case 1: // 'center'
			y -= CalcFontLineHeight(me2, ptr) / 2.0 * dstScaleY;
			break;
		case 2: // 'bottom'
			y -= CalcFontLineHeight(me2, ptr) * dstScaleY;
			break;
	}

	for (S64 i = 0; i < len; i++)
	{
		Char c = *ptr;
		switch (c)
		{
			case L'\n':
				x = dstX;
				ptr++;
				switch (me2->AlignHorizontal)
				{
					case 1: // 'center'
						x -= CalcFontLineWidth(me2, ptr) / 2.0 * dstScaleX;
						break;
					case 2: // 'right'
						x -= CalcFontLineWidth(me2, ptr) * dstScaleX;
						break;
				}
				y += me2->Height * dstScaleY;
				continue;
			case L'\t':
				c = L' ';
				break;
		}

		int pos = SearchFromCache(me2, cell_num_width, cell_num, c);
		double half_space = 0.0;
		if (!me2->Proportional)
			half_space = floor((me2->Advance - static_cast<double>(me2->GlyphWidth[pos])) / 2.0 * dstScaleX);
		{
			float const_buf_vs[8] =
			{
				static_cast<float>(half_space + x) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
				-(static_cast<float>(y) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
				static_cast<float>(static_cast<double>(me2->CellWidth) * dstScaleX) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
				-(static_cast<float>(static_cast<double>(me2->CellHeight) * dstScaleY) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
				0.0f,
				0.0f,
				static_cast<float>(me2->CellWidth) / static_cast<float>(me2->CellSizeAligned),
				-(static_cast<float>(me2->CellHeight) / static_cast<float>(me2->CellSizeAligned)),
			};
			float const_buf_ps[4] =
			{
				static_cast<float>(r),
				static_cast<float>(g),
				static_cast<float>(b),
				static_cast<float>(a),
			};
			ConstBuf(ShaderBufs[ShaderBuf_TexVs], const_buf_vs);
			Device->GSSetShader(nullptr);
			ConstBuf(ShaderBufs[ShaderBuf_FontPs], const_buf_ps);
			VertexBuf(VertexBufs[VertexBuf_RectVertex]);
			Device->PSSetShaderResources(0, 1, &me2->View);
			Device->DrawIndexed(6, 0, 0);
		}
		x += me2->Advance * dstScaleX;
		if (me2->Proportional)
			x += static_cast<double>(me2->GlyphWidth[pos]) * dstScaleX;
		ptr++;
	}
}

EXPORT_CPP void _fontFin(SClass* me_)
{
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	if (me2->Dc != nullptr)
	{
		DeleteDC(me2->Dc);
		me2->Dc = nullptr;
	}
	if (me2->Bitmap != nullptr)
	{
		DeleteObject(static_cast<HGDIOBJ>(me2->Bitmap));
		me2->Bitmap = nullptr;
	}
	if (me2->GlyphWidth != nullptr)
	{
		FreeMem(me2->GlyphWidth);
		me2->GlyphWidth = nullptr;
	}
	if (me2->CntMap != nullptr)
	{
		FreeMem(me2->CntMap);
		me2->CntMap = nullptr;
	}
	if (me2->CharMap != nullptr)
	{
		FreeMem(me2->CharMap);
		me2->CharMap = nullptr;
	}
	if (me2->View != nullptr)
	{
		me2->View->Release();
		me2->View = nullptr;
	}
	if (me2->Tex != nullptr)
	{
		me2->Tex->Release();
		me2->Tex = nullptr;
	}
}

EXPORT_CPP double _fontGetHeight(SClass* me_)
{
	return reinterpret_cast<SFont*>(me_)->Height;
}

EXPORT_CPP S64 _fontHandle(SClass* me_)
{
	return reinterpret_cast<S64>(reinterpret_cast<SFont*>(me_)->Font);
}

EXPORT_CPP double _fontMaxHeight(SClass* me_)
{
	return reinterpret_cast<SFont*>(me_)->CellHeight;
}

EXPORT_CPP double _fontMaxWidth(SClass* me_)
{
	return reinterpret_cast<SFont*>(me_)->CellWidth;
}

EXPORT_CPP void _fontSetHeight(SClass* me_, double height)
{
	reinterpret_cast<SFont*>(me_)->Height = height;
}

EXPORT_CPP SClass* _makeFont(SClass* me_, const U8* fontName, S64 size, bool bold, bool italic, bool proportional, double advance)
{
	THROWDBG(size < 1, 0xe9170006);
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	int char_height;
	{
		HDC dc = GetDC(nullptr);
		char_height = MulDiv(static_cast<int>(size), GetDeviceCaps(dc, LOGPIXELSY), 72);
		ReleaseDC(nullptr, dc);
	}
	me2->Font = CreateFont(-char_height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic ? TRUE : FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, fontName == nullptr ? L"Meiryo UI" : reinterpret_cast<const Char*>(fontName + 0x10));
	me2->Proportional = proportional;
	me2->AlignHorizontal = 0;
	me2->AlignVertical = 0;
	me2->Advance = advance;
	{
		BITMAPINFO info = { 0 };
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = static_cast<LONG>(FontBitmapSize);
		info.bmiHeader.biHeight = -static_cast<LONG>(FontBitmapSize);
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 24;
		info.bmiHeader.biCompression = BI_RGB;
		HDC dc = GetDC(nullptr);
		me2->Bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&me2->Pixel), nullptr, 0);
		me2->Dc = CreateCompatibleDC(dc);
		ReleaseDC(nullptr, dc);
	}
	{
		HGDIOBJ old_font = SelectObject(me2->Dc, static_cast<HGDIOBJ>(me2->Font));
		TEXTMETRIC tm;
		GetTextMetrics(me2->Dc, &tm);
		me2->CellWidth = tm.tmMaxCharWidth;
		me2->CellHeight = tm.tmHeight;
		me2->Height = (double)tm.tmHeight;
		SelectObject(me2->Dc, old_font);
	}
	me2->CellSizeAligned = 128; // Texture length must not be less than 128.
	while (me2->CellSizeAligned < me2->CellWidth + 1 || me2->CellSizeAligned < me2->CellHeight + 1)
		me2->CellSizeAligned *= 2;
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width = static_cast<UINT>(me2->CellSizeAligned);
		desc.Height = static_cast<UINT>(me2->CellSizeAligned);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_DYNAMIC;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateTexture2D(&desc, nullptr, &me2->Tex)))
			THROW(0xe9170009);
	}
	{
		D3D10_SHADER_RESOURCE_VIEW_DESC desc;
		memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;
		if (FAILED(Device->CreateShaderResourceView(me2->Tex, &desc, &me2->View)))
			THROW(0xe9170009);
	}
	size_t buf_size = static_cast<size_t>((FontBitmapSize / me2->CellWidth) * (FontBitmapSize / me2->CellHeight));
	ASSERT(buf_size != 0);
	me2->CharMap = static_cast<Char*>(AllocMem(sizeof(Char) * buf_size));
	me2->CntMap = static_cast<U32*>(AllocMem(sizeof(U32) * buf_size));
	me2->GlyphWidth = static_cast<int*>(AllocMem(sizeof(int) * buf_size));
	for (size_t i = 0; i < buf_size; i++)
		me2->GlyphWidth[i] = 0;
	for (size_t i = 0; i < buf_size; i++)
	{
		me2->CharMap[i] = L'\0';
		me2->CntMap[i] = 0;
	}
	me2->Cnt = 0;
	return me_;
}

static double CalcFontLineWidth(SFont* font, const Char* text)
{
	double x = 0.0;
	const Char* ptr = text;
	if (font->Proportional)
	{
		HGDIOBJ old_font = SelectObject(font->Dc, static_cast<HGDIOBJ>(font->Font));
		while (*ptr != L'\0' && *ptr != L'\n')
		{
			Char c = *ptr;
			switch (c)
			{
				case L'\t':
					c = L' ';
					break;
			}
			SIZE size;
			GetTextExtentPoint32(font->Dc, &c, 1, &size);
			x += font->Advance + static_cast<double>(size.cx);
			ptr++;
		}
		SelectObject(font->Dc, old_font);
	}
	else
	{
		while (*ptr != L'\0' && *ptr != L'\n')
		{
			Char c = *ptr;
			switch (c)
			{
				case L'\t':
					c = L' ';
					break;
			}
			x += font->Advance;
			ptr++;
		}
	}
	return x;
}

static double CalcFontLineHeight(SFont* font, const Char* text)
{
	int cnt = 0;
	const Char* ptr = text;
	while (*ptr != L'\0')
	{
		if (*ptr == L'\n')
			cnt++;
		ptr++;
	}
	return static_cast<double>(cnt + 1) * font->Height;
}

static int SearchFromCache(SFont* me, int cell_num_width, int cell_num, Char c)
{
	int pos = -1;
	for (int i = 0; i < cell_num; i++)
	{
		if (me->CharMap[i] == c)
		{
			pos = i;
			break;
		}
	}
	if (pos == -1)
	{
		U32 min = 0xffffffff;
		for (int i = 0; i < cell_num; i++)
		{
			if (me->CharMap[i] == L'\0')
			{
				pos = i;
				break;
			}
			if (min > static_cast<S64>(me->CntMap[i]))
			{
				min = static_cast<S64>(me->CntMap[i]);
				pos = i;
			}
		}
		{
			HGDIOBJ old_bitmap = SelectObject(me->Dc, static_cast<HGDIOBJ>(me->Bitmap));
			HGDIOBJ old_font = SelectObject(me->Dc, static_cast<HGDIOBJ>(me->Font));
			SetBkMode(me->Dc, OPAQUE);
			SetBkColor(me->Dc, RGB(0, 0, 0));
			SetTextColor(me->Dc, RGB(255, 255, 255));
			RECT rect;
			rect.left = static_cast<LONG>((pos % cell_num_width) * me->CellWidth);
			rect.top = static_cast<LONG>((pos / cell_num_width) * me->CellHeight);
			rect.right = rect.left + static_cast<LONG>(me->CellWidth);
			rect.bottom = rect.top + static_cast<LONG>(me->CellHeight);
			ExtTextOut(me->Dc, static_cast<int>(rect.left), static_cast<int>(rect.top), ETO_CLIPPED | ETO_OPAQUE, &rect, &c, 1, nullptr);
			{
				GLYPHMETRICS gm;
				MAT2 mat = { { 0, 1 },{ 0, 0 },{ 0, 0 },{ 0, 1 } };
				GetGlyphOutline(me->Dc, static_cast<UINT>(c), GGO_METRICS, &gm, 0, nullptr, &mat);
				me->GlyphWidth[pos] = static_cast<int>(gm.gmCellIncX);
			}
			SelectObject(me->Dc, old_font);
			SelectObject(me->Dc, old_bitmap);
		}
		me->CharMap[pos] = c;
		me->CntMap[pos] = me->Cnt;
	}
	{
		D3D10_MAPPED_TEXTURE2D map;
		me->Tex->Map(D3D10CalcSubresource(0, 0, 1), D3D10_MAP_WRITE_DISCARD, 0, &map);
		U8* dst = static_cast<U8*>(map.pData);
		for (int i = 0; i < me->CellHeight; i++)
		{
			int begin = ((pos / cell_num_width) * me->CellHeight + i) * FontBitmapSize + (pos % cell_num_width) * me->CellWidth;
			for (int k = 0; k < me->CellWidth; k++)
				dst[(i * me->CellSizeAligned + k) * 4] = me->Pixel[(begin + k) * 3];
			dst[(i * me->CellSizeAligned + me->CellWidth) * 4] = 0;
		}
		{
			int i = me->CellHeight;
			for (int k = 0; k < me->CellWidth + 1; k++)
				dst[(i * me->CellSizeAligned + k) * 4] = 0;
		}
		me->Tex->Unmap(D3D10CalcSubresource(0, 0, 1));
	}
	return pos;
}
