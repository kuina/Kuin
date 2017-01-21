#include "draw.h"

// DirectX 10 is preinstalled on Windows Vista or later.
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d10.h>

#include "png_decoder.h"
#include "jpg_decoder.h"
#include "bc_decoder.h"

const int DepthNum = 4;
const int BlendNum = 5;
const int SamplerNum = 2;
const int JointMax = 64;

struct SWndBuf
{
	IDXGISwapChain* SwapChain;
	ID3D10RenderTargetView* RenderTargetView;
	ID3D10DepthStencilView* DepthView;
	FLOAT ClearColor[4];
	int Width;
	int Height;
};

struct SShaderBuf
{
	Draw::EShaderKind Kind;
	void* Shader;
	size_t ConstBufSize;
	ID3D10Buffer* ConstBuf;
	ID3D10InputLayout* Layout;
};

struct SVertexBuf
{
	ID3D10Buffer* Vertex;
	size_t VertexLineSize;
	ID3D10Buffer* Idx;
};

struct STex
{
	SClass Class;
	void* draw;
	void* drawScale;
	int RawWidth;
	int RawHeight;
	int AlignedWidth;
	int AlignedHeight;
	ID3D10Texture2D* Tex;
	ID3D10ShaderResourceView* View;
};

struct SObj
{
	SClass Class;
	void* draw;
	void* mtx;
	void* pos;
	void* look;
	void* lookCamera;

	struct SPolygon
	{
		void* VertexBuf;
		int VertexNum;
		int JointNum;
		int Begin;
		int End;
		float(*Joints)[4][4];
	};

	int ElementNum;
	int* ElementKinds;
	void** Elements;
	float Mtx[4][4];
	float NormMtx[4][4];
};

struct SObjVsConstBuf
{
	float World[4][4];
	float NormWorld[4][4];
	float ProjView[4][4];
	float Eye[4];
};

struct SObjJointVsConstBuf
{
	float World[4][4];
	float NormWorld[4][4];
	float ProjView[4][4];
	float Eye[4];
	float Joint[JointMax][4][4];
};

struct SObjPsConstBuf
{
	float Dir[4];
	float DirColor[4];
#if defined(DBG)
	int Mode[4];
#endif
};

static const FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

const U8* GetTriVsBin(size_t* size);
const U8* GetTriPsBin(size_t* size);
const U8* GetRectVsBin(size_t* size);
const U8* GetCircleVsBin(size_t* size);
const U8* GetCirclePsBin(size_t* size);
const U8* GetTexVsBin(size_t* size);
const U8* GetTexPsBin(size_t* size);
const U8* GetObjVsBin(size_t* size);
const U8* GetObjJointVsBin(size_t* size);
const U8* GetObjPsBin(size_t* size);

static ID3D10Device* Device = NULL;
static ID3D10RasterizerState* RasterizerState = NULL;
static ID3D10DepthStencilState* DepthState[DepthNum] = { NULL };
static ID3D10BlendState* BlendState[BlendNum] = { NULL };
static ID3D10SamplerState* Sampler[SamplerNum] = { NULL };
static SWndBuf* CurWndBuf;
static void* TriVertex = NULL;
static void* TriVs = NULL;
static void* TriPs = NULL;
static void* RectVertex = NULL;
static void* RectVs = NULL;
static void* CircleVertex = NULL;
static void* CircleVs = NULL;
static void* CirclePs = NULL;
static void* TexVs = NULL;
static void* TexPs = NULL;
static void* ObjVs = NULL;
static void* ObjJointVs = NULL;
static void* ObjPs = NULL;
static double ViewMtx[4][4];
static double ProjMtx[4][4];
static SObjJointVsConstBuf ObjVsConstBuf;
static SObjPsConstBuf ObjPsConstBuf;

static void TexDtor(SClass* me_);
static void ObjDtor(SClass* me_);

