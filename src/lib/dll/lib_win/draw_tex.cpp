#include "draw_tex.h"

#include "bc_decoder.h"
#include "jpg_decoder.h"
#include "png_decoder.h"

static SClass* MakeTexImpl(SClass* me_, const U8* data, const U8* path, Bool as_argb);
static Bool StrCmpIgnoreCase(const Char* a, const Char* b);

EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double srcX, double srcY, double srcW, double srcH, S64 color)
{
	_texDrawScale(me_, dstX, dstY, srcW, srcH, srcX, srcY, srcW, srcH, color);
}

EXPORT_CPP void _texDrawRot(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, double centerX, double centerY, double angle, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;

	STex* me2 = reinterpret_cast<STex*>(me_);
	if (dstW < 0.0)
	{
		dstX += dstW;
		dstW = -dstW;
		srcX += srcW;
		srcW = -srcW;
	}
	if (dstH < 0.0)
	{
		dstY += dstH;
		dstH = -dstH;
		srcY += srcH;
		srcH = -srcH;
	}
	{
		float const_buf_vs[16] =
		{
			static_cast<float>(dstX) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(dstY) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(dstW) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(dstH) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
			static_cast<float>(srcX) / static_cast<float>(me2->Width),
			-(static_cast<float>(srcY) / static_cast<float>(me2->Height)),
			static_cast<float>(srcW) / static_cast<float>(me2->Width),
			-(static_cast<float>(srcH) / static_cast<float>(me2->Height)),
			static_cast<float>(centerX) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(centerY) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
			static_cast<float>(sin(-angle)),
			static_cast<float>(cos(-angle)),
			static_cast<float>(CurWndBuf->TexWidth) / static_cast<float>(CurWndBuf->TexHeight),
			0.0f,
			0.0f,
			0.0f,
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		ConstBuf(ShaderBufs[ShaderBuf_TexRotVs], const_buf_vs);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_TexPs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_RectVertex]);
		Device->PSSetShaderResources(0, 1, &me2->View);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _texDrawScale(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	if (a <= DiscardAlpha)
		return;

	STex* me2 = reinterpret_cast<STex*>(me_);
	if (dstW < 0.0)
	{
		dstX += dstW;
		dstW = -dstW;
		srcX += srcW;
		srcW = -srcW;
	}
	if (dstH < 0.0)
	{
		dstY += dstH;
		dstH = -dstH;
		srcY += srcH;
		srcH = -srcH;
	}
	{
		float const_buf_vs[8] =
		{
			static_cast<float>(dstX) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f,
			-(static_cast<float>(dstY) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f),
			static_cast<float>(dstW) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f,
			-(static_cast<float>(dstH) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f),
			static_cast<float>(srcX) / static_cast<float>(me2->Width),
			-(static_cast<float>(srcY) / static_cast<float>(me2->Height)),
			static_cast<float>(srcW) / static_cast<float>(me2->Width),
			-(static_cast<float>(srcH) / static_cast<float>(me2->Height)),
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
		ConstBuf(ShaderBufs[ShaderBuf_TexPs], const_buf_ps);
		VertexBuf(VertexBufs[VertexBuf_RectVertex]);
		Device->PSSetShaderResources(0, 1, &me2->View);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _texFin(SClass* me_)
{
	STex* me2 = reinterpret_cast<STex*>(me_);
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

EXPORT_CPP S64 _texHeight(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->Height);
}

EXPORT_CPP S64 _texImgHeight(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->ImgHeight);
}

EXPORT_CPP S64 _texImgWidth(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->ImgWidth);
}

EXPORT_CPP S64 _texWidth(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->Width);
}

EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* data, const U8* path)
{
	return MakeTexImpl(me_, data, path, False);
}

EXPORT_CPP SClass* _makeTexArgb(SClass* me_, const U8* data, const U8* path)
{
	return MakeTexImpl(me_, data, path, True);
}

EXPORT_CPP SClass* _makeTexEvenArgb(SClass* me_, double a, double r, double g, double b)
{
	STex* me2 = reinterpret_cast<STex*>(me_);
	float img[4] = { static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a) };
	me2->Width = 1;
	me2->Height = 1;
	me2->ImgWidth = 1;
	me2->ImgHeight = 1;
	if (!MakeTexWithImg(&me2->Tex, &me2->View, nullptr, 1, 1, img, sizeof(img), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D10_USAGE_IMMUTABLE, 0, False))
		THROW(0xe9170009);
	return me_;
}

EXPORT_CPP SClass* _makeTexEvenColor(SClass* me_, S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	return _makeTexEvenArgb(me_, a, r, g, b);
}

static SClass* MakeTexImpl(SClass* me_, const U8* data, const U8* path, Bool as_argb)
{
	THROWDBG(path == nullptr, EXCPT_ACCESS_VIOLATION);
	S64 path_len = *reinterpret_cast<const S64*>(path + 0x08);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	STex* me2 = reinterpret_cast<STex*>(me_);
	void* img = nullptr;
	Bool img_ref = False; // Set to true when 'img' should not be released.
	DXGI_FORMAT fmt;
	int width;
	int height;
	{
		size_t size = static_cast<size_t>(*reinterpret_cast<const S64*>(data + 0x08));
		const void* bin = data + 0x10;
		THROWDBG(path_len < 4, 0xe9170006);
		if (StrCmpIgnoreCase(path2 + path_len - 4, L".png"))
		{
			img = DecodePng(size, bin, &width, &height);
			me2->ImgWidth = width;
			me2->ImgHeight = height;
			fmt = as_argb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				img = AdjustTexSize(static_cast<U8*>(img), &width, &height);
		}
		else if (StrCmpIgnoreCase(path2 + path_len - 4, L".jpg"))
		{
			img = DecodeJpg(size, bin, &width, &height);
			me2->ImgWidth = width;
			me2->ImgHeight = height;
			fmt = as_argb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				img = AdjustTexSize(static_cast<U8*>(img), &width, &height);
		}
		else if (StrCmpIgnoreCase(path2 + path_len - 4, L".dds"))
		{
			img = DecodeBc(size, bin, &width, &height);
			me2->ImgWidth = width;
			me2->ImgHeight = height;
			img_ref = True;
			fmt = as_argb ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC3_UNORM_SRGB;
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				THROW(0xe9170008);
		}
		else
		{
			THROWDBG(True, 0xe9170006);
			me2->ImgWidth = 0;
			me2->ImgHeight = 0;
			fmt = DXGI_FORMAT_UNKNOWN;
			width = 0;
			height = 0;
		}
	}
	me2->Width = width;
	me2->Height = height;
	{
		Bool success = False;
		for (; ; )
		{
			if (!MakeTexWithImg(&me2->Tex, &me2->View, nullptr, me2->Width, me2->Height, img, me2->Width * 4, fmt, D3D10_USAGE_IMMUTABLE, 0, False))
				break;
			success = True;
			break;
		}
		if (img != nullptr && !img_ref)
			FreeMem(img);
		if (!success)
			THROW(0x0e9170009);
	}
	return me_;
}

static Bool StrCmpIgnoreCase(const Char* a, const Char* b)
{
	while (*a != L'\0')
	{
		Char a2 = L'A' <= *a && *a <= L'Z' ? (*a - L'A' + L'a') : *a;
		Char b2 = L'A' <= *b && *b <= L'Z' ? (*b - L'A' + L'a') : *b;
		if (a2 != b2)
			return False;
		a++;
		b++;
	}
	return *b == L'\0';
}