EXPORT_CPP void _render()
{
	CurWndBuf->SwapChain->Present(0, 0);
	Device->ClearRenderTargetView(CurWndBuf->RenderTargetView, CurWndBuf->ClearColor);
	Device->ClearDepthStencilView(CurWndBuf->DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	Device->RSSetState(RasterizerState);
}

EXPORT_CPP void _viewport(double x, double y, double w, double h)
{
	D3D10_VIEWPORT viewport =
	{
		static_cast<UINT>(x),
		static_cast<UINT>(y),
		static_cast<UINT>(w),
		static_cast<UINT>(h),
		0.0f,
		1.0f,
	};
	Device->RSSetViewports(1, &viewport);
}

EXPORT_CPP void _resetViewport()
{
	D3D10_VIEWPORT viewport =
	{
		0,
		0,
		static_cast<UINT>(CurWndBuf->Width),
		static_cast<UINT>(CurWndBuf->Height),
		0.0f,
		1.0f,
	};
	Device->RSSetViewports(1, &viewport);
}

EXPORT_CPP void _depth(Bool test, Bool write)
{
	int kind = (static_cast<int>(test) << 1) | static_cast<int>(write);
	/*
	// TODO:
	if (ZBuf == zbuf)
	return;
	*/
	Device->OMSetDepthStencilState(DepthState[kind], 0);
	// TODO: ZBuf = zbuf;
}

EXPORT_CPP void _blend(S64 kind)
{
	ASSERT(0 <= kind && kind < BlendNum);
	int kind2 = static_cast<int>(kind);
	/*
	// TODO:
	if (Blend == kind2)
	return;
	*/
	Device->OMSetBlendState(BlendState[kind2], BlendFactor, 0xffffffff);
	// TODO: Blend = kind2;
}

EXPORT_CPP void _sampler(S64 kind)
{
	ASSERT(0 <= kind && kind < SamplerNum);
	/*
	// TODO:
	if (Sampler == kind2)
	return;
	*/
	Device->PSSetSamplers(0, 1, &Sampler[kind]);
	// TODO: Sampler = kind2;
}

EXPORT_CPP void _clearColor(double r, double g, double b)
{
	CurWndBuf->ClearColor[0] = static_cast<FLOAT>(r);
	CurWndBuf->ClearColor[1] = static_cast<FLOAT>(g);
	CurWndBuf->ClearColor[2] = static_cast<FLOAT>(b);
}

EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, double r, double g, double b, double a)
{
	if (a <= 0.04)
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
			static_cast<float>(x1) / static_cast<float>(CurWndBuf->Width) * 2.0f - 1.0f,
			-(static_cast<float>(y1) / static_cast<float>(CurWndBuf->Height) * 2.0f - 1.0f),
			static_cast<float>(x2) / static_cast<float>(CurWndBuf->Width) * 2.0f - 1.0f,
			-(static_cast<float>(y2) / static_cast<float>(CurWndBuf->Height) * 2.0f - 1.0f),
			static_cast<float>(x3) / static_cast<float>(CurWndBuf->Width) * 2.0f - 1.0f,
			-(static_cast<float>(y3) / static_cast<float>(CurWndBuf->Height) * 2.0f - 1.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		Draw::ConstBuf(TriVs, const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(TriPs, const_buf_ps);
		Draw::VertexBuf(TriVertex);
	}
	Device->DrawIndexed(3, 0, 0);
}

EXPORT_CPP void _rect(double x, double y, double w, double h, double r, double g, double b, double a)
{
	if (a <= 0.04)
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
			static_cast<float>(x) / static_cast<float>(CurWndBuf->Width) * 2.0f - 1.0f,
			-(static_cast<float>(y) / static_cast<float>(CurWndBuf->Height) * 2.0f - 1.0f),
			static_cast<float>(w) / static_cast<float>(CurWndBuf->Width) * 2.0f,
			-(static_cast<float>(h) / static_cast<float>(CurWndBuf->Height) * 2.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		Draw::ConstBuf(RectVs, const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(TriPs, const_buf_ps);
		Draw::VertexBuf(RectVertex);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _circle(double x, double y, double radiusX, double radiusY, double r, double g, double b, double a)
{
	if (a <= 0.04)
		return;
	if (radiusX < 0.0)
		radiusX = -radiusX;
	if (radiusY < 0.0)
		radiusY = -radiusY;
	{
		float const_buf_vs[4] =
		{
			static_cast<float>(x) / static_cast<float>(CurWndBuf->Width) * 2.0f - 1.0f,
			-(static_cast<float>(y) / static_cast<float>(CurWndBuf->Height) * 2.0f - 1.0f),
			static_cast<float>(radiusX) / static_cast<float>(CurWndBuf->Width) * 2.0f,
			-(static_cast<float>(radiusY) / static_cast<float>(CurWndBuf->Height) * 2.0f),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(r),
			static_cast<float>(g),
			static_cast<float>(b),
			static_cast<float>(a),
		};
		Draw::ConstBuf(CircleVs, const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(CirclePs, const_buf_ps);
		Draw::VertexBuf(CircleVertex);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* path)
{
	S64 path_len = *reinterpret_cast<const S64*>(path + 0x08);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	STex* me2 = reinterpret_cast<STex*>(me_);
	void* bin = NULL;
	void* img = NULL;
	Bool img_ref = False; // Set to true when 'img' should not be released.
	DXGI_FORMAT format;
	int width;
	int height;
	InitClass(&me2->Class, NULL, TexDtor);
	{
		size_t size;
		bin = LoadFileAll(path2, &size, True);
		ASSERT(path_len >= 4);
		if (StrCmpIgnoreCase(path2 + path_len - 4, L".png"))
		{
			img = DecodePng(size, bin, &width, &height);
			format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		else if (StrCmpIgnoreCase(path2 + path_len - 4, L".jpg"))
		{
			img = DecodeJpg(size, bin, &width, &height);
			format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		else if (StrCmpIgnoreCase(path2 + path_len - 4, L".dds"))
		{
			img = DecodeBc(size, bin, &width, &height);
			img_ref = True;
			format = DXGI_FORMAT_BC3_UNORM_SRGB;
		}
		else
		{
			ASSERT(False);
			format = DXGI_FORMAT_UNKNOWN;
			width = 0;
			height = 0;
		}
	}
	me2->RawWidth = width;
	me2->RawHeight = height;
	me2->AlignedWidth = 1;
	while (me2->AlignedWidth < me2->RawWidth)
		me2->AlignedWidth *= 2;
	me2->AlignedHeight = 1;
	while (me2->AlignedHeight < me2->RawHeight)
		me2->AlignedHeight *= 2;
	{
		Bool success = False;
		for (; ; )
		{
			{
				D3D10_TEXTURE2D_DESC desc;
				D3D10_SUBRESOURCE_DATA sub;
				desc.Width = static_cast<UINT>(me2->AlignedWidth);
				desc.Height = static_cast<UINT>(me2->AlignedHeight);
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = D3D10_USAGE_IMMUTABLE;
				desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				sub.pSysMem = img;
				sub.SysMemPitch = static_cast<UINT>(me2->AlignedWidth * 4);
				sub.SysMemSlicePitch = 0;
				if (FAILED(Device->CreateTexture2D(&desc, &sub, &me2->Tex)))
					break;
			}
			{
				D3D10_SHADER_RESOURCE_VIEW_DESC desc;
				memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
				desc.Format = format;
				desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MostDetailedMip = 0;
				desc.Texture2D.MipLevels = 1;
				if (FAILED(Device->CreateShaderResourceView(me2->Tex, &desc, &me2->View)))
					break;
			}
			success = True;
			break;
		}
		if (img != NULL && !img_ref)
			FreeMem(img);
		if (bin != NULL)
			FreeMem(bin);
		if (!success)
			THROW(0x1000, L"");
	}
	return me_;
}

EXPORT_CPP SClass* _makeTexEven(SClass* me_, double r, double g, double b, double a)
{
	STex* me2 = reinterpret_cast<STex*>(me_);
	float img[4] = { static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a) };
	{
		D3D10_TEXTURE2D_DESC desc;
		D3D10_SUBRESOURCE_DATA sub;
		desc.Width = 1;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		sub.pSysMem = img;
		sub.SysMemPitch = static_cast<UINT>(sizeof(img));
		sub.SysMemSlicePitch = 0;
		if (FAILED(Device->CreateTexture2D(&desc, &sub, &me2->Tex)))
			THROW(0x1000, L"");
	}
	{
		D3D10_SHADER_RESOURCE_VIEW_DESC desc;
		memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;
		if (FAILED(Device->CreateShaderResourceView(me2->Tex, &desc, &me2->View)))
			THROW(0x1000, L"");
	}
	return me_;
}

EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double srcX, double srcY, double srcW, double srcH)
{
	_texDrawScale(me_, dstX, dstY, srcW, srcH, srcX, srcY, srcW, srcH);
}

EXPORT_CPP void _texDrawScale(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH)
{
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
			static_cast<float>(dstX) / static_cast<float>(CurWndBuf->Width) * 2.0f - 1.0f,
			-(static_cast<float>(dstY) / static_cast<float>(CurWndBuf->Height) * 2.0f - 1.0f),
			static_cast<float>(dstW) / static_cast<float>(CurWndBuf->Width) * 2.0f,
			-(static_cast<float>(dstH) / static_cast<float>(CurWndBuf->Height) * 2.0f),
			static_cast<float>(srcX) / static_cast<float>(me2->AlignedWidth),
			-(static_cast<float>(srcY) / static_cast<float>(me2->AlignedHeight)),
			static_cast<float>(srcW) / static_cast<float>(me2->AlignedWidth),
			-(static_cast<float>(srcH) / static_cast<float>(me2->AlignedHeight)),
		};
		float const_buf_ps[4] =
		{
			static_cast<float>(1.0),
			static_cast<float>(1.0),
			static_cast<float>(1.0),
			static_cast<float>(1.0),
		};
		Draw::ConstBuf(TexVs, const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(TexPs, const_buf_ps);
		Draw::VertexBuf(RectVertex);
		Device->PSSetShaderResources(0, 1, &me2->View);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ)
{
	double look[3], up[3], right[3], eye[3], pxyz[3], eye_len;

	look[0] = atX - eyeX;
	look[1] = atY - eyeY;
	look[2] = atZ - eyeZ;
	eye_len = Draw::Normalize(look);
	ASSERT(eye_len != 0.0);

	up[0] = upX;
	up[1] = upY;
	up[2] = upZ;
	Draw::Cross(right, up, look);
	if (Draw::Normalize(right) == 0.0)
	{
		ASSERT(false);
	}

	Draw::Cross(up, look, right);

	eye[0] = eyeX;
	eye[1] = eyeY;
	eye[2] = eyeZ;
	pxyz[0] = Draw::Dot(eye, right);
	pxyz[1] = Draw::Dot(eye, up);
	pxyz[2] = Draw::Dot(eye, look);

	ViewMtx[0][0] = right[0];
	ViewMtx[0][1] = up[0];
	ViewMtx[0][2] = look[0];
	ViewMtx[0][3] = 0.0;
	ViewMtx[1][0] = right[1];
	ViewMtx[1][1] = up[1];
	ViewMtx[1][2] = look[1];
	ViewMtx[1][3] = 0.0;
	ViewMtx[2][0] = right[2];
	ViewMtx[2][1] = up[2];
	ViewMtx[2][2] = look[2];
	ViewMtx[2][3] = 0.0;
	ViewMtx[3][0] = -pxyz[0];
	ViewMtx[3][1] = -pxyz[1];
	ViewMtx[3][2] = -pxyz[2];
	ViewMtx[3][3] = 1.0;

	ObjVsConstBuf.Eye[0] = static_cast<float>(eyeX);
	ObjVsConstBuf.Eye[1] = static_cast<float>(eyeY);
	ObjVsConstBuf.Eye[2] = static_cast<float>(eyeZ);
	ObjVsConstBuf.Eye[3] = static_cast<float>(eye_len);

	Draw::SetProjViewMtx(ObjVsConstBuf.ProjView, ProjMtx, ViewMtx);
}

EXPORT_CPP void _proj(double fovy, double aspectX, double aspectY, double nearZ, double farZ)
{
	double tan_theta = tan(fovy / 2.0);
	ProjMtx[0][0] = -1.0 / ((aspectX / aspectY) * tan_theta);
	ProjMtx[0][1] = 0.0;
	ProjMtx[0][2] = 0.0;
	ProjMtx[0][3] = 0.0;
	ProjMtx[1][0] = 0.0;
	ProjMtx[1][1] = 1.0 / tan_theta;
	ProjMtx[1][2] = 0.0;
	ProjMtx[1][3] = 0.0;
	ProjMtx[2][0] = 0.0;
	ProjMtx[2][1] = 0.0;
	ProjMtx[2][2] = farZ / (farZ - nearZ);
	ProjMtx[2][3] = 1.0;
	ProjMtx[3][0] = 0.0;
	ProjMtx[3][1] = 0.0;
	ProjMtx[3][2] = -farZ * nearZ / (farZ - nearZ);
	ProjMtx[3][3] = 0.0;

	Draw::SetProjViewMtx(ObjVsConstBuf.ProjView, ProjMtx, ViewMtx);
}

EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* path)
{
	SObj* me2 = (SObj*)me_;
	InitClass(&me2->Class, NULL, ObjDtor);
	me2->ElementKinds = NULL;
	me2->Elements = NULL;
	Draw::IdentityFloat(me2->Mtx);
	Draw::IdentityFloat(me2->NormMtx);
	{
		Bool correct = True;
		U8* buf = NULL;
		U32* idces = NULL;
		U8* vertices = NULL;
		for (; ; )
		{
			size_t size;
			buf = static_cast<U8*>(LoadFileAll(reinterpret_cast<const Char*>(path + 0x10), &size, True));
			size_t ptr = 0;
			if (ptr + sizeof(int) > size)
			{
				correct = False;
				break;
			}
			me2->ElementNum = *reinterpret_cast<const int*>(buf + ptr);
			ptr += sizeof(int);
			if (me2->ElementNum < 0)
			{
				correct = False;
				break;
			}
			me2->ElementKinds = static_cast<int*>(AllocMem(sizeof(int) * static_cast<size_t>(me2->ElementNum)));
			me2->Elements = static_cast<void**>(AllocMem(sizeof(void*) * static_cast<size_t>(me2->ElementNum)));
			for (int i = 0; i < me2->ElementNum; i++)
			{
				if (ptr + sizeof(int) > size)
				{
					correct = False;
					break;
				}
				me2->ElementKinds[i] = *reinterpret_cast<const int*>(buf + ptr);
				ptr += sizeof(int);
				switch (me2->ElementKinds[i])
				{
					case 0: // Polygon.
						{
							SObj::SPolygon* element = static_cast<SObj::SPolygon*>(AllocMem(sizeof(SObj::SPolygon)));
							element->VertexBuf = NULL;
							element->Joints = NULL;
							me2->Elements[i] = element;
							if (ptr + sizeof(int) > size)
							{
								correct = False;
								break;
							}
							element->VertexNum = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							idces = static_cast<U32*>(AllocMem(sizeof(U32) * static_cast<size_t>(element->VertexNum)));
							if (ptr + sizeof(U32) * static_cast<size_t>(element->VertexNum) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < element->VertexNum; j++)
							{
								idces[j] = *reinterpret_cast<const U32*>(buf + ptr);
								ptr += sizeof(U32);
							}
							if (ptr + sizeof(int) > size)
							{
								correct = False;
								break;
							}
							int idx_num = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							vertices = static_cast<U8*>(AllocMem((sizeof(float) * 12 + sizeof(int) * 4) * static_cast<size_t>(idx_num)));
							U8* ptr2 = vertices;
							if (ptr + (sizeof(float) * 12 + sizeof(int) * 4) * static_cast<size_t>(idx_num) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < idx_num; j++)
							{
								for (int k = 0; k < 12; k++)
								{
									*reinterpret_cast<float*>(ptr2) = *reinterpret_cast<const float*>(buf + ptr);
									ptr += sizeof(float);
									ptr2 += sizeof(float);
								}
								for (int k = 0; k < 4; k++)
								{
									*reinterpret_cast<int*>(ptr2) = *reinterpret_cast<const int*>(buf + ptr);
									ptr += sizeof(int);
									ptr2 += sizeof(int);
								}
							}
							element->VertexBuf = Draw::MakeVertexBuf((sizeof(float) * 12 + sizeof(int) * 4) * static_cast<size_t>(idx_num), vertices, sizeof(float) * 12 + sizeof(int) * 4, sizeof(U32) * static_cast<size_t>(element->VertexNum), idces);
							if (ptr + sizeof(int) * 3 > size)
							{
								correct = False;
								break;
							}
							element->JointNum = *reinterpret_cast<int*>(buf + ptr);
							ptr += sizeof(int);
							element->Begin = *reinterpret_cast<int*>(buf + ptr);
							ptr += sizeof(int);
							element->End = *reinterpret_cast<int*>(buf + ptr);
							ptr += sizeof(int);
							element->Joints = static_cast<float(*)[4][4]>(AllocMem(sizeof(float[4][4]) * static_cast<size_t>(element->JointNum * (element->End - element->Begin + 1))));
							if (ptr + sizeof(float[4 * 4]) * static_cast<size_t>(element->JointNum * (element->End - element->Begin + 1)) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < element->JointNum; j++)
							{
								for (int k = 0; k < element->End - element->Begin + 1; k++)
								{
									for (int l = 0; l < 4; l++)
									{
										for (int m = 0; m < 4; m++)
										{
											element->Joints[j * (element->End - element->Begin + 1) + k][l][m] = *reinterpret_cast<float*>(buf + ptr);
											ptr += sizeof(float);
										}
									}
								}
							}
						}
						break;
					default:
						ASSERT(False);
						break;
				}
				if (!correct)
					break;
			}
			break;
		}
		if (vertices != NULL)
			FreeMem(vertices);
		if (idces != NULL)
			FreeMem(idces);
		if (buf != NULL)
			FreeMem(buf);
		if (!correct)
		{
			ObjDtor(me_);
			THROW(0x1000, L"");
		}
	}
	return me_;
}

EXPORT_CPP SClass* _makeBox(SClass* me_, double w, double h, double d, double r, double g, double b, double a)
{
	// TODO:
	return NULL;
}

EXPORT_CPP void _objDraw(SClass* me_, SClass* diffuse, SClass* specular, S64 element, double frame)
{
	SObj* me2 = (SObj*)me_;
	ASSERT(0 <= element && element < static_cast<S64>(me2->ElementNum));
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				ASSERT(static_cast<double>(element2->Begin) <= frame && frame < static_cast<double>(element2->End));
				ASSERT(0 <= element2->JointNum && element2->JointNum <= JointMax);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.World, me2->Mtx, sizeof(float[4][4]));
				memcpy(ObjVsConstBuf.NormWorld, me2->NormMtx, sizeof(float[4][4]));
#if defined(DBG)
				ObjPsConstBuf.Mode[0] = 0;
#endif
				if (joint)
				{
					if (static_cast<double>(element2->Begin) <= frame && frame < static_cast<double>(element2->End))
					{
						for (int i = 0; i < element2->JointNum; i++)
						{
							int mtx_a = static_cast<int>(frame);
							int mtx_b = mtx_a + 1;
							float rate_b = static_cast<float>(frame - static_cast<double>(static_cast<int>(frame)));
							float rate_a = 1.0f - rate_b;
							for (int j = 0; j < 4; j++)
							{
								for (int k = 0; k < 4; k++)
									ObjVsConstBuf.Joint[i][j][k] = rate_a * element2->Joints[mtx_a][j][k] + rate_b * element2->Joints[mtx_b][j][k];
							}
						}
					}
					Draw::ConstBuf(ObjJointVs, &ObjVsConstBuf);
				}
				else
				{
					Draw::ConstBuf(ObjVs, &ObjVsConstBuf);
				}
				Device->GSSetShader(NULL);
				Draw::ConstBuf(ObjPs, &ObjPsConstBuf);
				Draw::VertexBuf(element2->VertexBuf);
				Device->PSSetShaderResources(0, 1, &reinterpret_cast<STex*>(diffuse)->View);
				Device->PSSetShaderResources(1, 1, &reinterpret_cast<STex*>(specular)->View);
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

EXPORT_CPP void _objMtx(SClass* me_, const U8* mtx, const U8* normMtx)
{
	SObj* me2 = (SObj*)me_;
	if (*(S64*)(mtx + 0x08) != 16 || *(S64*)(normMtx + 0x08) != 16)
		THROW(0x1000, L"");
	{
		const double* ptr = reinterpret_cast<const double*>(mtx + 0x10);
		me2->Mtx[0][0] = static_cast<float>(ptr[0]);
		me2->Mtx[0][1] = static_cast<float>(ptr[1]);
		me2->Mtx[0][2] = static_cast<float>(ptr[2]);
		me2->Mtx[0][3] = static_cast<float>(ptr[3]);
		me2->Mtx[1][0] = static_cast<float>(ptr[4]);
		me2->Mtx[1][1] = static_cast<float>(ptr[5]);
		me2->Mtx[1][2] = static_cast<float>(ptr[6]);
		me2->Mtx[1][3] = static_cast<float>(ptr[7]);
		me2->Mtx[2][0] = static_cast<float>(ptr[8]);
		me2->Mtx[2][1] = static_cast<float>(ptr[9]);
		me2->Mtx[2][2] = static_cast<float>(ptr[10]);
		me2->Mtx[2][3] = static_cast<float>(ptr[11]);
		me2->Mtx[3][0] = static_cast<float>(ptr[12]);
		me2->Mtx[3][1] = static_cast<float>(ptr[13]);
		me2->Mtx[3][2] = static_cast<float>(ptr[14]);
		me2->Mtx[3][3] = static_cast<float>(ptr[15]);
	}
	{
		const double* ptr = reinterpret_cast<const double*>(normMtx + 0x10);
		me2->NormMtx[0][0] = static_cast<float>(ptr[0]);
		me2->NormMtx[0][1] = static_cast<float>(ptr[1]);
		me2->NormMtx[0][2] = static_cast<float>(ptr[2]);
		me2->NormMtx[0][3] = static_cast<float>(ptr[3]);
		me2->NormMtx[1][0] = static_cast<float>(ptr[4]);
		me2->NormMtx[1][1] = static_cast<float>(ptr[5]);
		me2->NormMtx[1][2] = static_cast<float>(ptr[6]);
		me2->NormMtx[1][3] = static_cast<float>(ptr[7]);
		me2->NormMtx[2][0] = static_cast<float>(ptr[8]);
		me2->NormMtx[2][1] = static_cast<float>(ptr[9]);
		me2->NormMtx[2][2] = static_cast<float>(ptr[10]);
		me2->NormMtx[2][3] = static_cast<float>(ptr[11]);
		me2->NormMtx[3][0] = static_cast<float>(ptr[12]);
		me2->NormMtx[3][1] = static_cast<float>(ptr[13]);
		me2->NormMtx[3][2] = static_cast<float>(ptr[14]);
		me2->NormMtx[3][3] = static_cast<float>(ptr[15]);
	}
}

EXPORT_CPP void _objPos(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ)
{
	SObj* me2 = (SObj*)me_;
	double cos_x = cos(rotX);
	double sin_x = sin(rotX);
	double cos_y = cos(rotY);
	double sin_y = sin(rotY);
	double cos_z = cos(rotZ);
	double sin_z = sin(rotZ);
	me2->Mtx[0][0] = static_cast<float>(scaleX * (cos_y * cos_z));
	me2->Mtx[0][1] = static_cast<float>(scaleX * (cos_y * sin_z));
	me2->Mtx[0][2] = static_cast<float>(scaleX * (-sin_y));
	me2->Mtx[0][3] = 0.0f;
	me2->Mtx[1][0] = static_cast<float>(scaleY * (sin_x * sin_y * cos_z - cos_x * sin_z));
	me2->Mtx[1][1] = static_cast<float>(scaleY * (sin_x * sin_y * sin_z + cos_x * cos_z));
	me2->Mtx[1][2] = static_cast<float>(scaleY * (sin_x * cos_y));
	me2->Mtx[1][3] = 0.0f;
	me2->Mtx[2][0] = static_cast<float>(scaleZ * (cos_x * sin_y * cos_z + sin_x * sin_z));
	me2->Mtx[2][1] = static_cast<float>(scaleZ * (cos_x * sin_y * sin_z - sin_x * cos_z));
	me2->Mtx[2][2] = static_cast<float>(scaleZ * (cos_x * cos_y));
	me2->Mtx[2][3] = 0.0f;
	me2->Mtx[3][0] = static_cast<float>(transX);
	me2->Mtx[3][1] = static_cast<float>(transY);
	me2->Mtx[3][2] = static_cast<float>(transZ);
	me2->Mtx[3][3] = 1.0f;
	scaleX = 1.0 / scaleX;
	scaleY = 1.0 / scaleY;
	scaleZ = 1.0 / scaleZ;
	me2->NormMtx[0][0] = static_cast<float>(scaleX * (cos_y * cos_z));
	me2->NormMtx[0][1] = static_cast<float>(scaleX * (cos_y * sin_z));
	me2->NormMtx[0][2] = static_cast<float>(scaleX * (-sin_y));
	me2->NormMtx[0][3] = 0.0f;
	me2->NormMtx[1][0] = static_cast<float>(scaleY * (sin_x * sin_y * cos_z - cos_x * sin_z));
	me2->NormMtx[1][1] = static_cast<float>(scaleY * (sin_x * sin_y * sin_z + cos_x * cos_z));
	me2->NormMtx[1][2] = static_cast<float>(scaleY * (sin_x * cos_y));
	me2->NormMtx[1][3] = 0.0f;
	me2->NormMtx[2][0] = static_cast<float>(scaleZ * (cos_x * sin_y * cos_z + sin_x * sin_z));
	me2->NormMtx[2][1] = static_cast<float>(scaleZ * (cos_x * sin_y * sin_z - sin_x * cos_z));
	me2->NormMtx[2][2] = static_cast<float>(scaleZ * (cos_x * cos_y));
	me2->NormMtx[2][3] = 0.0f;
	me2->NormMtx[3][0] = 0.0f;
	me2->NormMtx[3][1] = 0.0f;
	me2->NormMtx[3][2] = 0.0f;
	me2->NormMtx[3][3] = 1.0f;
}

EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ, Bool fixUp)
{
	SObj* me2 = (SObj*)me_;
	double at[3] = { atX - x, atY - y, atZ - z }, up[3] = { upX, upY, upZ }, right[3];
	if (Draw::Normalize(at) == 0.0)
		THROW(0x1000, L"");
	if (Draw::Normalize(up) == 0.0)
		THROW(0x1000, L"");
	Draw::Cross(right, up, at);
	if (Draw::Normalize(right) == 0.0)
		THROW(0x1000, L"");
	if (fixUp)
		Draw::Cross(up, at, right);
	else
		Draw::Cross(at, right, up);
	me2->Mtx[0][0] = static_cast<float>(right[0]);
	me2->Mtx[0][1] = static_cast<float>(right[1]);
	me2->Mtx[0][2] = static_cast<float>(right[2]);
	me2->Mtx[0][3] = 0.0f;
	me2->Mtx[1][0] = static_cast<float>(up[0]);
	me2->Mtx[1][1] = static_cast<float>(up[1]);
	me2->Mtx[1][2] = static_cast<float>(up[2]);
	me2->Mtx[1][3] = 0.0f;
	me2->Mtx[2][0] = static_cast<float>(at[0]);
	me2->Mtx[2][1] = static_cast<float>(at[1]);
	me2->Mtx[2][2] = static_cast<float>(at[2]);
	me2->Mtx[2][3] = 0.0f;
	me2->Mtx[3][0] = static_cast<float>(x);
	me2->Mtx[3][1] = static_cast<float>(y);
	me2->Mtx[3][2] = static_cast<float>(z);
	me2->Mtx[3][3] = 1.0f;
	me2->NormMtx[0][0] = static_cast<float>(right[0]);
	me2->NormMtx[0][1] = static_cast<float>(right[1]);
	me2->NormMtx[0][2] = static_cast<float>(right[2]);
	me2->NormMtx[0][3] = 0.0f;
	me2->NormMtx[1][0] = static_cast<float>(up[0]);
	me2->NormMtx[1][1] = static_cast<float>(up[1]);
	me2->NormMtx[1][2] = static_cast<float>(up[2]);
	me2->NormMtx[1][3] = 0.0f;
	me2->NormMtx[2][0] = static_cast<float>(at[0]);
	me2->NormMtx[2][1] = static_cast<float>(at[1]);
	me2->NormMtx[2][2] = static_cast<float>(at[2]);
	me2->NormMtx[2][3] = 0.0f;
	me2->NormMtx[3][0] = 0.0f;
	me2->NormMtx[3][1] = 0.0f;
	me2->NormMtx[3][2] = 0.0f;
	me2->NormMtx[3][3] = 1.0f;
}

EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ, Bool fixUp)
{
	_objLook(me_, x, y, z, static_cast<double>(ObjVsConstBuf.Eye[0]), static_cast<double>(ObjVsConstBuf.Eye[1]), static_cast<double>(ObjVsConstBuf.Eye[2]), upX, upY, upZ, fixUp);
}

namespace Draw
{

void Init()
{
	if (FAILED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &Device)))
		THROW(0x1000, L"");

	// Create a rasterizer state.
	{
		D3D10_RASTERIZER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.FillMode = D3D10_FILL_SOLID;
		desc.CullMode = D3D10_CULL_FRONT; //D3D10_CULL_NONE; // TODO: D3D10_CULL_BACK;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = FALSE;
		desc.ScissorEnable = FALSE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerState)))
			THROW(0x1000, L"");
	}

	// Create depth buffer modes.
	for (int i = 0; i < DepthNum; i++)
	{
		D3D10_DEPTH_STENCIL_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.DepthFunc = D3D10_COMPARISON_LESS_EQUAL;
		desc.StencilEnable = FALSE;
		switch (i)
		{
			// Disable test, disable writing.
			case 0:
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
				break;
			// Disable test, enable writing.
			case 1:
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
				break;
			// Enable test, disable writing.
			case 2:
				desc.DepthEnable = TRUE;
				desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
				break;
			// Enable test, enable writing.
			case 3:
				desc.DepthEnable = TRUE;
				desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
				break;
			default:
				ASSERT(False);
				break;
		}
		if (FAILED(Device->CreateDepthStencilState(&desc, &DepthState[i])))
			THROW(0x1000, L"");
	}

	// Create blend modes.
	for (int i = 0; i < BlendNum; i++)
	{
		D3D10_BLEND_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.AlphaToCoverageEnable = FALSE;
		desc.BlendEnable[0] = TRUE;
		desc.SrcBlendAlpha = D3D10_BLEND_ONE;
		desc.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
		desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
		desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
		for (int j = 1; j < 8; j++)
		{
			desc.BlendEnable[j] = FALSE;
			desc.RenderTargetWriteMask[j] = 0;
		}
		switch (i)
		{
			// None: S * 1 + D * 0.
			case 0:
				desc.BlendEnable[0] = FALSE;
				desc.SrcBlend = D3D10_BLEND_ONE;
				desc.DestBlend = D3D10_BLEND_ZERO;
				desc.BlendOp = D3D10_BLEND_OP_ADD;
				break;
			// Alpha: S * A + D * (1 - A).
			case 1:
				desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
				desc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
				desc.BlendOp = D3D10_BLEND_OP_ADD;
				break;
			// Add: S * A + D * 1.
			case 2:
				desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
				desc.DestBlend = D3D10_BLEND_ONE;
				desc.BlendOp = D3D10_BLEND_OP_ADD;
				break;
			// Sub: S * A - D * 1.
			case 3:
				desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
				desc.DestBlend = D3D10_BLEND_ONE;
				desc.BlendOp = D3D10_BLEND_OP_REV_SUBTRACT;
				break;
			// Mul: S * D + D * 0.
			case 4:
				desc.SrcBlend = D3D10_BLEND_DEST_COLOR;
				desc.DestBlend = D3D10_BLEND_ZERO;
				desc.BlendOp = D3D10_BLEND_OP_ADD;
				break;
			default:
				ASSERT(False);
				break;
		}
		if (FAILED(Device->CreateBlendState(&desc, &BlendState[i])))
			THROW(0x1000, L"");
	}

	// Create a sampler.
	for (int i = 0; i < SamplerNum; i++)
	{
		D3D10_SAMPLER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		switch (i)
		{
			// Nearest neighbor.
			case 0:
				desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
				break;
			// Bilinear.
			case 1:
				desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
				break;
		}
		desc.AddressU = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.AddressV = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.AddressW = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 0;
		desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D10_FLOAT32_MAX;
		if (FAILED(Device->CreateSamplerState(&desc, &Sampler[i])))
			THROW(0x1000, L"");
	}

	// Initialize 'Tri'.
	{
		{
			float vertices[] =
			{
				1.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
			};

			U32 idces[] =
			{
				0, 1, 2,
			};

			TriVertex = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 3, sizeof(idces), idces);
		}

		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float3,
			};

			const Char* layout_semantics[1] =
			{
				L"K_WEIGHT",
			};

			{
				size_t size;
				const U8* bin = GetTriVsBin(&size);
				TriVs = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 8, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetTriPsBin(&size);
				TriPs = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Rect'.
	{
		{
			float vertices[] =
			{
				0.0, 0.0,
				1.0, 0.0,
				0.0, 1.0,
				1.0, 1.0,
			};

			U32 idces[] =
			{
				0, 1, 2,
				3, 2, 1,
			};

			RectVertex = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
		}

		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float2,
			};

			const Char* layout_semantics[1] =
			{
				L"K_WEIGHT",
			};

			{
				size_t size;
				const U8* bin = GetRectVsBin(&size);
				RectVs = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 4, 1, layout_types, layout_semantics);
			}
		}
	}

	// Initialize 'Circle'.
	{
		{
			float vertices[] =
			{
				-1.0f, -1.0f,
				1.0f, -1.0f,
				-1.0f, 1.0f,
				1.0f, 1.0f,
			};

			U32 idces[] =
			{
				0, 1, 2,
				3, 2, 1,
			};

			CircleVertex = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
		}

		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float2,
			};

			const Char* layout_semantics[1] =
			{
				L"K_WEIGHT",
			};

			{
				size_t size;
				const U8* bin = GetCircleVsBin(&size);
				CircleVs = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 4, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetCirclePsBin(&size);
				CirclePs = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Tex'.
	{
		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float2,
			};

			const Char* layout_semantics[1] =
			{
				L"K_WEIGHT",
			};

			{
				size_t size;
				const U8* bin = GetTexVsBin(&size);
				TexVs = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 8, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetTexPsBin(&size);
				TexPs = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Obj'.
	{
		{
			ELayoutType layout_types[5] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
			};

			const Char* layout_semantics[5] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"BINORMAL",
				L"TEXCOORD",
			};

			{
				size_t size;
				const U8* bin = GetObjVsBin(&size);
				ObjVs = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 5, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjPsBin(&size);
				ObjPs = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, NULL, NULL);
			}
		}
		{
			ELayoutType layout_types[7] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[7] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"BINORMAL",
				L"TEXCOORD",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjJointVsBin(&size);
				ObjJointVs = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjJointVsConstBuf), 7, layout_types, layout_semantics);
			}
		}
	}

	memset(&ObjVsConstBuf, 0, sizeof(SObjVsConstBuf));
	memset(&ObjPsConstBuf, 0, sizeof(SObjPsConstBuf));
	_camera(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	_proj(M_PI / 180.0 * 27.0, 16.0, 9.0, 0.01, 1000.0); // The angle of view of a 50mm lens is 27 degrees.
	_camera(100.0, 150.0, 250.0, 20.0, 30.0, 0.0, 0.0, 1.0, 0.0);
	_proj(M_PI / 180.0 * 27.0, 16.0, 9.0, 0.01, 1000.0); // The angle of view of a 50mm lens is 27 degrees.
	{
		double dir[3] = { 2.0, 5.0, 1.0 };
		Draw::Normalize(dir);
		ObjPsConstBuf.Dir[0] = static_cast<float>(dir[0]);
		ObjPsConstBuf.Dir[1] = static_cast<float>(dir[1]);
		ObjPsConstBuf.Dir[2] = static_cast<float>(dir[2]);
		ObjPsConstBuf.Dir[3] = 0.0f;
	}
	ObjPsConstBuf.DirColor[0] = 1.5f;
	ObjPsConstBuf.DirColor[1] = 1.5f;
	ObjPsConstBuf.DirColor[2] = 1.5f;
	ObjPsConstBuf.DirColor[3] = 0.0f;

	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Device->RSSetState(RasterizerState);
	_depth(False, False);
	_depth(True, True);
	_blend(1);
	_sampler(1);
}

void Fin()
{
	if (ObjPs != NULL)
		FinShaderBuf(ObjPs);
	if (ObjJointVs != NULL)
		FinShaderBuf(ObjJointVs);
	if (ObjVs != NULL)
		FinShaderBuf(ObjVs);
	if (TexPs != NULL)
		FinShaderBuf(TexPs);
	if (TexVs != NULL)
		FinShaderBuf(TexVs);
	if (CirclePs != NULL)
		FinShaderBuf(CirclePs);
	if (CircleVs != NULL)
		FinShaderBuf(CircleVs);
	if (CircleVertex != NULL)
		FinVertexBuf(CircleVertex);
	if (RectVs != NULL)
		FinShaderBuf(RectVs);
	if (RectVertex != NULL)
		FinVertexBuf(RectVertex);
	if (TriPs != NULL)
		FinShaderBuf(TriPs);
	if (TriVs != NULL)
		FinShaderBuf(TriVs);
	if (TriVertex != NULL)
		FinVertexBuf(TriVertex);
	for (int i = 0; i < SamplerNum; i++)
	{
		if (Sampler[i] != NULL)
			Sampler[i]->Release();
	}
	for (int i = 0; i < BlendNum; i++)
	{
		if (BlendState[i] != NULL)
			BlendState[i]->Release();
	}
	for (int i = 0; i < DepthNum; i++)
	{
		if (DepthState[i] != NULL)
			DepthState[i]->Release();
	}
	if (RasterizerState != NULL)
		RasterizerState->Release();
	if (Device != NULL)
		Device->Release();
}

void* MakeWndBuf(int width, int height, HWND wnd)
{
	SWndBuf* wnd_buf = static_cast<SWndBuf*>(AllocMem(sizeof(SWndBuf)));
	memset(wnd_buf, 0, sizeof(SWndBuf));
	wnd_buf->ClearColor[4] = 1.0f;
	wnd_buf->Width = width;
	wnd_buf->Height = height;

	// Create a swap chain.
	{
		IDXGIFactory* factory = NULL;
		DXGI_SWAP_CHAIN_DESC desc;
		Bool success = False;
		for (; ; )
		{
			if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
				break;
			desc.BufferDesc.Width = static_cast<UINT>(width);
			desc.BufferDesc.Height = static_cast<UINT>(height);
			desc.BufferDesc.RefreshRate.Numerator = 60;
			desc.BufferDesc.RefreshRate.Denominator = 1;
			desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			desc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = 1;
			desc.OutputWindow = wnd;
			desc.Windowed = TRUE;
			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			if (FAILED(factory->CreateSwapChain(Device, &desc, &wnd_buf->SwapChain)))
				break;
			success = True;
			break;
		}
		if (factory != NULL)
			factory->Release();
		if (!success)
			THROW(0x1000, L"");
	}

	// Create a back buffer and a depth buffer.
	{
		ID3D10Texture2D* back = NULL;
		ID3D10Texture2D* depth_stencil = NULL;
		Bool success = False;
		for (; ; )
		{
			if (FAILED(wnd_buf->SwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void**>(&back))))
				break;
			if (FAILED(Device->CreateRenderTargetView(back, NULL, &wnd_buf->RenderTargetView)))
				break;
			{
				D3D10_TEXTURE2D_DESC desc;
				memset(&desc, 0, sizeof(desc));
				desc.Width = static_cast<UINT>(width);
				desc.Height = static_cast<UINT>(height);
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_D32_FLOAT;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = D3D10_USAGE_DEFAULT;
				desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				if (FAILED(Device->CreateTexture2D(&desc, NULL, &depth_stencil)))
					break;
			}
			{
				D3D10_DEPTH_STENCIL_VIEW_DESC desc;
				memset(&desc, 0, sizeof(desc));
				desc.Format = DXGI_FORMAT_D32_FLOAT;
				desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = 0;
				if (FAILED(Device->CreateDepthStencilView(depth_stencil, &desc, &wnd_buf->DepthView)))
					break;
			}
			success = True;
			break;
		}
		if (depth_stencil != NULL)
			depth_stencil->Release();
		if (back != NULL)
			back->Release();
		if (!success)
			THROW(0x1000, L"");
	}

	CurWndBuf = wnd_buf;
	Device->OMSetRenderTargets(1, &CurWndBuf->RenderTargetView, CurWndBuf->DepthView); // TODO:
	_resetViewport(); // TODO:
	return wnd_buf;
}

void FinWndBuf(void* wnd_buf)
{
	SWndBuf* wnd_buf2 = static_cast<SWndBuf*>(wnd_buf);
	if (wnd_buf2->DepthView != NULL)
		wnd_buf2->DepthView->Release();
	if (wnd_buf2->RenderTargetView != NULL)
		wnd_buf2->RenderTargetView->Release();
	if (wnd_buf2->SwapChain != NULL)
		wnd_buf2->SwapChain->Release();
	FreeMem(wnd_buf);
}

void* MakeShaderBuf(EShaderKind kind, size_t size, const void* bin, size_t const_buf_size, int layout_num, const ELayoutType* layout_types, const Char** layout_semantics)
{
	SShaderBuf* shader_buf = static_cast<SShaderBuf*>(AllocMem(sizeof(SShaderBuf)));
	ASSERT(const_buf_size % 16 == 0);
	switch (kind)
	{
		case ShaderKind_Vs:
			if (FAILED(Device->CreateVertexShader(bin, size, reinterpret_cast<ID3D10VertexShader**>(&shader_buf->Shader))))
				ASSERT(False);
			break;
		case ShaderKind_Gs:
			if (FAILED(Device->CreateGeometryShader(bin, size, reinterpret_cast<ID3D10GeometryShader**>(&shader_buf->Shader))))
				ASSERT(False);
			break;
		case ShaderKind_Ps:
			if (FAILED(Device->CreatePixelShader(bin, size, reinterpret_cast<ID3D10PixelShader**>(&shader_buf->Shader))))
				ASSERT(False);
			break;
		default:
			ASSERT(False);
			break;
	}
	shader_buf->Kind = kind;
	shader_buf->ConstBufSize = const_buf_size;

	{
		D3D10_BUFFER_DESC desc;
		desc.ByteWidth = static_cast<UINT>(const_buf_size);
		desc.Usage = D3D10_USAGE_DYNAMIC;
		desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateBuffer(&desc, NULL, &shader_buf->ConstBuf)))
			ASSERT(False);
	}

	if (layout_num == 0)
		shader_buf->Layout = NULL;
	else
	{
		D3D10_INPUT_ELEMENT_DESC* descs = static_cast<D3D10_INPUT_ELEMENT_DESC*>(AllocMem(sizeof(D3D10_INPUT_ELEMENT_DESC) * static_cast<size_t>(layout_num)));
		char(*semantics)[33] = static_cast<char(*)[33]>(AllocMem(sizeof(char[33]) * static_cast<size_t>(layout_num)));
		size_t offset = 0;
		for (int i = 0; i < layout_num; i++)
		{
			{
				size_t len = wcslen(layout_semantics[i]);
				ASSERT(len <= 32);
				for (int j = 0; j < len; j++)
					semantics[i][j] = static_cast<char>(layout_semantics[i][j]);
				semantics[i][len] = '\0';
				descs[i].SemanticName = semantics[i];
				descs[i].SemanticIndex = 0;
			}
			{
				DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
				size_t size2;
				switch (layout_types[i])
				{
					case LayoutType_Int1:
						format = DXGI_FORMAT_R8_SINT;
						size2 = sizeof(int);
						break;
					case LayoutType_Int2:
						format = DXGI_FORMAT_R8G8_SINT;
						size2 = sizeof(int) * 2;
						break;
					case LayoutType_Int4:
						format = DXGI_FORMAT_R8G8B8A8_SINT;
						size2 = sizeof(int) * 4;
						break;
					case LayoutType_Float1:
						format = DXGI_FORMAT_R32_FLOAT;
						size2 = sizeof(float);
						break;
					case LayoutType_Float2:
						format = DXGI_FORMAT_R32G32_FLOAT;
						size2 = sizeof(float) * 2;
						break;
					case LayoutType_Float3:
						format = DXGI_FORMAT_R32G32B32_FLOAT;
						size2 = sizeof(float) * 3;
						break;
					case LayoutType_Float4:
						format = DXGI_FORMAT_R32G32B32A32_FLOAT;
						size2 = sizeof(float) * 4;
						break;
					default:
						ASSERT(False);
						size2 = 0;
						break;
				}
				descs[i].Format = format;
				descs[i].InputSlot = 0;
				descs[i].AlignedByteOffset = static_cast<UINT>(offset);
				descs[i].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
				descs[i].InstanceDataStepRate = 0;
				offset += size2;
			}
		}
		if (FAILED(Device->CreateInputLayout(descs, static_cast<UINT>(layout_num), bin, size, &shader_buf->Layout)))
			ASSERT(False);
		FreeMem(semantics);
		FreeMem(descs);
	}

	return shader_buf;
}

void FinShaderBuf(void* shader_buf)
{
	SShaderBuf* shader_buf2 = static_cast<SShaderBuf*>(shader_buf);
	if (shader_buf2->Layout != NULL)
		shader_buf2->Layout->Release();
	if (shader_buf2->ConstBuf != NULL)
		shader_buf2->ConstBuf->Release();
	if (shader_buf2->Shader != NULL)
	{
		switch (shader_buf2->Kind)
		{
			case ShaderKind_Vs:
				static_cast<ID3D10VertexShader*>(shader_buf2->Shader)->Release();
				break;
			case ShaderKind_Gs:
				static_cast<ID3D10GeometryShader*>(shader_buf2->Shader)->Release();
				break;
			case ShaderKind_Ps:
				static_cast<ID3D10PixelShader*>(shader_buf2->Shader)->Release();
				break;
			default:
				ASSERT(False);
				break;
		}
	}
	FreeMem(shader_buf);
}

void ConstBuf(void* shader_buf, const void* data)
{
	SShaderBuf* shader_buf2 = static_cast<SShaderBuf*>(shader_buf);
	void* buf;
	if (shader_buf2->ConstBuf->Map(D3D10_MAP_WRITE_DISCARD, 0, &buf))
		ASSERT(False);
	memcpy(buf, data, shader_buf2->ConstBufSize);
	shader_buf2->ConstBuf->Unmap();

	switch (shader_buf2->Kind)
	{
		case ShaderKind_Vs:
			Device->VSSetConstantBuffers(0, 1, &shader_buf2->ConstBuf);
			Device->VSSetShader(static_cast<ID3D10VertexShader*>(shader_buf2->Shader));
			break;
		case ShaderKind_Gs:
			Device->GSSetConstantBuffers(0, 1, &shader_buf2->ConstBuf);
			Device->GSSetShader(static_cast<ID3D10GeometryShader*>(shader_buf2->Shader));
			break;
		case ShaderKind_Ps:
			Device->PSSetConstantBuffers(0, 1, &shader_buf2->ConstBuf);
			Device->PSSetShader(static_cast<ID3D10PixelShader*>(shader_buf2->Shader));
			break;
		default:
			ASSERT(False);
			break;
	}

	if (shader_buf2->Layout != NULL)
		Device->IASetInputLayout(shader_buf2->Layout);
}

void VertexBuf(void* vertex_buf)
{
	SVertexBuf* vertex_buf2 = static_cast<SVertexBuf*>(vertex_buf);
	const UINT stride = static_cast<UINT>(vertex_buf2->VertexLineSize);
	const UINT offset = 0;
	Device->IASetVertexBuffers(0, 1, &vertex_buf2->Vertex, &stride, &offset);
	Device->IASetIndexBuffer(vertex_buf2->Idx, DXGI_FORMAT_R32_UINT, 0);
}

void* MakeVertexBuf(size_t vertex_size, const void* vertices, size_t vertex_line_size, size_t idx_size, const U32* idces)
{
	SVertexBuf* vertex_buf = static_cast<SVertexBuf*>(AllocMem(sizeof(SVertexBuf)));

	{
		D3D10_BUFFER_DESC desc;
		desc.ByteWidth = static_cast<UINT>(vertex_size);
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D10_SUBRESOURCE_DATA sub;
		sub.pSysMem = vertices;
		sub.SysMemPitch = 0;
		sub.SysMemSlicePitch = 0;

		if (FAILED(Device->CreateBuffer(&desc, &sub, &vertex_buf->Vertex)))
			ASSERT(False);
	}

	vertex_buf->VertexLineSize = vertex_line_size;

	{
		D3D10_BUFFER_DESC desc;
		desc.ByteWidth = static_cast<UINT>(idx_size);
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D10_SUBRESOURCE_DATA sub;
		sub.pSysMem = idces;
		sub.SysMemPitch = 0;
		sub.SysMemSlicePitch = 0;

		if (FAILED(Device->CreateBuffer(&desc, &sub, &vertex_buf->Idx)))
			ASSERT(False);
	}

	return vertex_buf;
}

void FinVertexBuf(void* vertex_buf)
{
	SVertexBuf* vertex_buf2 = static_cast<SVertexBuf*>(vertex_buf);
	if (vertex_buf2->Idx != NULL)
		vertex_buf2->Idx->Release();
	if (vertex_buf2->Vertex != NULL)
		vertex_buf2->Vertex->Release();
	FreeMem(vertex_buf);
}

void Identity(double mtx[4][4])
{
	mtx[0][0] = 1.0;
	mtx[0][1] = 0.0;
	mtx[0][2] = 0.0;
	mtx[0][3] = 0.0;
	mtx[1][0] = 0.0;
	mtx[1][1] = 1.0;
	mtx[1][2] = 0.0;
	mtx[1][3] = 0.0;
	mtx[2][0] = 0.0;
	mtx[2][1] = 0.0;
	mtx[2][2] = 1.0;
	mtx[2][3] = 0.0;
	mtx[3][0] = 0.0;
	mtx[3][1] = 0.0;
	mtx[3][2] = 0.0;
	mtx[3][3] = 1.0;
}

void IdentityFloat(float mtx[4][4])
{
	mtx[0][0] = 1.0f;
	mtx[0][1] = 0.0f;
	mtx[0][2] = 0.0f;
	mtx[0][3] = 0.0f;
	mtx[1][0] = 0.0f;
	mtx[1][1] = 1.0f;
	mtx[1][2] = 0.0f;
	mtx[1][3] = 0.0f;
	mtx[2][0] = 0.0f;
	mtx[2][1] = 0.0f;
	mtx[2][2] = 1.0f;
	mtx[2][3] = 0.0f;
	mtx[3][0] = 0.0f;
	mtx[3][1] = 0.0f;
	mtx[3][2] = 0.0f;
	mtx[3][3] = 1.0f;
}

double Normalize(double vec[3])
{
	double len = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	if (len != 0.0)
	{
		vec[0] /= len;
		vec[1] /= len;
		vec[2] /= len;
	}
	return len;
}

double Dot(const double a[3], const double b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Cross(double out[3], const double a[3], const double b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void SetProjViewMtx(float out[4][4], const double proj[4][4], const double view[4][4])
{
	out[0][0] = static_cast<float>(proj[0][0] * view[0][0] + proj[1][0] * view[0][1] + proj[2][0] * view[0][2] + proj[3][0] * view[0][3]);
	out[0][1] = static_cast<float>(proj[0][1] * view[0][0] + proj[1][1] * view[0][1] + proj[2][1] * view[0][2] + proj[3][1] * view[0][3]);
	out[0][2] = static_cast<float>(proj[0][2] * view[0][0] + proj[1][2] * view[0][1] + proj[2][2] * view[0][2] + proj[3][2] * view[0][3]);
	out[0][3] = static_cast<float>(proj[0][3] * view[0][0] + proj[1][3] * view[0][1] + proj[2][3] * view[0][2] + proj[3][3] * view[0][3]);
	out[1][0] = static_cast<float>(proj[0][0] * view[1][0] + proj[1][0] * view[1][1] + proj[2][0] * view[1][2] + proj[3][0] * view[1][3]);
	out[1][1] = static_cast<float>(proj[0][1] * view[1][0] + proj[1][1] * view[1][1] + proj[2][1] * view[1][2] + proj[3][1] * view[1][3]);
	out[1][2] = static_cast<float>(proj[0][2] * view[1][0] + proj[1][2] * view[1][1] + proj[2][2] * view[1][2] + proj[3][2] * view[1][3]);
	out[1][3] = static_cast<float>(proj[0][3] * view[1][0] + proj[1][3] * view[1][1] + proj[2][3] * view[1][2] + proj[3][3] * view[1][3]);
	out[2][0] = static_cast<float>(proj[0][0] * view[2][0] + proj[1][0] * view[2][1] + proj[2][0] * view[2][2] + proj[3][0] * view[2][3]);
	out[2][1] = static_cast<float>(proj[0][1] * view[2][0] + proj[1][1] * view[2][1] + proj[2][1] * view[2][2] + proj[3][1] * view[2][3]);
	out[2][2] = static_cast<float>(proj[0][2] * view[2][0] + proj[1][2] * view[2][1] + proj[2][2] * view[2][2] + proj[3][2] * view[2][3]);
	out[2][3] = static_cast<float>(proj[0][3] * view[2][0] + proj[1][3] * view[2][1] + proj[2][3] * view[2][2] + proj[3][3] * view[2][3]);
	out[3][0] = static_cast<float>(proj[0][0] * view[3][0] + proj[1][0] * view[3][1] + proj[2][0] * view[3][2] + proj[3][0] * view[3][3]);
	out[3][1] = static_cast<float>(proj[0][1] * view[3][0] + proj[1][1] * view[3][1] + proj[2][1] * view[3][2] + proj[3][1] * view[3][3]);
	out[3][2] = static_cast<float>(proj[0][2] * view[3][0] + proj[1][2] * view[3][1] + proj[2][2] * view[3][2] + proj[3][2] * view[3][3]);
	out[3][3] = static_cast<float>(proj[0][3] * view[3][0] + proj[1][3] * view[3][1] + proj[2][3] * view[3][2] + proj[3][3] * view[3][3]);
}

} // namespace Draw

static void TexDtor(SClass* me_)
{
	STex* me2 = reinterpret_cast<STex*>(me_);
	if (me2->View != NULL)
		me2->View->Release();
	if (me2->Tex != NULL)
		me2->Tex->Release();
}

static void ObjDtor(SClass* me_)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	for (int i = 0; i < me2->ElementNum; i++)
	{
		switch (me2->ElementKinds[i])
		{
			case 0: // Polygon.
				{
					SObj::SPolygon* element = static_cast<SObj::SPolygon*>(me2->Elements[i]);
					if (element->Joints != NULL)
						FreeMem(element->Joints);
					if (element->VertexBuf != NULL)
						Draw::FinVertexBuf(element->VertexBuf);
					FreeMem(element);
				}
				break;
			default:
				ASSERT(False);
				break;
		}
	}
	if (me2->Elements != NULL)
		FreeMem(me2->Elements);
	if (me2->ElementKinds != NULL)
		FreeMem(me2->ElementKinds);
}
