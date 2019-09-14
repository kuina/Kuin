#include "draw.h"

// DirectX 10 is preinstalled on Windows Vista or later.
#pragma comment(lib, "d3d10_1.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d10_1.h>

#include "png_decoder.h"
#include "jpg_decoder.h"
#include "bc_decoder.h"

static const int DepthNum = 4;
static const int BlendNum = 6;
static const int SamplerNum = 3;
static const int JointMax = 256;
static const int FontBitmapSize = 512;
static const int TexEvenNum = 3;
static const double DiscardAlpha = 0.02;
static const int ParticleNum = 256;
static const int ParticleTexNum = 3;
static const int PolyVerticesNum = 64;
static const int JointInfluenceMax = 2;
static const int JointInfluenceMaxAligned = 4;

struct SWndBuf
{
	void* Extra;
	IDXGISwapChain* SwapChain;
	ID3D10RenderTargetView* RenderTargetView;
	ID3D10DepthStencilView* DepthView;
	FLOAT ClearColor[4];
	int TexWidth;
	int TexHeight;
	ID3D10Texture2D* TmpTex;
	ID3D10Texture2D* EditableTex;
	ID3D10ShaderResourceView* TmpShaderResView;
	ID3D10RenderTargetView* TmpRenderTargetView;
	Bool AutoClear;
	Bool Editable;
	int Split;
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
	int Width;
	int Height;
	int ImgWidth;
	int ImgHeight;
	ID3D10Texture2D* Tex;
	ID3D10ShaderResourceView* View;
};

struct SFont
{
	SClass Class;
	ID3D10Texture2D* Tex;
	ID3D10ShaderResourceView* View;
	int CellWidth;
	int CellHeight;
	int CellSizeAligned;
	U32 Cnt;
	double Advance;
	double Height;
	bool Proportional;
	U8 AlignHorizontal;
	U8 AlignVertical;
	HFONT Font;
	Char* CharMap;
	U32* CntMap;
	U8* Pixel;
	HBITMAP Bitmap;
	HDC Dc;
	int* GlyphWidth;
};

enum EFormat
{
	Format_HasTangent = 0x01,
	Format_HasJoint = 0x02,
};

struct SObj
{
	SClass Class;

	struct SPolygon
	{
		void* VertexBuf;
		int VertexNum;
		int JointNum;
		int Begin;
		int End;
		float(*Joints)[4][4];
	};

	EFormat Format;
	int ElementNum;
	int* ElementKinds;
	void** Elements;
	float Mat[4][4];
	float NormMat[4][4];
};

struct SObjCommonVsConstBuf
{
	float World[4][4];
	float NormWorld[4][4];
	float ProjView[4][4];
	float ShadowProjView[4][4];
	float Eye[4];
	float Dir[4];
};
STATIC_ASSERT(sizeof(SObjCommonVsConstBuf) == 4 * 72);

struct SObjCommonPsConstBuf
{
	float AmbTopColor[4];
	float AmbBottomColor[4];
	float DirColor[4];
};
STATIC_ASSERT(sizeof(SObjCommonPsConstBuf) == 4 * 12);

struct SObjVsConstBuf
{
	SObjCommonVsConstBuf CommonParam;
	float Joint[JointMax][4][4];
};

struct SObjPsConstBuf
{
	SObjCommonPsConstBuf CommonParam;
};

struct SObjFastPsConstBuf
{
	SObjCommonPsConstBuf CommonParam;
	float Eye[4];
	float Dir[4];
	float Half[4];
};

struct SObjOutlineVsConstBuf
{
	SObjCommonVsConstBuf CommonParam;
	float OutlineParam[4];
	float Joint[JointMax][4][4];
};

struct SObjOutlinePsConstBuf
{
	float OutlineColor[4];
};

struct SObjShadowVsConstBuf
{
	float World[4][4];
	float ProjView[4][4];
	float Joint[JointMax][4][4];
};

struct SParticleTexSet
{
	ID3D10Texture2D* TexParam;
	ID3D10ShaderResourceView* ViewParam;
	ID3D10RenderTargetView* RenderTargetViewParam;
};

struct SParticlePsConstBuf
{
	float Color1[4];
	float Color2[4];
};

struct SParticleUpdatingPsConstBuf
{
	float AccelAndFriction[4];
	float SizeAccelAndRotAccel[4];
};

struct SParticle
{
	SClass Class;
	S64 Lifespan;
	SParticlePsConstBuf PsConstBuf;
	SParticleUpdatingPsConstBuf UpdatingPsConstBuf;
	S64 ParticlePtr;
	SParticleTexSet* TexSet;
	ID3D10Texture2D* TexTmp;
	Bool Draw1To2;
};

struct SShadow
{
	SClass Class;
	ID3D10Texture2D* DepthTex;
	ID3D10DepthStencilView* DepthView;
	ID3D10ShaderResourceView* DepthResView;
	int DepthWidth;
	int DepthHeight;
	float(*ShadowProjView)[4];
};

enum EShaderBuf
{
	ShaderBuf_TriVs,
	ShaderBuf_TriPs,
	ShaderBuf_RectVs,
	ShaderBuf_CircleVs,
	ShaderBuf_CirclePs,
	ShaderBuf_CircleLinePs,
	ShaderBuf_TexVs,
	ShaderBuf_TexRotVs,
	ShaderBuf_TexPs,
	ShaderBuf_FontPs,
	ShaderBuf_ObjVs,
	ShaderBuf_ObjSmVs,
	ShaderBuf_ObjJointVs,
	ShaderBuf_ObjJointSmVs,
	ShaderBuf_ObjPs,
	ShaderBuf_ObjSmPs,
	ShaderBuf_ObjToonPs,
	ShaderBuf_ObjToonSmPs,
	ShaderBuf_ObjFastVs,
	ShaderBuf_ObjFastSmVs,
	ShaderBuf_ObjFastJointVs,
	ShaderBuf_ObjFastJointSmVs,
	ShaderBuf_ObjFastPs,
	ShaderBuf_ObjFastSmPs,
	ShaderBuf_ObjToonFastPs,
	ShaderBuf_ObjToonFastSmPs,
	ShaderBuf_ObjFlatVs,
	ShaderBuf_ObjFlatJointVs,
	ShaderBuf_ObjFlatFastVs,
	ShaderBuf_ObjFlatFastJointVs,
	ShaderBuf_ObjFlatPs,
	ShaderBuf_ObjOutlineVs,
	ShaderBuf_ObjOutlineJointVs,
	ShaderBuf_ObjOutlinePs,
	ShaderBuf_ObjShadowVs,
	ShaderBuf_ObjShadowJointVs,
	ShaderBuf_FilterVs,
	ShaderBuf_Filter0Ps,
	ShaderBuf_Filter1Ps,
	ShaderBuf_Particle2dVs,
	ShaderBuf_Particle2dPs,
	ShaderBuf_ParticleUpdatingVs,
	ShaderBuf_ParticleUpdatingPs,
	ShaderBuf_PolyVs,
	ShaderBuf_PolyPs,

	ShaderBuf_Num,
};

enum EVertexBuf
{
	VertexBuf_TriVertex,
	VertexBuf_RectVertex,
	VertexBuf_LineVertex,
	VertexBuf_RectLineVertex,
	VertexBuf_CircleVertex,
	VertexBuf_FilterVertex,
	VertexBuf_ParticleVertex,
	VertexBuf_ParticleUpdatingVertex,
	VertexBuf_PolyVertex,

	VertexBuf_Num,
};

static const FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

static const D3D10_VIEWPORT ParticleViewport = { 0, 0, static_cast<UINT>(ParticleNum), 1, 0.0f, 1.0f };

const U8* GetTriVsBin(size_t* size);
const U8* GetTriPsBin(size_t* size);
const U8* GetFontPsBin(size_t* size);
const U8* GetRectVsBin(size_t* size);
const U8* GetCircleVsBin(size_t* size);
const U8* GetCirclePsBin(size_t* size);
const U8* GetCircleLinePsBin(size_t* size);
const U8* GetTexVsBin(size_t* size);
const U8* GetTexRotVsBin(size_t* size);
const U8* GetTexPsBin(size_t* size);
const U8* GetObjVsBin(size_t* size);
const U8* GetObjSmVsBin(size_t* size);
const U8* GetObjJointVsBin(size_t* size);
const U8* GetObjJointSmVsBin(size_t* size);
const U8* GetObjPsBin(size_t* size);
const U8* GetObjSmPsBin(size_t* size);
const U8* GetObjToonPsBin(size_t* size);
const U8* GetObjToonSmPsBin(size_t* size);
const U8* GetObjFastVsBin(size_t* size);
const U8* GetObjFastSmVsBin(size_t* size);
const U8* GetObjFastJointVsBin(size_t* size);
const U8* GetObjFastJointSmVsBin(size_t* size);
const U8* GetObjFastPsBin(size_t* size);
const U8* GetObjFastSmPsBin(size_t* size);
const U8* GetObjToonFastPsBin(size_t* size);
const U8* GetObjToonFastSmPsBin(size_t* size);
const U8* GetObjFlatVsBin(size_t* size);
const U8* GetObjFlatJointVsBin(size_t* size);
const U8* GetObjFlatFastVsBin(size_t* size);
const U8* GetObjFlatFastJointVsBin(size_t* size);
const U8* GetObjFlatPsBin(size_t* size);
const U8* GetObjOutlineVsBin(size_t* size);
const U8* GetObjOutlineJointVsBin(size_t* size);
const U8* GetObjOutlinePsBin(size_t* size);
const U8* GetObjShadowVsBin(size_t* size);
const U8* GetObjShadowJointVsBin(size_t* size);
const U8* GetFilterVsBin(size_t* size);
const U8* GetFilterNonePsBin(size_t* size);
const U8* GetFilterMonotonePsBin(size_t* size);
const U8* GetToonRampPngBin(size_t* size);
const U8* GetParticle2dVsBin(size_t* size);
const U8* GetParticle2dPsBin(size_t* size);
const U8* GetParticleUpdatingVsBin(size_t* size);
const U8* GetParticleUpdatingPsBin(size_t* size);
const U8* GetPolyVsBin(size_t* size);
const U8* GetPolyPsBin(size_t* size);
const U8* GetBoxKnobjBin(size_t* size);
const U8* GetSphereKnobjBin(size_t* size);
const U8* GetPlaneKnobjBin(size_t* size);

static void* (*Callback2d)(int kind, void* arg1, void* arg2) = NULL;
static S64 Cnt;
static U32 PrevTime;
static ID3D10Device1* Device = NULL;
static ID3D10RasterizerState* RasterizerState = NULL;
static ID3D10RasterizerState* RasterizerStateInverted = NULL;
static ID3D10RasterizerState* RasterizerStateNone = NULL;
static ID3D10DepthStencilState* DepthState[DepthNum] = { NULL };
static ID3D10BlendState* BlendState[BlendNum] = { NULL };
static ID3D10SamplerState* Sampler[SamplerNum] = { NULL };
static SWndBuf* CurWndBuf = NULL;
static void* ShaderBufs[ShaderBuf_Num] = { NULL };
static void* VertexBufs[VertexBuf_Num] = { NULL };
static double ViewMat[4][4];
static double ProjMat[4][4];
static SObjVsConstBuf ObjVsConstBuf;
static SObjPsConstBuf ObjPsConstBuf;
static int CurZBuf = -1;
static int CurBlend = -1;
static int CurSampler = -1;
ID3D10Texture2D* TexToonRamp;
ID3D10ShaderResourceView* ViewToonRamp;
ID3D10Texture2D* TexEven[TexEvenNum];
ID3D10ShaderResourceView* ViewEven[TexEvenNum];
static int FilterIdx = 0;
static float FilterParam[4][4];

static Bool MakeTexWithImg(ID3D10Texture2D** tex, ID3D10ShaderResourceView** view, ID3D10RenderTargetView** render_target_view, int width, int height, const void* img, size_t pitch, DXGI_FORMAT fmt, D3D10_USAGE usage, UINT cpu_access_flag, Bool render_target);
static void UpdateParticles(SParticle* particle);
static void WriteBack();
static double CalcFontLineWidth(SFont* font, const Char* text);
static double CalcFontLineHeight(SFont* font, const Char* text);
static SClass* MakeObjImpl(SClass* me_, size_t size, const void* binary);
static void WriteFastPsConstBuf(SObjFastPsConstBuf* ps_const_buf);
static int SearchFromCache(SFont* me, int cell_num_width, int cell_num, Char c);
static void ObjDrawImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
static void ObjDrawToonImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
static void ObjDrawFlatImpl(SClass* me_, S64 element, double frame, SClass* diffuse);

EXPORT_CPP void _set2dCallback(void*(*callback)(int, void*, void*))
{
	Callback2d = callback;
}

EXPORT_CPP void _render(S64 fps)
{
	WriteBack();

	// Draw with a filter.
	{
		int old_z_buf = CurZBuf;
		int old_blend = CurBlend;
		int old_sampler = CurSampler;

		D3D10_VIEWPORT viewport =
		{
			0,
			0,
			static_cast<UINT>(CurWndBuf->TexWidth),
			static_cast<UINT>(CurWndBuf->TexHeight),
			0.0f,
			1.0f,
		};
		Device->RSSetViewports(1, &viewport);

		_depth(False, False);
		_blend(0);
		_sampler(0);

		Device->OMSetRenderTargets(1, &CurWndBuf->RenderTargetView, NULL);
		{
			Draw::ConstBuf(ShaderBufs[ShaderBuf_FilterVs], NULL);
			Device->GSSetShader(NULL);
			Draw::ConstBuf(ShaderBufs[ShaderBuf_Filter0Ps + FilterIdx], FilterIdx == 0 ? NULL : FilterParam);
			Draw::VertexBuf(VertexBufs[VertexBuf_FilterVertex]);
			Device->PSSetShaderResources(0, 1, &CurWndBuf->TmpShaderResView);
		}
		Device->DrawIndexed(6, 0, 0);

		_depth((old_z_buf & 2) != 0, (old_z_buf & 1) != 0);
		_blend(old_blend);
		_sampler(old_sampler);

		Draw::ResetViewport();
	}

	CurWndBuf->SwapChain->Present(fps == 0 ? 0 : 1, 0);
	Device->OMSetRenderTargets(1, &CurWndBuf->TmpRenderTargetView, CurWndBuf->DepthView);
	if (CurWndBuf->AutoClear)
		Draw::Clear();
	Device->RSSetState(RasterizerState);

	if (fps == 0)
		return;
	Cnt++;
	U32 now = static_cast<U32>(timeGetTime());
	U32 diff = now - PrevTime;
	int next_wait = 0;
	int sleep_time = 0;
	switch (fps)
	{
		case 30:
			sleep_time = (Cnt % 3 == 0 ? 34 : 33);
			break;
		case 60:
			sleep_time = (Cnt % 3 == 0 ? 16 : 17);
			break;
		default:
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
			return;
	}
	sleep_time -= static_cast<int>(diff);
	if (sleep_time > 2)
	{
		U32 to_time = now + static_cast<U32>(sleep_time);
		Sleep(static_cast<DWORD>(sleep_time - 1));
		while (static_cast<U32>(timeGetTime()) < to_time)
		{
			// Do nothing.
		}
	}
	else
	{
		Sleep(1);
		next_wait = sleep_time;
		if (next_wait <= -100)
			next_wait = 0;
	}
	PrevTime = static_cast<U32>(static_cast<int>(timeGetTime()) - next_wait);
}

EXPORT_CPP S64 _cnt()
{
	return Cnt;
}

EXPORT_CPP S64 _screenWidth()
{
	return CurWndBuf->TexWidth;
}

EXPORT_CPP S64 _screenHeight()
{
	return CurWndBuf->TexHeight;
}

EXPORT_CPP void _depth(Bool test, Bool write)
{
	int kind = (static_cast<int>(test) << 1) | static_cast<int>(write);
	if (CurZBuf == kind)
		return;
	Device->OMSetDepthStencilState(DepthState[kind], 0);
	CurZBuf = kind;
}

EXPORT_CPP void _blend(S64 kind)
{
	THROWDBG(kind < 0 || BlendNum <= kind, EXCPT_DBG_ARG_OUT_DOMAIN);
	int kind2 = static_cast<int>(kind);
	if (CurBlend == kind2)
		return;
	Device->OMSetBlendState(BlendState[kind2], BlendFactor, 0xffffffff);
	CurBlend = kind2;
}

EXPORT_CPP void _sampler(S64 kind)
{
	THROWDBG(kind < 0 || SamplerNum <= kind, EXCPT_DBG_ARG_OUT_DOMAIN);
	int kind2 = static_cast<int>(kind);
	if (CurSampler == kind2)
		return;
	Device->PSSetSamplers(0, 1, &Sampler[kind2]);
	CurSampler = kind2;
}

EXPORT_CPP void _clearColor(S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
	CurWndBuf->ClearColor[0] = static_cast<FLOAT>(r);
	CurWndBuf->ClearColor[1] = static_cast<FLOAT>(g);
	CurWndBuf->ClearColor[2] = static_cast<FLOAT>(b);
}

EXPORT_CPP void _autoClear(Bool enabled)
{
	CurWndBuf->AutoClear = enabled;
}

EXPORT_CPP void _clear()
{
	WriteBack();
	Draw::Clear();
}

EXPORT_CPP void _editPixels(const void* callback)
{
	THROWDBG(callback == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(!CurWndBuf->Editable, EXCPT_DBG_INOPERABLE_STATE);
	WriteBack();
	const size_t buf_size = static_cast<size_t>(CurWndBuf->TexWidth * CurWndBuf->TexHeight);
	U8* buf = static_cast<U8*>(AllocMem(0x10 + sizeof(U32) * buf_size));
	((S64*)buf)[0] = 2;
	((S64*)buf)[1] = buf_size;
	Device->CopyResource(CurWndBuf->EditableTex, CurWndBuf->TmpTex);
	{
		D3D10_MAPPED_TEXTURE2D map;
		CurWndBuf->EditableTex->Map(D3D10CalcSubresource(0, 0, 1), D3D10_MAP_READ_WRITE, 0, &map);
		U8* dst = static_cast<U8*>(map.pData);
		memcpy(buf + 0x10, dst, sizeof(U32) * buf_size);
		Call3Asm(reinterpret_cast<void*>(static_cast<U64>(CurWndBuf->TexWidth)), reinterpret_cast<void*>(static_cast<U64>(CurWndBuf->TexHeight)), buf, (void*)callback);
		memcpy(dst, buf + 0x10, sizeof(U32) * buf_size);
		CurWndBuf->EditableTex->Unmap(D3D10CalcSubresource(0, 0, 1));
	}
	Device->CopyResource(CurWndBuf->TmpTex, CurWndBuf->EditableTex);
	THROWDBG(((S64*)buf)[0] != 1, EXCPT_ACCESS_VIOLATION);
	FreeMem(buf);
}

EXPORT_CPP Bool _capture(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	ID3D10Texture2D* tex;
	WriteBack();
	if (CurWndBuf->Editable)
		tex = CurWndBuf->EditableTex;
	else
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width = static_cast<UINT>(CurWndBuf->TexWidth);
		desc.Height = static_cast<UINT>(CurWndBuf->TexHeight);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateTexture2D(&desc, NULL, &tex)))
			return False;
	}
	Bool result = False;
	Device->CopyResource(tex, CurWndBuf->TmpTex);
	{
		FILE* file_ptr = _wfopen(reinterpret_cast<const Char*>(path + 0x10), L"wb");
		if (file_ptr != NULL)
		{
			D3D10_MAPPED_TEXTURE2D map;
			tex->Map(D3D10CalcSubresource(0, 0, 1), D3D10_MAP_READ, 0, &map);
			const U8* dst = static_cast<U8*>(map.pData);
			{
				BITMAPFILEHEADER header;
				BITMAPINFOHEADER info;
				U8 buf[3];
				header.bfType = 0x4d42;
				header.bfSize = static_cast<DWORD>(sizeof(header) + sizeof(info) + CurWndBuf->TexWidth * CurWndBuf->TexHeight * 3);
				header.bfReserved1 = 0;
				header.bfReserved2 = 0;
				header.bfOffBits = sizeof(header) + sizeof(info);
				info.biSize = sizeof(info);
				info.biWidth = static_cast<LONG>(CurWndBuf->TexWidth);
				info.biHeight = static_cast<LONG>(CurWndBuf->TexHeight);
				info.biPlanes = 1;
				info.biBitCount = 24;
				info.biCompression = BI_RGB;
				info.biSizeImage = static_cast<DWORD>(CurWndBuf->TexWidth * CurWndBuf->TexHeight * 3);
				info.biXPelsPerMeter = 0;
				info.biYPelsPerMeter = 0;
				info.biClrUsed = 0;
				info.biClrImportant = 0;
				fwrite(&header, sizeof(header), 1, file_ptr);
				fwrite(&info, sizeof(info), 1, file_ptr);
				for (S64 j = CurWndBuf->TexHeight - 1; j >= 0; j--)
				{
					for (S64 i = 0; i < CurWndBuf->TexWidth; i++)
					{
						buf[0] = static_cast<U8>(max(min(static_cast<int>(Draw::Degamma(static_cast<double>(dst[(j * CurWndBuf->TexWidth + i) * 4 + 0]) / 255.0) * 255.0), 255), 0));
						buf[1] = static_cast<U8>(max(min(static_cast<int>(Draw::Degamma(static_cast<double>(dst[(j * CurWndBuf->TexWidth + i) * 4 + 1]) / 255.0) * 255.0), 255), 0));
						buf[2] = static_cast<U8>(max(min(static_cast<int>(Draw::Degamma(static_cast<double>(dst[(j * CurWndBuf->TexWidth + i) * 4 + 2]) / 255.0) * 255.0), 255), 0));
						fwrite(buf, sizeof(buf), 1, file_ptr);
					}
				}
			}
			tex->Unmap(D3D10CalcSubresource(0, 0, 1));
			fclose(file_ptr);
			result = True;
		}
	}
	if (!CurWndBuf->Editable)
		tex->Release();
	return result;
}

EXPORT_CPP void _line(double x1, double y1, double x2, double y2, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_RectVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_LineVertex]);
	}
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	Device->DrawIndexed(2, 0, 0);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

EXPORT_CPP void _tri(double x1, double y1, double x2, double y2, double x3, double y3, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TriVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_TriVertex]);
	}
	Device->DrawIndexed(3, 0, 0);
}

EXPORT_CPP void _rect(double x, double y, double w, double h, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_RectVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_RectVertex]);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _rectLine(double x, double y, double w, double h, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_RectVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TriPs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_RectLineVertex]);
	}
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Device->DrawIndexed(5, 0, 0);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

EXPORT_CPP void _circle(double x, double y, double radiusX, double radiusY, S64 color)
{
	if (radiusX == 0.0 || radiusY == 0.0)
		return;
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_CircleVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_CirclePs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_CircleVertex]);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _circleLine(double x, double y, double radiusX, double radiusY, S64 color)
{
	if (radiusX == 0.0 || radiusY == 0.0)
		return;
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_CircleVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_CircleLinePs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_CircleVertex]);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _poly(const void* x, const void* y, const void* color)
{
	S64 len_x = static_cast<const S64*>(x)[1];
#if defined(DBG)
	S64 len_y = static_cast<const S64*>(y)[1];
	S64 len_color = static_cast<const S64*>(color)[1];
	THROWDBG(len_x != len_y || len_y != len_color || len_x > PolyVerticesNum, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	const double* x2 = reinterpret_cast<const double*>(static_cast<const U8*>(x) + 0x10);
	const double* y2 = reinterpret_cast<const double*>(static_cast<const U8*>(y) + 0x10);
	const S64* color2 = reinterpret_cast<const S64*>(static_cast<const U8*>(color) + 0x10);
	if (len_x <= 2)
		return;
	float const_buf_vs[4 * PolyVerticesNum * 2];
	for (S64 i = 0; i < len_x; i++)
	{
		const_buf_vs[4 * i + 0] = static_cast<float>(x2[i]) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f;
		const_buf_vs[4 * i + 1] = -(static_cast<float>(y2[i]) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f);
		const_buf_vs[4 * i + 2] = 0.0f;
		const_buf_vs[4 * i + 3] = 0.0f;

		double r, g, b, a;
		Draw::ColorToArgb(&a, &r, &g, &b, color2[i]);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 0] = static_cast<float>(r);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 1] = static_cast<float>(g);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 2] = static_cast<float>(b);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 3] = static_cast<float>(a);
	}
	Draw::ConstBuf(ShaderBufs[ShaderBuf_PolyVs], const_buf_vs);
	Device->GSSetShader(NULL);
	Draw::ConstBuf(ShaderBufs[ShaderBuf_PolyPs], NULL);
	Draw::VertexBuf(VertexBufs[VertexBuf_PolyVertex]);
	Device->RSSetState(RasterizerStateNone);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Device->DrawIndexed(static_cast<UINT>(len_x), 0, 0);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Device->RSSetState(RasterizerState);
}

EXPORT_CPP void _polyLine(const void* x, const void* y, const void* color)
{
	S64 len_x = static_cast<const S64*>(x)[1];
#if defined(DBG)
	S64 len_y = static_cast<const S64*>(y)[1];
	S64 len_color = static_cast<const S64*>(color)[1];
	THROWDBG(len_x != len_y || len_y != len_color || len_x > PolyVerticesNum, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	const double* x2 = reinterpret_cast<const double*>(static_cast<const U8*>(x) + 0x10);
	const double* y2 = reinterpret_cast<const double*>(static_cast<const U8*>(y) + 0x10);
	const S64* color2 = reinterpret_cast<const S64*>(static_cast<const U8*>(color) + 0x10);
	if (len_x <= 1)
		return;
	float const_buf_vs[4 * PolyVerticesNum * 2];
	for (S64 i = 0; i < len_x; i++)
	{
		const_buf_vs[4 * i + 0] = static_cast<float>(x2[i]) / static_cast<float>(CurWndBuf->TexWidth) * 2.0f - 1.0f;
		const_buf_vs[4 * i + 1] = -(static_cast<float>(y2[i]) / static_cast<float>(CurWndBuf->TexHeight) * 2.0f - 1.0f);
		const_buf_vs[4 * i + 2] = 0.0f;
		const_buf_vs[4 * i + 3] = 0.0f;

		double r, g, b, a;
		Draw::ColorToArgb(&a, &r, &g, &b, color2[i]);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 0] = static_cast<float>(r);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 1] = static_cast<float>(g);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 2] = static_cast<float>(b);
		const_buf_vs[4 * PolyVerticesNum + 4 * i + 3] = static_cast<float>(a);
	}
	Draw::ConstBuf(ShaderBufs[ShaderBuf_PolyVs], const_buf_vs);
	Device->GSSetShader(NULL);
	Draw::ConstBuf(ShaderBufs[ShaderBuf_PolyPs], NULL);
	Draw::VertexBuf(VertexBufs[VertexBuf_PolyVertex]);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Device->DrawIndexed(static_cast<UINT>(len_x), 0, 0);
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

EXPORT_CPP void _filterNone()
{
	FilterIdx = 0;
}

EXPORT_CPP void _filterMonotone(S64 color, double rate)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
	if (rate < 0.0)
		rate = 0.0;
	else if (rate > 1.0)
		rate = 1.0;

	FilterIdx = 1;
	FilterParam[0][0] = static_cast<float>(r);
	FilterParam[0][1] = static_cast<float>(g);
	FilterParam[0][2] = static_cast<float>(b);
	FilterParam[0][3] = static_cast<float>(rate);
}

EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* path)
{
	return Draw::MakeTexImpl(me_, path, False);
}

EXPORT_CPP SClass* _makeTexArgb(SClass* me_, const U8* path)
{
	return Draw::MakeTexImpl(me_, path, True);
}

EXPORT_CPP SClass* _makeTexEvenArgb(SClass* me_, double a, double r, double g, double b)
{
	STex* me2 = reinterpret_cast<STex*>(me_);
	float img[4] = { static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a) };
	me2->Width = 1;
	me2->Height = 1;
	me2->ImgWidth = 1;
	me2->ImgHeight = 1;
	if (!MakeTexWithImg(&me2->Tex, &me2->View, NULL, 1, 1, img, sizeof(img), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D10_USAGE_IMMUTABLE, 0, False))
		THROW(EXCPT_DEVICE_INIT_FAILED);
	return me_;
}

EXPORT_CPP SClass* _makeTexEvenColor(SClass* me_, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
	return _makeTexEvenArgb(me_, a, r, g, b);
}

EXPORT_CPP void _texDtor(SClass* me_)
{
	STex* me2 = reinterpret_cast<STex*>(me_);
	if (me2->View != NULL)
		me2->View->Release();
	if (me2->Tex != NULL)
		me2->Tex->Release();
}

EXPORT_CPP S64 _texWidth(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->Width);
}

EXPORT_CPP S64 _texHeight(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->Height);
}

EXPORT_CPP S64 _texImgWidth(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->ImgWidth);
}

EXPORT_CPP S64 _texImgHeight(SClass* me_)
{
	return static_cast<S64>(reinterpret_cast<STex*>(me_)->ImgHeight);
}

EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double srcX, double srcY, double srcW, double srcH, S64 color)
{
	_texDrawScale(me_, dstX, dstY, srcW, srcH, srcX, srcY, srcW, srcH, color);
}

EXPORT_CPP void _texDrawScale(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TexVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TexPs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_RectVertex]);
		Device->PSSetShaderResources(0, 1, &me2->View);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP void _texDrawRot(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, double centerX, double centerY, double angle, S64 color)
{
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TexRotVs], const_buf_vs);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_TexPs], const_buf_ps);
		Draw::VertexBuf(VertexBufs[VertexBuf_RectVertex]);
		Device->PSSetShaderResources(0, 1, &me2->View);
	}
	Device->DrawIndexed(6, 0, 0);
}

EXPORT_CPP SClass* _makeFont(SClass* me_, const U8* fontName, S64 size, bool bold, bool italic, bool proportional, double advance)
{
	THROWDBG(size < 1, EXCPT_DBG_ARG_OUT_DOMAIN);
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	int char_height;
	{
		HDC dc = GetDC(NULL);
		char_height = MulDiv(static_cast<int>(size), GetDeviceCaps(dc, LOGPIXELSY), 72);
		ReleaseDC(NULL, dc);
	}
	me2->Font = CreateFont(-char_height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic ? TRUE : FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, fontName == NULL ? L"Meiryo UI" : reinterpret_cast<const Char*>(fontName + 0x10));
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
		HDC dc = GetDC(NULL);
		me2->Bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&me2->Pixel), NULL, 0);
		me2->Dc = CreateCompatibleDC(dc);
		ReleaseDC(NULL, dc);
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
		if (FAILED(Device->CreateTexture2D(&desc, NULL, &me2->Tex)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}
	{
		D3D10_SHADER_RESOURCE_VIEW_DESC desc;
		memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;
		if (FAILED(Device->CreateShaderResourceView(me2->Tex, &desc, &me2->View)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
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

EXPORT_CPP void _fontDtor(SClass* me_)
{
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	DeleteDC(me2->Dc);
	DeleteObject(static_cast<HGDIOBJ>(me2->Bitmap));
	if (me2->GlyphWidth != NULL)
		FreeMem(me2->GlyphWidth);
	FreeMem(me2->CntMap);
	FreeMem(me2->CharMap);
	if (me2->View != NULL)
		me2->View->Release();
	if (me2->Tex != NULL)
		me2->Tex->Release();
}

EXPORT_CPP void _fontDraw(SClass* me_, double dstX, double dstY, const U8* text, S64 color)
{
	_fontDrawScale(me_, dstX, dstY, 1.0, 1.0, text, color);
}

EXPORT_CPP void _fontDrawScale(SClass* me_, double dstX, double dstY, double dstScaleX, double dstScaleY, const U8* text, S64 color)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	double r, g, b, a;
	Draw::ColorToArgb(&a, &r, &g, &b, color);
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
			Draw::ConstBuf(ShaderBufs[ShaderBuf_TexVs], const_buf_vs);
			Device->GSSetShader(NULL);
			Draw::ConstBuf(ShaderBufs[ShaderBuf_FontPs], const_buf_ps);
			Draw::VertexBuf(VertexBufs[VertexBuf_RectVertex]);
			Device->PSSetShaderResources(0, 1, &me2->View);
			Device->DrawIndexed(6, 0, 0);
		}
		x += me2->Advance * dstScaleX;
		if (me2->Proportional)
			x += static_cast<double>(me2->GlyphWidth[pos]) * dstScaleX;
		ptr++;
	}
}

EXPORT_CPP double _fontMaxWidth(SClass* me_)
{
	return reinterpret_cast<SFont*>(me_)->CellWidth;
}

EXPORT_CPP double _fontMaxHeight(SClass* me_)
{
	return reinterpret_cast<SFont*>(me_)->CellHeight;
}

EXPORT_CPP double _fontCalcWidth(SClass* me_, const U8* text)
{
	double width;
	double height;
	_fontCalcSize(me_, &width, &height, text);
	return width;
}

EXPORT_CPP void _fontCalcSize(SClass* me_, double* width, double* height, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
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

EXPORT_CPP void _fontSetHeight(SClass* me_, double height)
{
	reinterpret_cast<SFont*>(me_)->Height = height;
}

EXPORT_CPP double _fontGetHeight(SClass* me_)
{
	return reinterpret_cast<SFont*>(me_)->Height;
}

EXPORT_CPP void _fontAlign(SClass* me_, S64 horizontal, S64 vertical)
{
	SFont* me2 = reinterpret_cast<SFont*>(me_);
	me2->AlignHorizontal = static_cast<U8>(horizontal);
	me2->AlignVertical = static_cast<U8>(vertical);
}

EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ)
{
	double look[3], up[3], right[3], eye[3], pxyz[3], eye_len;

	look[0] = atX - eyeX;
	look[1] = atY - eyeY;
	look[2] = atZ - eyeZ;
	eye_len = Draw::Normalize(look);
	if (eye_len == 0.0)
		return;

	up[0] = upX;
	up[1] = upY;
	up[2] = upZ;
	Draw::Cross(right, up, look);
	if (Draw::Normalize(right) == 0.0)
		return;

	Draw::Cross(up, look, right);

	eye[0] = eyeX;
	eye[1] = eyeY;
	eye[2] = eyeZ;
	pxyz[0] = Draw::Dot(eye, right);
	pxyz[1] = Draw::Dot(eye, up);
	pxyz[2] = Draw::Dot(eye, look);

	ViewMat[0][0] = right[0];
	ViewMat[0][1] = up[0];
	ViewMat[0][2] = look[0];
	ViewMat[0][3] = 0.0;
	ViewMat[1][0] = right[1];
	ViewMat[1][1] = up[1];
	ViewMat[1][2] = look[1];
	ViewMat[1][3] = 0.0;
	ViewMat[2][0] = right[2];
	ViewMat[2][1] = up[2];
	ViewMat[2][2] = look[2];
	ViewMat[2][3] = 0.0;
	ViewMat[3][0] = -pxyz[0];
	ViewMat[3][1] = -pxyz[1];
	ViewMat[3][2] = -pxyz[2];
	ViewMat[3][3] = 1.0;

	ObjVsConstBuf.CommonParam.Eye[0] = static_cast<float>(-look[0]);
	ObjVsConstBuf.CommonParam.Eye[1] = static_cast<float>(-look[1]);
	ObjVsConstBuf.CommonParam.Eye[2] = static_cast<float>(-look[2]);
	ObjVsConstBuf.CommonParam.Eye[3] = static_cast<float>(eye_len);

	Draw::SetProjViewMat(ObjVsConstBuf.CommonParam.ProjView, ProjMat, ViewMat);
}

EXPORT_CPP void _proj(double fovy, double aspectX, double aspectY, double nearZ, double farZ)
{
	THROWDBG(fovy <= 0.0 || M_PI / 2.0 <= fovy || aspectX <= 0.0 || aspectY <= 0.0 || nearZ <= 0.0 || farZ <= nearZ, EXCPT_DBG_ARG_OUT_DOMAIN);
	double tan_theta = tan(fovy / 2.0);
	ProjMat[0][0] = -1.0 / ((aspectX / aspectY) * tan_theta);
	ProjMat[0][1] = 0.0;
	ProjMat[0][2] = 0.0;
	ProjMat[0][3] = 0.0;
	ProjMat[1][0] = 0.0;
	ProjMat[1][1] = 1.0 / tan_theta;
	ProjMat[1][2] = 0.0;
	ProjMat[1][3] = 0.0;
	ProjMat[2][0] = 0.0;
	ProjMat[2][1] = 0.0;
	ProjMat[2][2] = farZ / (farZ - nearZ);
	ProjMat[2][3] = 1.0;
	ProjMat[3][0] = 0.0;
	ProjMat[3][1] = 0.0;
	ProjMat[3][2] = -farZ * nearZ / (farZ - nearZ);
	ProjMat[3][3] = 0.0;

	Draw::SetProjViewMat(ObjVsConstBuf.CommonParam.ProjView, ProjMat, ViewMat);
}

EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* path)
{
	U8* buf = NULL;
	size_t size;
	buf = static_cast<U8*>(LoadFileAll(reinterpret_cast<const Char*>(path + 0x10), &size));
	if (buf == NULL)
	{
		THROW(EXCPT_FILE_READ_FAILED);
		return NULL;
	}
	SClass* obj = MakeObjImpl(me_, size, buf);
	FreeMem(buf);
	return obj;
}

EXPORT_CPP void _objDtor(SClass* me_)
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

EXPORT_CPP SClass* _makeBox(SClass* me_)
{
	size_t size;
	const U8* binary = GetBoxKnobjBin(&size);
	return MakeObjImpl(me_, size, binary);
}

EXPORT_CPP SClass* _makeSphere(SClass* me_)
{
	size_t size;
	const U8* binary = GetSphereKnobjBin(&size);
	return MakeObjImpl(me_, size, binary);
}

EXPORT_CPP SClass* _makePlane(SClass* me_)
{
	size_t size;
	const U8* binary = GetPlaneKnobjBin(&size);
	return MakeObjImpl(me_, size, binary);
}

EXPORT_CPP void _objDraw(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal)
{
	ObjDrawImpl(me_, element, frame, diffuse, specular, normal, NULL);
}

EXPORT_CPP void _objDrawToon(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal)
{
	ObjDrawToonImpl(me_, element, frame, diffuse, specular, normal, NULL);
}

EXPORT_CPP void _objDrawFlat(SClass* me_, S64 element, double frame, SClass* diffuse)
{
	ObjDrawFlatImpl(me_, element, frame, diffuse);
}

EXPORT_CPP void _objDrawOutline(SClass* me_, S64 element, double frame, double width, S64 color)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, EXCPT_DBG_ARG_OUT_DOMAIN);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), EXCPT_DBG_ARG_OUT_DOMAIN);
				THROWDBG(element2->JointNum < 0 || JointMax < element2->JointNum, EXCPT_DBG_ARG_OUT_DOMAIN);
				Bool joint = element2->JointNum != 0;

				SObjOutlineVsConstBuf vs_const_buf;
				SObjOutlinePsConstBuf ps_const_buf;
				vs_const_buf.CommonParam = ObjVsConstBuf.CommonParam;
				vs_const_buf.OutlineParam[0] = static_cast<float>(width);
				{
					double r, g, b, a;
					Draw::ColorToArgb(&a, &r, &g, &b, color);
					ps_const_buf.OutlineColor[0] = static_cast<float>(r);
					ps_const_buf.OutlineColor[1] = static_cast<float>(g);
					ps_const_buf.OutlineColor[2] = static_cast<float>(b);
					ps_const_buf.OutlineColor[3] = static_cast<float>(a);
				}

				if (joint && frame >= 0.0f)
				{
					Draw::SetJointMat(element2, frame, vs_const_buf.Joint);
					Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjOutlineJointVs], &vs_const_buf);
				}
				else
					Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjOutlineVs], &vs_const_buf);
				Device->GSSetShader(NULL);
				Device->RSSetState(RasterizerStateInverted);
				Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjOutlinePs], &ps_const_buf);

				Draw::VertexBuf(element2->VertexBuf);
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
				Device->RSSetState(RasterizerState);
		}
			break;
	}
}

EXPORT_CPP void _objDrawWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	ObjDrawImpl(me_, element, frame, diffuse, specular, normal, shadow);
}

EXPORT_CPP void _objDrawToonWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	ObjDrawToonImpl(me_, element, frame, diffuse, specular, normal, shadow);
}

EXPORT_CPP void _objMat(SClass* me_, const U8* mat, const U8* normMat)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(*(S64*)(mat + 0x08) != 16 || *(S64*)(normMat + 0x08) != 16, EXCPT_DBG_ARG_OUT_DOMAIN);
	{
		const double* ptr = reinterpret_cast<const double*>(mat + 0x10);
		me2->Mat[0][0] = static_cast<float>(ptr[0]);
		me2->Mat[0][1] = static_cast<float>(ptr[1]);
		me2->Mat[0][2] = static_cast<float>(ptr[2]);
		me2->Mat[0][3] = static_cast<float>(ptr[3]);
		me2->Mat[1][0] = static_cast<float>(ptr[4]);
		me2->Mat[1][1] = static_cast<float>(ptr[5]);
		me2->Mat[1][2] = static_cast<float>(ptr[6]);
		me2->Mat[1][3] = static_cast<float>(ptr[7]);
		me2->Mat[2][0] = static_cast<float>(ptr[8]);
		me2->Mat[2][1] = static_cast<float>(ptr[9]);
		me2->Mat[2][2] = static_cast<float>(ptr[10]);
		me2->Mat[2][3] = static_cast<float>(ptr[11]);
		me2->Mat[3][0] = static_cast<float>(ptr[12]);
		me2->Mat[3][1] = static_cast<float>(ptr[13]);
		me2->Mat[3][2] = static_cast<float>(ptr[14]);
		me2->Mat[3][3] = static_cast<float>(ptr[15]);
	}
	{
		const double* ptr = reinterpret_cast<const double*>(normMat + 0x10);
		me2->NormMat[0][0] = static_cast<float>(ptr[0]);
		me2->NormMat[0][1] = static_cast<float>(ptr[1]);
		me2->NormMat[0][2] = static_cast<float>(ptr[2]);
		me2->NormMat[0][3] = static_cast<float>(ptr[3]);
		me2->NormMat[1][0] = static_cast<float>(ptr[4]);
		me2->NormMat[1][1] = static_cast<float>(ptr[5]);
		me2->NormMat[1][2] = static_cast<float>(ptr[6]);
		me2->NormMat[1][3] = static_cast<float>(ptr[7]);
		me2->NormMat[2][0] = static_cast<float>(ptr[8]);
		me2->NormMat[2][1] = static_cast<float>(ptr[9]);
		me2->NormMat[2][2] = static_cast<float>(ptr[10]);
		me2->NormMat[2][3] = static_cast<float>(ptr[11]);
		me2->NormMat[3][0] = static_cast<float>(ptr[12]);
		me2->NormMat[3][1] = static_cast<float>(ptr[13]);
		me2->NormMat[3][2] = static_cast<float>(ptr[14]);
		me2->NormMat[3][3] = static_cast<float>(ptr[15]);
	}
}

EXPORT_CPP void _objPos(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	double cos_x = cos(rotX);
	double sin_x = sin(rotX);
	double cos_y = cos(rotY);
	double sin_y = sin(rotY);
	double cos_z = cos(rotZ);
	double sin_z = sin(rotZ);
	me2->Mat[0][0] = static_cast<float>(scaleX * (cos_y * cos_z));
	me2->Mat[0][1] = static_cast<float>(scaleX * (cos_y * sin_z));
	me2->Mat[0][2] = static_cast<float>(scaleX * (-sin_y));
	me2->Mat[0][3] = 0.0f;
	me2->Mat[1][0] = static_cast<float>(scaleY * (sin_x * sin_y * cos_z - cos_x * sin_z));
	me2->Mat[1][1] = static_cast<float>(scaleY * (sin_x * sin_y * sin_z + cos_x * cos_z));
	me2->Mat[1][2] = static_cast<float>(scaleY * (sin_x * cos_y));
	me2->Mat[1][3] = 0.0f;
	me2->Mat[2][0] = static_cast<float>(scaleZ * (cos_x * sin_y * cos_z + sin_x * sin_z));
	me2->Mat[2][1] = static_cast<float>(scaleZ * (cos_x * sin_y * sin_z - sin_x * cos_z));
	me2->Mat[2][2] = static_cast<float>(scaleZ * (cos_x * cos_y));
	me2->Mat[2][3] = 0.0f;
	me2->Mat[3][0] = static_cast<float>(transX);
	me2->Mat[3][1] = static_cast<float>(transY);
	me2->Mat[3][2] = static_cast<float>(transZ);
	me2->Mat[3][3] = 1.0f;
	scaleX = 1.0 / scaleX;
	scaleY = 1.0 / scaleY;
	scaleZ = 1.0 / scaleZ;
	me2->NormMat[0][0] = static_cast<float>(scaleX * (cos_y * cos_z));
	me2->NormMat[0][1] = static_cast<float>(scaleX * (cos_y * sin_z));
	me2->NormMat[0][2] = static_cast<float>(scaleX * (-sin_y));
	me2->NormMat[0][3] = 0.0f;
	me2->NormMat[1][0] = static_cast<float>(scaleY * (sin_x * sin_y * cos_z - cos_x * sin_z));
	me2->NormMat[1][1] = static_cast<float>(scaleY * (sin_x * sin_y * sin_z + cos_x * cos_z));
	me2->NormMat[1][2] = static_cast<float>(scaleY * (sin_x * cos_y));
	me2->NormMat[1][3] = 0.0f;
	me2->NormMat[2][0] = static_cast<float>(scaleZ * (cos_x * sin_y * cos_z + sin_x * sin_z));
	me2->NormMat[2][1] = static_cast<float>(scaleZ * (cos_x * sin_y * sin_z - sin_x * cos_z));
	me2->NormMat[2][2] = static_cast<float>(scaleZ * (cos_x * cos_y));
	me2->NormMat[2][3] = 0.0f;
	me2->NormMat[3][0] = 0.0f;
	me2->NormMat[3][1] = 0.0f;
	me2->NormMat[3][2] = 0.0f;
	me2->NormMat[3][3] = 1.0f;
}

EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ, Bool fixUp)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	double at[3] = { atX - x, atY - y, atZ - z }, up[3] = { upX, upY, upZ }, right[3];
	if (Draw::Normalize(at) == 0.0)
		return;
	if (Draw::Normalize(up) == 0.0)
		return;
	Draw::Cross(right, up, at);
	if (Draw::Normalize(right) == 0.0)
		return;
	if (fixUp)
		Draw::Cross(up, at, right);
	else
		Draw::Cross(at, right, up);
	me2->Mat[0][0] = static_cast<float>(right[0]);
	me2->Mat[0][1] = static_cast<float>(right[1]);
	me2->Mat[0][2] = static_cast<float>(right[2]);
	me2->Mat[0][3] = 0.0f;
	me2->Mat[1][0] = static_cast<float>(up[0]);
	me2->Mat[1][1] = static_cast<float>(up[1]);
	me2->Mat[1][2] = static_cast<float>(up[2]);
	me2->Mat[1][3] = 0.0f;
	me2->Mat[2][0] = static_cast<float>(at[0]);
	me2->Mat[2][1] = static_cast<float>(at[1]);
	me2->Mat[2][2] = static_cast<float>(at[2]);
	me2->Mat[2][3] = 0.0f;
	me2->Mat[3][0] = static_cast<float>(x);
	me2->Mat[3][1] = static_cast<float>(y);
	me2->Mat[3][2] = static_cast<float>(z);
	me2->Mat[3][3] = 1.0f;
	me2->NormMat[0][0] = static_cast<float>(right[0]);
	me2->NormMat[0][1] = static_cast<float>(right[1]);
	me2->NormMat[0][2] = static_cast<float>(right[2]);
	me2->NormMat[0][3] = 0.0f;
	me2->NormMat[1][0] = static_cast<float>(up[0]);
	me2->NormMat[1][1] = static_cast<float>(up[1]);
	me2->NormMat[1][2] = static_cast<float>(up[2]);
	me2->NormMat[1][3] = 0.0f;
	me2->NormMat[2][0] = static_cast<float>(at[0]);
	me2->NormMat[2][1] = static_cast<float>(at[1]);
	me2->NormMat[2][2] = static_cast<float>(at[2]);
	me2->NormMat[2][3] = 0.0f;
	me2->NormMat[3][0] = 0.0f;
	me2->NormMat[3][1] = 0.0f;
	me2->NormMat[3][2] = 0.0f;
	me2->NormMat[3][3] = 1.0f;
}

EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ, Bool fixUp)
{
	_objLook(me_, x, y, z, x + static_cast<double>(ObjVsConstBuf.CommonParam.Eye[0]), y + static_cast<double>(ObjVsConstBuf.CommonParam.Eye[1]), z + static_cast<double>(ObjVsConstBuf.CommonParam.Eye[2]), upX, upY, upZ, fixUp);
}

EXPORT_CPP void _ambLight(double topR, double topG, double topB, double bottomR, double bottomG, double bottomB)
{
	ObjPsConstBuf.CommonParam.AmbTopColor[0] = static_cast<float>(topR);
	ObjPsConstBuf.CommonParam.AmbTopColor[1] = static_cast<float>(topG);
	ObjPsConstBuf.CommonParam.AmbTopColor[2] = static_cast<float>(topB);
	ObjPsConstBuf.CommonParam.AmbBottomColor[0] = static_cast<float>(bottomR);
	ObjPsConstBuf.CommonParam.AmbBottomColor[1] = static_cast<float>(bottomG);
	ObjPsConstBuf.CommonParam.AmbBottomColor[2] = static_cast<float>(bottomB);
}

EXPORT_CPP void _dirLight(double atX, double atY, double atZ, double r, double g, double b)
{
	double dir[3] = { atX, atY, atZ };
	Draw::Normalize(dir);
	ObjVsConstBuf.CommonParam.Dir[0] = -static_cast<float>(dir[0]);
	ObjVsConstBuf.CommonParam.Dir[1] = -static_cast<float>(dir[1]);
	ObjVsConstBuf.CommonParam.Dir[2] = -static_cast<float>(dir[2]);
	ObjPsConstBuf.CommonParam.DirColor[0] = static_cast<float>(r);
	ObjPsConstBuf.CommonParam.DirColor[1] = static_cast<float>(g);
	ObjPsConstBuf.CommonParam.DirColor[2] = static_cast<float>(b);
}

EXPORT_CPP S64 _argbToColor(double a, double r, double g, double b)
{
	return Draw::ArgbToColor(a, r, g, b);
}

EXPORT_CPP void _colorToArgb(double* a, double* r, double* g, double* b, S64 color)
{
	Draw::ColorToArgb(a, r, g, b, color);
}

EXPORT_CPP void _particleDtor(SClass* me_)
{
	SParticle* me2 = reinterpret_cast<SParticle*>(me_);
	if (me2->TexSet != NULL)
	{
		for (int i = 0; i < ParticleTexNum * 2; i++)
		{
			if (me2->TexSet[i].RenderTargetViewParam != NULL)
				me2->TexSet[i].RenderTargetViewParam->Release();
			if (me2->TexSet[i].ViewParam != NULL)
				me2->TexSet[i].ViewParam->Release();
			if (me2->TexSet[i].TexParam != NULL)
				me2->TexSet[i].TexParam->Release();
		}
		FreeMem(me2->TexSet);
	}
	if (me2->TexTmp != NULL)
		me2->TexTmp->Release();
}

EXPORT_CPP void _particleDraw(SClass* me_, SClass* tex)
{
	SParticle* me2 = reinterpret_cast<SParticle*>(me_);
	UpdateParticles(me2);
	// TODO:
}

EXPORT_CPP void _particleDraw2d(SClass* me_, SClass* tex)
{
	SParticle* me2 = reinterpret_cast<SParticle*>(me_);
	UpdateParticles(me2);

	float screen[4] =
	{
		1.0f / static_cast<float>(CurWndBuf->TexWidth),
		1.0f / static_cast<float>(CurWndBuf->TexHeight),
		0.0f,
		static_cast<float>(me2->Lifespan - 1.0f)
	};
	Draw::ConstBuf(ShaderBufs[ShaderBuf_Particle2dVs], screen);
	Device->GSSetShader(NULL);
	Draw::ConstBuf(ShaderBufs[ShaderBuf_Particle2dPs], &me2->PsConstBuf);
	Draw::VertexBuf(VertexBufs[VertexBuf_ParticleVertex]);
	ID3D10ShaderResourceView* views[2];
	views[0] = me2->Draw1To2 ? me2->TexSet[0].ViewParam : me2->TexSet[ParticleTexNum + 0].ViewParam;
	views[1] = me2->Draw1To2 ? me2->TexSet[2].ViewParam : me2->TexSet[ParticleTexNum + 2].ViewParam;
	Device->VSSetShaderResources(0, 2, views);
	Device->PSSetShaderResources(0, 1, &(reinterpret_cast<STex*>(tex))->View);
	Device->DrawIndexed(ParticleNum * 6, 0, 0);
}

EXPORT_CPP void _particleEmit(SClass* me_, double x, double y, double z, double velo_x, double velo_y, double velo_z, double size, double size_velo, double rot, double rot_velo)
{
	SParticle* particle = reinterpret_cast<SParticle*>(me_);
	for (int i = 0; i < ParticleTexNum; i++)
	{
		ID3D10Texture2D* tex = particle->Draw1To2 ? particle->TexSet[i].TexParam : particle->TexSet[ParticleTexNum + i].TexParam;
		D3D10_MAPPED_TEXTURE2D map;
		Device->CopyResource(particle->TexTmp, tex);
		particle->TexTmp->Map(D3D10CalcSubresource(0, 0, 1), D3D10_MAP_READ_WRITE, 0, &map);
		float* dst = static_cast<float*>(map.pData);
		float* ptr = dst + particle->ParticlePtr * 4;
		switch (i)
		{
			case 0:
				ptr[0] = static_cast<float>(x);
				ptr[1] = static_cast<float>(y);
				ptr[2] = static_cast<float>(z);
				ptr[3] = static_cast<float>(particle->Lifespan);
				break;
			case 1:
				ptr[0] = static_cast<float>(velo_x);
				ptr[1] = static_cast<float>(velo_y);
				ptr[2] = static_cast<float>(velo_z);
				ptr[3] = 0.0f;
				break;
			case 2:
				ptr[0] = static_cast<float>(size);
				ptr[1] = static_cast<float>(size_velo);
				ptr[2] = static_cast<float>(rot);
				ptr[3] = static_cast<float>(rot_velo);
				break;
		}
		particle->TexTmp->Unmap(D3D10CalcSubresource(0, 0, 1));
		Device->CopyResource(tex, particle->TexTmp);
	}

	particle->ParticlePtr++;
	if (particle->ParticlePtr >= ParticleNum)
		particle->ParticlePtr = 0;
}

EXPORT_CPP SClass* _makeParticle(SClass* me_, S64 life_span, S64 color1, S64 color2, double friction, double accel_x, double accel_y, double accel_z, double size_accel, double rot_accel)
{
	THROWDBG(life_span <= 0, EXCPT_DBG_INOPERABLE_STATE);
	THROWDBG(friction < 0.0, EXCPT_DBG_INOPERABLE_STATE);

	SParticle* me2 = reinterpret_cast<SParticle*>(me_);
	me2->Lifespan = life_span;
	me2->ParticlePtr = 0;
	me2->Draw1To2 = True;

	{
		double a, r, g, b;
		Draw::ColorToArgb(&a, &r, &g, &b, color1);
		me2->PsConstBuf.Color1[0] = static_cast<float>(r);
		me2->PsConstBuf.Color1[1] = static_cast<float>(g);
		me2->PsConstBuf.Color1[2] = static_cast<float>(b);
		me2->PsConstBuf.Color1[3] = static_cast<float>(a);
	}
	{
		double a, r, g, b;
		Draw::ColorToArgb(&a, &r, &g, &b, color2);
		me2->PsConstBuf.Color2[0] = static_cast<float>(r);
		me2->PsConstBuf.Color2[1] = static_cast<float>(g);
		me2->PsConstBuf.Color2[2] = static_cast<float>(b);
		me2->PsConstBuf.Color2[3] = static_cast<float>(a);
	}

	SParticleUpdatingPsConstBuf const_buf;
	const_buf.AccelAndFriction[0] = static_cast<float>(accel_x);
	const_buf.AccelAndFriction[1] = static_cast<float>(accel_y);
	const_buf.AccelAndFriction[2] = static_cast<float>(accel_z);
	const_buf.AccelAndFriction[3] = static_cast<float>(friction);
	const_buf.SizeAccelAndRotAccel[0] = static_cast<float>(size_accel);
	const_buf.SizeAccelAndRotAccel[1] = static_cast<float>(rot_accel);
	const_buf.SizeAccelAndRotAccel[2] = 0.0f;
	const_buf.SizeAccelAndRotAccel[3] = 0.0f;

	{
		Bool success = True;
		void* img = AllocMem(sizeof(float) * ParticleNum * 4);
		memset(img, 0, sizeof(float) * ParticleNum * 4);
		me2->TexSet = reinterpret_cast<SParticleTexSet*>(AllocMem(sizeof(SParticleTexSet) * ParticleTexNum * 2));
		for (int i = 0; i < ParticleTexNum * 2; i++)
		{
			if (!MakeTexWithImg(&me2->TexSet[i].TexParam, &me2->TexSet[i].ViewParam, &me2->TexSet[i].RenderTargetViewParam, ParticleNum, 1, img, sizeof(float) * ParticleNum * 4, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D10_USAGE_DEFAULT, 0, True))
			{
				success = False;
				break;
			}
		}
		FreeMem(img);
		if (!success)
			return NULL;
	}
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width = static_cast<UINT>(ParticleNum);
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateTexture2D(&desc, NULL, &me2->TexTmp)))
			return False;
	}
	return me_;
}

EXPORT_CPP void _shadowDtor(SClass* me_)
{
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);
	if (me2->ShadowProjView != NULL)
		FreeMem(me2->ShadowProjView);
	if (me2->DepthResView != NULL)
		me2->DepthResView->Release();
	if (me2->DepthView != NULL)
		me2->DepthView->Release();
	if (me2->DepthTex != NULL)
		me2->DepthTex->Release();
}

EXPORT_CPP void _shadowBeginRecord(SClass* me_, double x, double y, double z, double radius)
{
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);

	double proj_mat[4][4];
	double view_mat[4][4];

	double dir_at[3] =
	{
		-static_cast<double>(ObjVsConstBuf.CommonParam.Dir[0]),
		-static_cast<double>(ObjVsConstBuf.CommonParam.Dir[1]),
		-static_cast<double>(ObjVsConstBuf.CommonParam.Dir[2])
	};
	double len = Draw::Normalize(dir_at);
	THROWDBG(len == 0.0, EXCPT_DBG_INOPERABLE_STATE);
	
	double eye_pos[3] =
	{
		x - dir_at[0] * radius,
		y - dir_at[1] * radius,
		z - dir_at[2] * radius
	};

	{
		proj_mat[0][0] = -1.0 / radius;
		proj_mat[0][1] = 0.0;
		proj_mat[0][2] = 0.0;
		proj_mat[0][3] = 0.0;
		proj_mat[1][0] = 0.0;
		proj_mat[1][1] = 1.0 / radius;
		proj_mat[1][2] = 0.0;
		proj_mat[1][3] = 0.0;
		proj_mat[2][0] = 0.0;
		proj_mat[2][1] = 0.0;
		proj_mat[2][2] = 0.5 / radius;
		proj_mat[2][3] = 0.0;
		proj_mat[3][0] = 0.0;
		proj_mat[3][1] = 0.0;
		proj_mat[3][2] = 0.0;
		proj_mat[3][3] = 1.0;
	}

	{
		double up[3], right[3], eye[3], pxyz[3];

		up[0] = 0.0;
		up[1] = 1.0;
		up[2] = 0.0;
		Draw::Cross(right, up, dir_at);
		if (Draw::Normalize(right) == 0.0)
		{
			up[0] = 0.0;
			up[1] = 0.0;
			up[2] = 1.0;
			Draw::Cross(right, up, dir_at);
			Draw::Normalize(right);
		}

		Draw::Cross(up, dir_at, right);

		eye[0] = eye_pos[0];
		eye[1] = eye_pos[1];
		eye[2] = eye_pos[2];
		pxyz[0] = Draw::Dot(eye, right);
		pxyz[1] = Draw::Dot(eye, up);
		pxyz[2] = Draw::Dot(eye, dir_at);

		view_mat[0][0] = right[0];
		view_mat[0][1] = up[0];
		view_mat[0][2] = dir_at[0];
		view_mat[0][3] = 0.0;
		view_mat[1][0] = right[1];
		view_mat[1][1] = up[1];
		view_mat[1][2] = dir_at[1];
		view_mat[1][3] = 0.0;
		view_mat[2][0] = right[2];
		view_mat[2][1] = up[2];
		view_mat[2][2] = dir_at[2];
		view_mat[2][3] = 0.0;
		view_mat[3][0] = -pxyz[0];
		view_mat[3][1] = -pxyz[1];
		view_mat[3][2] = -pxyz[2];
		view_mat[3][3] = 1.0;
	}

	double proj_view[4][4];
	Draw::MulMat(proj_view, proj_mat, view_mat);
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			me2->ShadowProjView[i][j] = static_cast<float>(proj_view[i][j]);
	}
	{
		double uv_mat[4][4];
		uv_mat[0][0] = 0.5;
		uv_mat[0][1] = 0.0;
		uv_mat[0][2] = 0.0;
		uv_mat[0][3] = 0.0;
		uv_mat[1][0] = 0.0;
		uv_mat[1][1] = -0.5;
		uv_mat[1][2] = 0.0;
		uv_mat[1][3] = 0.0;
		uv_mat[2][0] = 0.0;
		uv_mat[2][1] = 0.0;
		uv_mat[2][2] = 1.0;
		uv_mat[2][3] = 0.0;
		uv_mat[3][0] = 0.5;
		uv_mat[3][1] = 0.5;
		uv_mat[3][2] = 0.0;
		uv_mat[3][3] = 1.0;
		Draw::SetProjViewMat(ObjVsConstBuf.CommonParam.ShadowProjView, uv_mat, proj_view);
	}

	Device->OMSetRenderTargets(0, NULL, me2->DepthView);
	Device->ClearDepthStencilView(me2->DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);

	D3D10_VIEWPORT viewport =
	{
		0,
		0,
		static_cast<UINT>(me2->DepthWidth),
		static_cast<UINT>(me2->DepthHeight),
		0.0f,
		1.0f,
	};
	Device->RSSetViewports(1, &viewport);
}

EXPORT_CPP void _shadowEndRecord(SClass* me_)
{
	Device->OMSetRenderTargets(1, &CurWndBuf->TmpRenderTargetView, CurWndBuf->DepthView);
	Draw::ResetViewport();
}

EXPORT_CPP void _shadowAdd(SClass* me_, SClass* obj, S64 element, double frame)
{
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);
	SObj* obj2 = reinterpret_cast<SObj*>(obj);
	THROWDBG(element < 0 || static_cast<S64>(obj2->ElementNum) <= element, EXCPT_DBG_ARG_OUT_DOMAIN);
	SObjShadowVsConstBuf const_buf;
	switch (obj2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(obj2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), EXCPT_DBG_ARG_OUT_DOMAIN);
				THROWDBG(element2->JointNum < 0 || JointMax < element2->JointNum, EXCPT_DBG_ARG_OUT_DOMAIN);
				Bool joint = element2->JointNum != 0;

				memcpy(const_buf.World, obj2->Mat, sizeof(float[4][4]));
				memcpy(const_buf.ProjView, me2->ShadowProjView, sizeof(float[4][4]));
				if ((obj2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, const_buf.Joint);
						Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjShadowJointVs], &const_buf);
					}
					else
						Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjShadowVs], &const_buf);
					Device->GSSetShader(NULL);
					Device->PSSetShader(NULL);
					Draw::VertexBuf(element2->VertexBuf);
				}
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

EXPORT_CPP SClass* _makeShadow(SClass* me_, S64 width, S64 height)
{
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);
	{
		Bool success = False;
		for (; ; )
		{
			{
				D3D10_TEXTURE2D_DESC desc;
				memset(&desc, 0, sizeof(desc));
				desc.Width = static_cast<UINT>(width);
				desc.Height = static_cast<UINT>(height);
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_R32_TYPELESS;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = D3D10_USAGE_DEFAULT;
				desc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				if (FAILED(Device->CreateTexture2D(&desc, NULL, &me2->DepthTex)))
					break;
			}
			{
				D3D10_DEPTH_STENCIL_VIEW_DESC desc;
				memset(&desc, 0, sizeof(desc));
				desc.Format = DXGI_FORMAT_D32_FLOAT;
				desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = 0;
				if (FAILED(Device->CreateDepthStencilView(me2->DepthTex, &desc, &me2->DepthView)))
					break;
			}
			{
				D3D10_SHADER_RESOURCE_VIEW_DESC desc;
				memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
				desc.Format = DXGI_FORMAT_R32_FLOAT;
				desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MostDetailedMip = 0;
				desc.Texture2D.MipLevels = 1;
				if (FAILED(Device->CreateShaderResourceView(me2->DepthTex, &desc, &me2->DepthResView)))
					return False;
			}
			success = True;
			break;
		}
		if (!success)
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}
	me2->DepthWidth = static_cast<int>(width);
	me2->DepthHeight = static_cast<int>(height);
	me2->ShadowProjView = static_cast<float(*)[4]>(AllocMem(sizeof(float) * 4 * 4));
	return me_;
}

static Bool MakeTexWithImg(ID3D10Texture2D** tex, ID3D10ShaderResourceView** view, ID3D10RenderTargetView** render_target_view, int width, int height, const void* img, size_t pitch, DXGI_FORMAT fmt, D3D10_USAGE usage, UINT cpu_access_flag, Bool render_target)
{
	{
		D3D10_TEXTURE2D_DESC desc;
		D3D10_SUBRESOURCE_DATA sub;
		desc.Width = static_cast<UINT>(width);
		desc.Height = static_cast<UINT>(height);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = fmt;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = usage;
		desc.BindFlags = render_target ? (D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE) : D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = cpu_access_flag;
		desc.MiscFlags = 0;
		sub.pSysMem = img;
		sub.SysMemPitch = static_cast<UINT>(pitch);
		sub.SysMemSlicePitch = 0;
		if (FAILED(Device->CreateTexture2D(&desc, &sub, tex)))
			return False;
	}
	{
		D3D10_SHADER_RESOURCE_VIEW_DESC desc;
		memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
		desc.Format = fmt;
		desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;
		if (FAILED(Device->CreateShaderResourceView(*tex, &desc, view)))
			return False;
	}
	if (render_target_view != NULL)
	{
		if (FAILED(Device->CreateRenderTargetView(*tex, NULL, render_target_view)))
			return False;
	}
	return True;
}

static void UpdateParticles(SParticle* particle)
{
	int old_z_buf = CurZBuf;
	int old_blend = CurBlend;
	int old_sampler = CurSampler;
	_depth(False, False);
	_blend(0);
	_sampler(0);

	ID3D10RenderTargetView* targets[ParticleTexNum];
	const int particle_tex_idx = particle->Draw1To2 ? ParticleTexNum : 0;
	for (int i = 0; i < ParticleTexNum; i++)
		targets[i] = particle->TexSet[particle_tex_idx + i].RenderTargetViewParam;
	Device->OMSetRenderTargets(static_cast<UINT>(ParticleTexNum), targets, NULL);
	Device->RSSetViewports(1, &ParticleViewport);
	{
		Draw::ConstBuf(ShaderBufs[ShaderBuf_ParticleUpdatingVs], NULL);
		Device->GSSetShader(NULL);
		Draw::ConstBuf(ShaderBufs[ShaderBuf_ParticleUpdatingPs], &particle->UpdatingPsConstBuf);
		Draw::VertexBuf(VertexBufs[VertexBuf_ParticleUpdatingVertex]);
		ID3D10ShaderResourceView* views[3];
		views[0] = particle->Draw1To2 ? particle->TexSet[0].ViewParam : particle->TexSet[ParticleTexNum + 0].ViewParam;
		views[1] = particle->Draw1To2 ? particle->TexSet[1].ViewParam : particle->TexSet[ParticleTexNum + 1].ViewParam;
		views[2] = particle->Draw1To2 ? particle->TexSet[2].ViewParam : particle->TexSet[ParticleTexNum + 2].ViewParam;
		Device->PSSetShaderResources(0, 3, views);
	}
	Device->DrawIndexed(6, 0, 0);

	_depth((old_z_buf & 2) != 0, (old_z_buf & 1) != 0);
	_blend(old_blend);
	_sampler(old_sampler);

	Device->OMSetRenderTargets(1, &CurWndBuf->TmpRenderTargetView, CurWndBuf->DepthView);
	Draw::ResetViewport();

	particle->Draw1To2 = !particle->Draw1To2;
}

static void WriteBack()
{
	if (Callback2d != NULL)
		Callback2d(3, NULL, NULL);
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

static SClass* MakeObjImpl(SClass* me_, size_t size, const void* binary)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	me2->ElementKinds = NULL;
	me2->Elements = NULL;
	Draw::IdentityFloat(me2->Mat);
	Draw::IdentityFloat(me2->NormMat);
	{
		Bool correct = True;
		U32* idces = NULL;
		U8* vertices = NULL;
		const U8* buf = static_cast<const U8*>(binary);
		for (; ; )
		{
			size_t ptr = 0;
			if (ptr + sizeof(int) * 3 > size)
			{
				correct = False;
				break;
			}
			
			int version = *reinterpret_cast<const int*>(buf + ptr);
			UNUSED(version);
			ptr += sizeof(int);
			ASSERT(version == 1);
			me2->Format = static_cast<EFormat>(*reinterpret_cast<const int*>(buf + ptr));
			ptr += sizeof(int);
			int vertex_size = 8;
			int vertex_size_aligned = 8;
			if ((me2->Format & Format_HasJoint) != 0)
			{
				vertex_size += JointInfluenceMax;
				vertex_size_aligned += JointInfluenceMaxAligned;
			}
			if ((me2->Format & Format_HasTangent) != 0)
			{
				vertex_size += 3;
				vertex_size_aligned += 3;
			}
			int joint_influence_size = 0;
			int joint_influence_size_aligned = 0;
			if ((me2->Format & Format_HasJoint) != 0)
			{
				joint_influence_size += JointInfluenceMax;
				joint_influence_size_aligned += JointInfluenceMaxAligned;
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
							vertices = static_cast<U8*>(AllocMem((sizeof(float) * vertex_size_aligned + sizeof(int) * joint_influence_size_aligned) * static_cast<size_t>(idx_num)));
							U8* ptr2 = vertices;
							if (ptr + (sizeof(float) * vertex_size + sizeof(int) * joint_influence_size) * static_cast<size_t>(idx_num) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < idx_num; j++)
							{
								for (int k = 0; k < vertex_size; k++)
								{
									*reinterpret_cast<float*>(ptr2) = *reinterpret_cast<const float*>(buf + ptr);
									ptr += sizeof(float);
									ptr2 += sizeof(float);
								}
								for (int k = 0; k < vertex_size_aligned - vertex_size; k++)
								{
									*reinterpret_cast<float*>(ptr2) = 0.0f;
									ptr2 += sizeof(float);
								}
								for (int k = 0; k < joint_influence_size; k++)
								{
									*reinterpret_cast<int*>(ptr2) = *reinterpret_cast<const int*>(buf + ptr);
									ptr += sizeof(int);
									ptr2 += sizeof(int);
								}
								for (int k = 0; k < joint_influence_size_aligned - joint_influence_size; k++)
								{
									*reinterpret_cast<int*>(ptr2) = 0;
									ptr2 += sizeof(int);
								}
							}
							element->VertexBuf = Draw::MakeVertexBuf((sizeof(float) * vertex_size_aligned + sizeof(int) * joint_influence_size_aligned) * static_cast<size_t>(idx_num), vertices, sizeof(float) * vertex_size_aligned + sizeof(int) * joint_influence_size_aligned, sizeof(U32) * static_cast<size_t>(element->VertexNum), idces);
							if (ptr + sizeof(int) * 3 > size)
							{
								correct = False;
								break;
							}
							element->JointNum = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							element->Begin = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							element->End = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							element->Joints = static_cast<float(*)[4][4]>(AllocMem(sizeof(float[4][4]) * static_cast<size_t>(element->JointNum * (element->End - element->Begin + 1))));
							if (ptr + sizeof(float[4][4]) * static_cast<size_t>(element->JointNum * (element->End - element->Begin + 1)) > size)
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
											element->Joints[j * (element->End - element->Begin + 1) + k][l][m] = *reinterpret_cast<const float*>(buf + ptr);
											ptr += sizeof(float);
										}
									}
								}
							}
						}
						break;
					default:
						THROW(EXCPT_INVALID_DATA_FMT);
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
		if (!correct)
		{
			_objDtor(me_);
			THROW(EXCPT_INVALID_DATA_FMT);
			return NULL;
		}
	}
	return me_;
}

static void WriteFastPsConstBuf(SObjFastPsConstBuf* ps_const_buf)
{
	ps_const_buf->CommonParam = ObjPsConstBuf.CommonParam;
	double eye[3] =
	{
		static_cast<double>(ObjVsConstBuf.CommonParam.Eye[0]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Eye[1]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Eye[2])
	};
	Draw::Normalize(eye);
	double dir[3] =
	{
		static_cast<double>(ObjVsConstBuf.CommonParam.Dir[0]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Dir[1]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Dir[2])
	};
	Draw::Normalize(dir);
	double half[3] =
	{
		static_cast<double>(dir[0] + eye[0]),
		static_cast<double>(dir[1] + eye[1]),
		static_cast<double>(dir[2] + eye[2])
	};
	Draw::Normalize(half);
	ps_const_buf->Eye[0] = static_cast<float>(eye[0]);
	ps_const_buf->Eye[1] = static_cast<float>(eye[1]);
	ps_const_buf->Eye[2] = static_cast<float>(eye[2]);
	ps_const_buf->Eye[3] = 0.0f;
	ps_const_buf->Dir[0] = static_cast<float>(dir[0]);
	ps_const_buf->Dir[1] = static_cast<float>(dir[1]);
	ps_const_buf->Dir[2] = static_cast<float>(dir[2]);
	ps_const_buf->Dir[3] = 0.0f;
	ps_const_buf->Half[0] = static_cast<float>(half[0]);
	ps_const_buf->Half[1] = static_cast<float>(half[1]);
	ps_const_buf->Half[2] = static_cast<float>(half[2]);
	ps_const_buf->Half[3] = static_cast<float>(pow(max(1.0 - Draw::Dot(eye, half), 0.0), 5.0));
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
			ExtTextOut(me->Dc, static_cast<int>(rect.left), static_cast<int>(rect.top), ETO_CLIPPED | ETO_OPAQUE, &rect, &c, 1, NULL);
			{
				GLYPHMETRICS gm;
				MAT2 mat = { { 0, 1 },{ 0, 0 },{ 0, 0 },{ 0, 1 } };
				GetGlyphOutline(me->Dc, static_cast<UINT>(c), GGO_METRICS, &gm, 0, NULL, &mat);
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

static void ObjDrawImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, EXCPT_DBG_ARG_OUT_DOMAIN);
	Device->PSSetSamplers(1, 1, &Sampler[2]);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), EXCPT_DBG_ARG_OUT_DOMAIN);
				THROWDBG(element2->JointNum < 0 || JointMax < element2->JointNum, EXCPT_DBG_ARG_OUT_DOMAIN);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.CommonParam.World, me2->Mat, sizeof(float[4][4]));
				memcpy(ObjVsConstBuf.CommonParam.NormWorld, me2->NormMat, sizeof(float[4][4]));
				if ((me2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjFastJointVs] : ShaderBufs[ShaderBuf_ObjFastJointSmVs], &ObjVsConstBuf);
					}
					else
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjFastVs] : ShaderBufs[ShaderBuf_ObjFastSmVs], &ObjVsConstBuf);
					Device->GSSetShader(NULL);

					SObjFastPsConstBuf ps_const_buf;
					WriteFastPsConstBuf(&ps_const_buf);
					Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjFastPs] : ShaderBufs[ShaderBuf_ObjFastSmPs], &ps_const_buf);
					Draw::VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[3];
					views[0] = diffuse == NULL ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == NULL ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					if (shadow == NULL)
						Device->PSSetShaderResources(0, 2, views);
					else
					{
						views[2] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 3, views);
					}
				}
				else
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjJointVs] : ShaderBufs[ShaderBuf_ObjJointSmVs], &ObjVsConstBuf);
					}
					else
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjVs] : ShaderBufs[ShaderBuf_ObjSmVs], &ObjVsConstBuf);
					Device->GSSetShader(NULL);
					Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjPs] : ShaderBufs[ShaderBuf_ObjSmPs], &ObjPsConstBuf);
					Draw::VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[4];
					views[0] = diffuse == NULL ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == NULL ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					views[2] = normal == NULL ? ViewEven[2] : reinterpret_cast<STex*>(normal)->View;
					if (shadow == NULL)
						Device->PSSetShaderResources(0, 3, views);
					else
					{
						views[3] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 4, views);
					}
				}
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

static void ObjDrawToonImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, EXCPT_DBG_ARG_OUT_DOMAIN);
	Device->PSSetSamplers(1, 1, &Sampler[2]);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), EXCPT_DBG_ARG_OUT_DOMAIN);
				THROWDBG(element2->JointNum < 0 || JointMax < element2->JointNum, EXCPT_DBG_ARG_OUT_DOMAIN);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.CommonParam.World, me2->Mat, sizeof(float[4][4]));
				memcpy(ObjVsConstBuf.CommonParam.NormWorld, me2->NormMat, sizeof(float[4][4]));
				if ((me2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjFastJointVs] : ShaderBufs[ShaderBuf_ObjFastJointSmVs], &ObjVsConstBuf);
					}
					else
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjFastVs] : ShaderBufs[ShaderBuf_ObjFastSmVs], &ObjVsConstBuf);
					Device->GSSetShader(NULL);

					SObjFastPsConstBuf ps_const_buf;
					WriteFastPsConstBuf(&ps_const_buf);
					Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjToonFastPs] : ShaderBufs[ShaderBuf_ObjToonFastSmPs], &ps_const_buf);
					Draw::VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[4];
					views[0] = diffuse == NULL ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == NULL ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					views[2] = ViewToonRamp;
					if (shadow == NULL)
						Device->PSSetShaderResources(0, 3, views);
					else
					{
						views[3] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 4, views);
					}
				}
				else
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjJointVs] : ShaderBufs[ShaderBuf_ObjJointSmVs], &ObjVsConstBuf);
					}
					else
						Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjVs] : ShaderBufs[ShaderBuf_ObjSmVs], &ObjVsConstBuf);
					Device->GSSetShader(NULL);
					Draw::ConstBuf(shadow == NULL ? ShaderBufs[ShaderBuf_ObjToonPs] : ShaderBufs[ShaderBuf_ObjToonSmPs], &ObjPsConstBuf);
					Draw::VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[5];
					views[0] = diffuse == NULL ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == NULL ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					views[2] = normal == NULL ? ViewEven[2] : reinterpret_cast<STex*>(normal)->View;
					views[3] = ViewToonRamp;
					if (shadow == NULL)
						Device->PSSetShaderResources(0, 4, views);
					else
					{
						views[4] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 5, views);
					}
				}
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

static void ObjDrawFlatImpl(SClass* me_, S64 element, double frame, SClass* diffuse)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, EXCPT_DBG_ARG_OUT_DOMAIN);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), EXCPT_DBG_ARG_OUT_DOMAIN);
				THROWDBG(element2->JointNum < 0 || JointMax < element2->JointNum, EXCPT_DBG_ARG_OUT_DOMAIN);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.CommonParam.World, me2->Mat, sizeof(float[4][4]));
				if ((me2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjFlatFastJointVs], &ObjVsConstBuf);
					}
					else
						Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjFlatFastVs], &ObjVsConstBuf);
				}
				else
				{
					if (joint && frame >= 0.0f)
					{
						Draw::SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjFlatJointVs], &ObjVsConstBuf);
					}
					else
						Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjFlatVs], &ObjVsConstBuf);
				}
				Device->GSSetShader(NULL);
				Draw::ConstBuf(ShaderBufs[ShaderBuf_ObjFlatPs], NULL);
				Draw::VertexBuf(element2->VertexBuf);
				ID3D10ShaderResourceView* views[1];
				views[0] = diffuse == NULL ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
				Device->PSSetShaderResources(0, 1, views);

				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

namespace Draw
{

void Init()
{
	{
		const D3D10_DRIVER_TYPE type[] =
		{
			D3D10_DRIVER_TYPE_HARDWARE,
			D3D10_DRIVER_TYPE_WARP
		};
		const D3D10_FEATURE_LEVEL1 level[] =
		{
			D3D10_FEATURE_LEVEL_10_1,
			D3D10_FEATURE_LEVEL_10_0,
			D3D10_FEATURE_LEVEL_9_3,
			D3D10_FEATURE_LEVEL_9_2,
			D3D10_FEATURE_LEVEL_9_1
		};
		Bool success = False;
		for (int i = 0; i < sizeof(type) / sizeof(type[0]); i++)
		{
			for (int j = 0; j < sizeof(level) / sizeof(level[0]); j++)
			{
				if (SUCCEEDED(D3D10CreateDevice1(NULL, type[i], NULL, D3D10_CREATE_DEVICE_BGRA_SUPPORT, level[j], D3D10_1_SDK_VERSION, &Device)))
				{
					success = True;
					break;
				}
			}
			if (success)
				break;
		}
		if (!success)
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	Cnt = 0;
	PrevTime = static_cast<U32>(timeGetTime());

	// Create a rasterizer state.
	{
		D3D10_RASTERIZER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.FillMode = D3D10_FILL_SOLID;
		desc.CullMode = D3D10_CULL_FRONT; // Cull the front in order to exchange the right handed system and the left handed system.
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = FALSE;
		desc.ScissorEnable = FALSE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerState)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
		desc.CullMode = D3D10_CULL_BACK;
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerStateInverted)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
		desc.CullMode = D3D10_CULL_NONE;
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerStateNone)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
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
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	// Create blend modes.
	for (int i = 0; i < BlendNum; i++)
	{
		D3D10_BLEND_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.AlphaToCoverageEnable = FALSE;
		desc.SrcBlendAlpha = D3D10_BLEND_ONE;
		desc.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
		desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
		for (int j = 0; j < 8; j++)
		{
			desc.BlendEnable[j] = i == 0 ? FALSE : TRUE;
			desc.RenderTargetWriteMask[j] = D3D10_COLOR_WRITE_ENABLE_ALL;
		}
		switch (i)
		{
			// None: S * 1 + D * 0.
			case 0:
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
			// Exclusion: S * (1 - D) + D * (S - 1).
			case 5:
				desc.SrcBlend = D3D10_BLEND_INV_DEST_COLOR;
				desc.DestBlend = D3D10_BLEND_INV_SRC_COLOR;
				desc.BlendOp = D3D10_BLEND_OP_ADD;
				break;
			default:
				ASSERT(False);
				break;
		}
		if (FAILED(Device->CreateBlendState(&desc, &BlendState[i])))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	// Create a sampler.
	for (int i = 0; i < SamplerNum; i++)
	{
		D3D10_SAMPLER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.AddressU = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.AddressV = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.AddressW = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 0;
		desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
		desc.MinLOD = -D3D10_FLOAT32_MAX;
		desc.MaxLOD = D3D10_FLOAT32_MAX;
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
			// Shadow
			case 2:
				desc.AddressU = D3D10_TEXTURE_ADDRESS_BORDER;
				desc.AddressV = D3D10_TEXTURE_ADDRESS_BORDER;
				desc.AddressW = D3D10_TEXTURE_ADDRESS_BORDER;
				desc.BorderColor[0] = 1.0f;
				desc.BorderColor[1] = 1.0f;
				desc.BorderColor[2] = 1.0f;
				desc.BorderColor[3] = 1.0f;
				desc.ComparisonFunc = D3D10_COMPARISON_LESS_EQUAL;
				desc.Filter = D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
				break;
		}
		if (FAILED(Device->CreateSamplerState(&desc, &Sampler[i])))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	// Initialize 'Tri'.
	{
		{
			float vertices[] =
			{
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
				0.0f, 1.0f, 0.0f,
			};

			U32 idces[] =
			{
				0, 1, 2,
			};

			VertexBufs[VertexBuf_TriVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 3, sizeof(idces), idces);
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
				ShaderBufs[ShaderBuf_TriVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 8, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetTriPsBin(&size);
				ShaderBufs[ShaderBuf_TriPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Rect'.
	{
		{
			float vertices[] =
			{
				0.0, 0.0,
				0.0, 1.0,
				1.0, 0.0,
				1.0, 1.0,
			};

			U32 idces[] =
			{
				0, 1, 2,
				3, 2, 1,
			};

			VertexBufs[VertexBuf_RectVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
		}

		{
			float vertices[] =
			{
				0.0, 0.0,
				1.0, 1.0,
			};

			U32 idces[] =
			{
				0, 1,
			};

			VertexBufs[VertexBuf_LineVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
		}

		{
			float vertices[] =
			{
				0.0, 0.0,
				0.0, 1.0,
				1.0, 1.0,
				1.0, 0.0,
			};

			U32 idces[] =
			{
				0, 1, 2, 3, 0
			};

			VertexBufs[VertexBuf_RectLineVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
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
				ShaderBufs[ShaderBuf_RectVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 4, 1, layout_types, layout_semantics);
			}
		}
	}

	if (IsResUsed(UseResFlagsKind_Draw_Circle))
	{
		// Initialize 'Circle'.
		{
			float vertices[] =
			{
				-1.0f, -1.0f,
				-1.0f, 1.0f,
				1.0f, -1.0f,
				1.0f, 1.0f,
			};

			U32 idces[] =
			{
				0, 1, 2,
				3, 2, 1,
			};

			VertexBufs[VertexBuf_CircleVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
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
				ShaderBufs[ShaderBuf_CircleVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 4, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetCirclePsBin(&size);
				ShaderBufs[ShaderBuf_CirclePs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 8, 0, NULL, NULL);
			}
		}

		// Initialize 'CircleLine'.
		{
			size_t size;
			const U8* bin = GetCircleLinePsBin(&size);
			ShaderBufs[ShaderBuf_CircleLinePs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 8, 0, NULL, NULL);
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
				ShaderBufs[ShaderBuf_TexVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 8, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetTexRotVsBin(&size);
				ShaderBufs[ShaderBuf_TexRotVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 16, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetTexPsBin(&size);
				ShaderBufs[ShaderBuf_TexPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetFontPsBin(&size);
				ShaderBufs[ShaderBuf_FontPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Obj'.
	if (IsResUsed(UseResFlagsKind_Draw_ObjDraw))
	{
		{
			ELayoutType layout_types[4] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
			};

			const Char* layout_semantics[4] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"TEXCOORD",
			};

			{
				size_t size;
				const U8* bin = GetObjVsBin(&size);
				ShaderBufs[ShaderBuf_ObjVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf) - sizeof(SObjVsConstBuf::Joint), 4, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjSmVsBin(&size);
				ShaderBufs[ShaderBuf_ObjSmVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf) - sizeof(SObjVsConstBuf::Joint), 4, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjPsBin(&size);
				ShaderBufs[ShaderBuf_ObjPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetObjSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetObjToonPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetObjToonSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, NULL, NULL);
			}
		}
		{
			ELayoutType layout_types[7] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[6] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"TEXCOORD",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjJointVsBin(&size);
				ShaderBufs[ShaderBuf_ObjJointVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 6, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjJointSmVsBin(&size);
				ShaderBufs[ShaderBuf_ObjJointSmVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 6, layout_types, layout_semantics);
			}
		}
	}

	// Initialize 'ObjFast'.
	if (IsResUsed(UseResFlagsKind_Draw_ObjDraw))
	{
		{
			ELayoutType layout_types[3] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
			};

			const Char* layout_semantics[3] =
			{
				L"POSITION",
				L"NORMAL",
				L"TEXCOORD",
			};

			{
				size_t size;
				const U8* bin = GetObjFastVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf) - sizeof(SObjVsConstBuf::Joint), 3, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjFastSmVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastSmVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf) - sizeof(SObjVsConstBuf::Joint), 3, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjFastPsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetObjFastSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetObjToonFastPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonFastPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, NULL, NULL);
			}
			{
				size_t size;
				const U8* bin = GetObjToonFastSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonFastSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, NULL, NULL);
			}
		}
		{
			ELayoutType layout_types[5] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[5] =
			{
				L"POSITION",
				L"NORMAL",
				L"TEXCOORD",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjFastJointVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastJointVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 5, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjFastJointSmVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastJointSmVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 5, layout_types, layout_semantics);
			}
		}
	}

	// Initialize 'ObjFlat'.
	if (IsResUsed(UseResFlagsKind_Draw_ObjDraw))
	{
		{
			ELayoutType layout_types[4] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
			};

			const Char* layout_semantics[4] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"TEXCOORD",
			};

			{
				size_t size;
				const U8* bin = GetObjFlatVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFlatVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf) - sizeof(SObjVsConstBuf::Joint), 4, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjFlatPsBin(&size);
				ShaderBufs[ShaderBuf_ObjFlatPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, NULL, NULL);
			}
		}
		{
			ELayoutType layout_types[7] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[6] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"TEXCOORD",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjFlatJointVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFlatJointVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 6, layout_types, layout_semantics);
			}
		}
		{
			ELayoutType layout_types[3] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
			};

			const Char* layout_semantics[3] =
			{
				L"POSITION",
				L"NORMAL",
				L"TEXCOORD",
			};

			{
				size_t size;
				const U8* bin = GetObjFlatFastVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFlatFastVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf) - sizeof(SObjVsConstBuf::Joint), 3, layout_types, layout_semantics);
			}
		}
		{
			ELayoutType layout_types[5] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[5] =
			{
				L"POSITION",
				L"NORMAL",
				L"TEXCOORD",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjFlatFastJointVsBin(&size);
				ShaderBufs[ShaderBuf_ObjFlatFastJointVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjVsConstBuf), 5, layout_types, layout_semantics);
			}
		}
	}

	// Initialize 'ObjOutline'.
	if (IsResUsed(UseResFlagsKind_Draw_ObjDrawOutline))
	{
		{
			ELayoutType layout_types[4] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
			};

			const Char* layout_semantics[4] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"TEXCOORD",
			};

			{
				size_t size;
				const U8* bin = GetObjOutlineVsBin(&size);
				ShaderBufs[ShaderBuf_ObjOutlineVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjOutlineVsConstBuf) - sizeof(SObjOutlineVsConstBuf::Joint), 4, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetObjOutlinePsBin(&size);
				ShaderBufs[ShaderBuf_ObjOutlinePs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjOutlinePsConstBuf), 0, NULL, NULL);
			}
		}
		{
			ELayoutType layout_types[7] =
			{
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float3,
				LayoutType_Float2,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[6] =
			{
				L"POSITION",
				L"NORMAL",
				L"TANGENT",
				L"TEXCOORD",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjOutlineJointVsBin(&size);
				ShaderBufs[ShaderBuf_ObjOutlineJointVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjOutlineVsConstBuf), 6, layout_types, layout_semantics);
			}
		}
	}

	// Initialize 'ObjShadow'.
	if (IsResUsed(UseResFlagsKind_Draw_ObjDraw))
	{
		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float3,
			};

			const Char* layout_semantics[1] =
			{
				L"POSITION",
			};

			{
				size_t size;
				const U8* bin = GetObjShadowVsBin(&size);
				ShaderBufs[ShaderBuf_ObjShadowVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjShadowVsConstBuf) - sizeof(SObjShadowVsConstBuf::Joint), 1, layout_types, layout_semantics);
			}
		}
		{
			ELayoutType layout_types[3] =
			{
				LayoutType_Float3,
				LayoutType_Float4,
				LayoutType_Int4,
			};

			const Char* layout_semantics[3] =
			{
				L"POSITION",
				L"K_WEIGHT",
				L"K_JOINT",
			};

			{
				size_t size;
				const U8* bin = GetObjShadowJointVsBin(&size);
				ShaderBufs[ShaderBuf_ObjShadowJointVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(SObjShadowVsConstBuf), 3, layout_types, layout_semantics);
			}
		}
	}

	// Initialize 'Filter'.
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

			VertexBufs[VertexBuf_FilterVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
		}

		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float2,
			};

			const Char* layout_semantics[1] =
			{
				L"K_POSITION",
			};

			{
				size_t size;
				const U8* bin = GetFilterVsBin(&size);
				ShaderBufs[ShaderBuf_FilterVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, 0, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetFilterNonePsBin(&size);
				ShaderBufs[ShaderBuf_Filter0Ps] = MakeShaderBuf(ShaderKind_Ps, size, bin, 0, 0, NULL, NULL);
			}
			if (IsResUsed(UseResFlagsKind_Draw_FilterMonotone))
			{
				size_t size;
				const U8* bin = GetFilterMonotonePsBin(&size);
				ShaderBufs[ShaderBuf_Filter1Ps] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Particle'.
	if (IsResUsed(UseResFlagsKind_Draw_Particle))
	{
		{
			float vertices[ParticleNum * 4 * 3];
			U32 idces[ParticleNum * 6];
			for (int i = 0; i < ParticleNum; i++)
			{
				const float idx = (static_cast<float>(i) + 0.5f) / static_cast<float>(ParticleNum);
				vertices[i * 4 * 3 + 0] = -1.0f;
				vertices[i * 4 * 3 + 1] = -1.0f;
				vertices[i * 4 * 3 + 2] = idx;
				vertices[i * 4 * 3 + 3] = 1.0f;
				vertices[i * 4 * 3 + 4] = -1.0f;
				vertices[i * 4 * 3 + 5] = idx;
				vertices[i * 4 * 3 + 6] = -1.0f;
				vertices[i * 4 * 3 + 7] = 1.0f;
				vertices[i * 4 * 3 + 8] = idx;
				vertices[i * 4 * 3 + 9] = 1.0f;
				vertices[i * 4 * 3 + 10] = 1.0f;
				vertices[i * 4 * 3 + 11] = idx;
				idces[i * 6 + 0] = i * 4 + 0;
				idces[i * 6 + 1] = i * 4 + 1;
				idces[i * 6 + 2] = i * 4 + 2;
				idces[i * 6 + 3] = i * 4 + 3;
				idces[i * 6 + 4] = i * 4 + 2;
				idces[i * 6 + 5] = i * 4 + 1;
			}

			VertexBufs[VertexBuf_ParticleVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 3, sizeof(idces), idces);
		}
		{
			ELayoutType layout_types[2] =
			{
				LayoutType_Float2,
				LayoutType_Float1,
			};

			const Char* layout_semantics[2] =
			{
				L"K_POSITION",
				L"K_IDX",
			};

			{
				size_t size;
				const U8* bin = GetParticle2dVsBin(&size);
				ShaderBufs[ShaderBuf_Particle2dVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 4, 2, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetParticle2dPsBin(&size);
				ShaderBufs[ShaderBuf_Particle2dPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SParticlePsConstBuf), 0, NULL, NULL);
			}
		}
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

			VertexBufs[VertexBuf_ParticleUpdatingVertex] = MakeVertexBuf(sizeof(vertices), vertices, sizeof(float) * 2, sizeof(idces), idces);
		}
		{
			ELayoutType layout_types[1] =
			{
				LayoutType_Float2,
			};

			const Char* layout_semantics[1] =
			{
				L"K_POSITION",
			};

			{
				size_t size;
				const U8* bin = GetParticleUpdatingVsBin(&size);
				ShaderBufs[ShaderBuf_ParticleUpdatingVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, 0, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetParticleUpdatingPsBin(&size);
				ShaderBufs[ShaderBuf_ParticleUpdatingPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SParticleUpdatingPsConstBuf), 0, NULL, NULL);
			}
		}
	}

	// Initialize 'Particle'.
	if (IsResUsed(UseResFlagsKind_Draw_Poly))
	{
		{
			int vertex_idx[PolyVerticesNum];
			U32 idces[PolyVerticesNum];
			for (int i = 0; i < PolyVerticesNum; i++)
			{
				vertex_idx[i] = i;
				idces[i] = i;
			}

			VertexBufs[VertexBuf_PolyVertex] = MakeVertexBuf(sizeof(vertex_idx), vertex_idx, sizeof(int), sizeof(idces), idces);
		}
		{
			ELayoutType layout_types[2] =
			{
				LayoutType_Int1,
			};

			const Char* layout_semantics[2] =
			{
				L"K_IDX",
			};

			{
				size_t size;
				const U8* bin = GetPolyVsBin(&size);
				ShaderBufs[ShaderBuf_PolyVs] = MakeShaderBuf(ShaderKind_Vs, size, bin, sizeof(float) * 4 * 64 + sizeof(float) * 4 * 64, 1, layout_types, layout_semantics);
			}
			{
				size_t size;
				const U8* bin = GetPolyPsBin(&size);
				ShaderBufs[ShaderBuf_PolyPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, 0, 0, NULL, NULL);
			}
		}
	}

	// Initialize the toon ramp texture.
	{
		void* img = NULL;
		Bool success = False;
		for (; ; )
		{
			size_t size;
			int width;
			int height;
			const U8* bin = GetToonRampPngBin(&size);
			img = DecodePng(size, bin, &width, &height);
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				img = Draw::AdjustTexSize(static_cast<U8*>(img), &width, &height);
			if (!MakeTexWithImg(&TexToonRamp, &ViewToonRamp, NULL, width, height, img, 4, DXGI_FORMAT_R8G8B8A8_UNORM, D3D10_USAGE_IMMUTABLE, 0, False))
				break;
			success = True;
			break;
		}
		if (img != NULL)
			FreeMem(img);
		if (!success)
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	for (int i = 0; i < TexEvenNum; i++)
	{
		float img[4];
		switch (i)
		{
			case 0:
				img[0] = 0.6f; // Diffuse red.
				img[1] = 0.6f; // Diffuse green.
				img[2] = 0.6f; // Diffuse blue.
				img[3] = 1.0f; // Alpha.
				break;
			case 1:
				img[0] = 0.7f; // Metallic F(0) red.
				img[1] = 0.7f; // Metallic F(0) green.
				img[2] = 0.7f; // Metallic F(0) blue.
				img[3] = 3.0f; // Glossiness = 2.0 / (Roughness ^ 4) - 2.0
				break;
			case 2:
				img[0] = 0.5f; // Normal x.
				img[1] = 0.5f; // Normal y.
				img[2] = 1.0f; // Normal z.
				img[3] = 0.0f; // Not used.
				break;
			default:
				ASSERT(False);
				break;
		}
		if (!MakeTexWithImg(&TexEven[i], &ViewEven[i], NULL, 1, 1, img, sizeof(img), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D10_USAGE_IMMUTABLE, 0, False))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	memset(&ObjVsConstBuf, 0, sizeof(SObjVsConstBuf));
	memset(&ObjPsConstBuf, 0, sizeof(SObjPsConstBuf));
	ObjPsConstBuf.CommonParam.AmbTopColor[3] = 0.0f;
	ObjPsConstBuf.CommonParam.AmbBottomColor[3] = 0.0f;
	ObjVsConstBuf.CommonParam.Dir[3] = 0.0f;
	ObjPsConstBuf.CommonParam.DirColor[3] = 0.0f;
	_camera(0.0, 5.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	_proj(M_PI / 180.0 * 27.0, 16.0, 9.0, 0.1, 1000.0); // The angle of view of a 50mm lens is 27 degrees.
	_ambLight(0.2, 0.2, 0.32, 0.32, 0.2, 0.2);
	_dirLight(1.0, -1.0, -1.0, 1.0, 1.0, 1.0);

	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Device->RSSetState(RasterizerState);
	_depth(False, False);
	_blend(1);
	_sampler(1);
}

void Fin()
{
	for (int i = 0; i < TexEvenNum; i++)
	{
		if (ViewEven[i] != NULL)
			ViewEven[i]->Release();
		if (TexEven[i] != NULL)
			TexEven[i]->Release();
	}
	if (ViewToonRamp != NULL)
		ViewToonRamp->Release();
	if (TexToonRamp != NULL)
		TexToonRamp->Release();
	for (int i = 0; i < VertexBuf_Num; i++)
		FinVertexBuf(VertexBufs[i]);
	for (int i = 0; i < ShaderBuf_Num; i++)
		FinShaderBuf(ShaderBufs[i]);
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
	if (RasterizerStateNone != NULL)
		RasterizerStateNone->Release();
	if (RasterizerStateInverted != NULL)
		RasterizerStateInverted->Release();
	if (RasterizerState != NULL)
		RasterizerState->Release();
	if (Device != NULL)
		Device->Release();
}

void* MakeDrawBuf(int tex_width, int tex_height, int split, HWND wnd, void* old, Bool editable)
{
	SWndBuf* old2 = static_cast<SWndBuf*>(old);
	FLOAT clear_color[4];
	if (old == NULL)
	{
		clear_color[0] = 0.0f;
		clear_color[1] = 0.0f;
		clear_color[2] = 0.0f;
		clear_color[3] = 1.0f;
	}
	else
	{
		memcpy(clear_color, old2->ClearColor, sizeof(FLOAT) * 4);
		Draw::FinDrawBuf(old2);
	}
	SWndBuf* wnd_buf = static_cast<SWndBuf*>(AllocMem(sizeof(SWndBuf)));
	memset(wnd_buf, 0, sizeof(SWndBuf));
	memcpy(wnd_buf->ClearColor, clear_color, sizeof(FLOAT) * 4);
	wnd_buf->TexWidth = tex_width;
	wnd_buf->TexHeight = tex_height;
	wnd_buf->AutoClear = True;
	wnd_buf->Editable = editable;
	wnd_buf->Split = split;

	// Create a swap chain.
	{
		IDXGIFactory* factory = NULL;
		DXGI_SWAP_CHAIN_DESC desc;
		Bool success = False;
		for (; ; )
		{
			if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
				break;
			desc.BufferDesc.Width = static_cast<UINT>(tex_width);
			desc.BufferDesc.Height = static_cast<UINT>(tex_height);
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
			THROW(EXCPT_DEVICE_INIT_FAILED);
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
				desc.Width = static_cast<UINT>(tex_width);
				desc.Height = static_cast<UINT>(tex_height);
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
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}

	// Create a temporary texture.
	{
		{
			D3D10_TEXTURE2D_DESC desc;
			desc.Width = static_cast<UINT>(tex_width / split);
			desc.Height = static_cast<UINT>(tex_height / split);
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D10_USAGE_DEFAULT;
			desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			if (FAILED(Device->CreateTexture2D(&desc, NULL, &wnd_buf->TmpTex)))
				THROW(EXCPT_DEVICE_INIT_FAILED);
		}
		{
			D3D10_SHADER_RESOURCE_VIEW_DESC desc;
			memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = 0;
			desc.Texture2D.MipLevels = 1;
			if (FAILED(Device->CreateShaderResourceView(wnd_buf->TmpTex, &desc, &wnd_buf->TmpShaderResView)))
				THROW(EXCPT_DEVICE_INIT_FAILED);
		}
		if (FAILED(Device->CreateRenderTargetView(wnd_buf->TmpTex, NULL, &wnd_buf->TmpRenderTargetView)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}
	if (editable)
	{
		D3D10_TEXTURE2D_DESC desc;
		desc.Width = static_cast<UINT>(tex_width);
		desc.Height = static_cast<UINT>(tex_height);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateTexture2D(&desc, NULL, &wnd_buf->EditableTex)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}
	else
		wnd_buf->EditableTex = NULL;

	if (Callback2d != NULL)
	{
		IDXGISurface* surface = NULL;
		if (FAILED(wnd_buf->TmpTex->QueryInterface(&surface)))
			THROW(EXCPT_DEVICE_INIT_FAILED);
		Callback2d(0, wnd_buf, surface);
	}

	ActiveDrawBuf(wnd_buf);
	Draw::ResetViewport();
	return wnd_buf;
}

void FinDrawBuf(void* wnd_buf)
{
	if (CurWndBuf == wnd_buf)
		CurWndBuf = NULL;
	SWndBuf* wnd_buf2 = static_cast<SWndBuf*>(wnd_buf);
	if (Callback2d != NULL)
		Callback2d(1, wnd_buf, NULL);
	if (wnd_buf2->TmpRenderTargetView != NULL)
		wnd_buf2->TmpRenderTargetView->Release();
	if (wnd_buf2->TmpShaderResView != NULL)
		wnd_buf2->TmpShaderResView->Release();
	if (wnd_buf2->TmpTex != NULL)
		wnd_buf2->TmpTex->Release();
	if (wnd_buf2->DepthView != NULL)
		wnd_buf2->DepthView->Release();
	if (wnd_buf2->RenderTargetView != NULL)
		wnd_buf2->RenderTargetView->Release();
	if (wnd_buf2->SwapChain != NULL)
		wnd_buf2->SwapChain->Release();
	FreeMem(wnd_buf);
}

void ActiveDrawBuf(void* wnd_buf)
{
	if (CurWndBuf != wnd_buf)
	{
		SWndBuf* wnd_buf2 = static_cast<SWndBuf*>(wnd_buf);
		CurWndBuf = wnd_buf2;
		Device->OMSetRenderTargets(1, &CurWndBuf->TmpRenderTargetView, CurWndBuf->DepthView);
		Draw::ResetViewport();

		if (Callback2d != NULL)
			Callback2d(2, wnd_buf, NULL);
	}
}

void Viewport(double x, double y, double w, double h)
{
	D3D10_VIEWPORT viewport =
	{
		static_cast<INT>(x),
		static_cast<INT>(y),
		static_cast<UINT>(w),
		static_cast<UINT>(h),
		0.0f,
		1.0f,
	};
	Device->RSSetViewports(1, &viewport);
}

void ResetViewport()
{
	D3D10_VIEWPORT viewport =
	{
		0,
		0,
		static_cast<UINT>(CurWndBuf->TexWidth / CurWndBuf->Split),
		static_cast<UINT>(CurWndBuf->TexHeight / CurWndBuf->Split),
		0.0f,
		1.0f,
	};
	Device->RSSetViewports(1, &viewport);
}

void* MakeShaderBuf(EShaderKind kind, size_t size, const void* bin, size_t const_buf_size, int layout_num, const ELayoutType* layout_types, const Char** layout_semantics)
{
	SShaderBuf* shader_buf = static_cast<SShaderBuf*>(AllocMem(sizeof(SShaderBuf)));
	ASSERT(const_buf_size % 16 == 0);
	switch (kind)
	{
		case ShaderKind_Vs:
			if (FAILED(Device->CreateVertexShader(bin, size, reinterpret_cast<ID3D10VertexShader**>(&shader_buf->Shader))))
				return NULL;
			break;
		case ShaderKind_Gs:
			if (FAILED(Device->CreateGeometryShader(bin, size, reinterpret_cast<ID3D10GeometryShader**>(&shader_buf->Shader))))
				return NULL;
			break;
		case ShaderKind_Ps:
			if (FAILED(Device->CreatePixelShader(bin, size, reinterpret_cast<ID3D10PixelShader**>(&shader_buf->Shader))))
				return NULL;
			break;
		default:
			ASSERT(False);
			break;
	}
	shader_buf->Kind = kind;
	shader_buf->ConstBufSize = const_buf_size;

	if (const_buf_size == 0)
		shader_buf->ConstBuf = NULL;
	else
	{
		D3D10_BUFFER_DESC desc;
		desc.ByteWidth = static_cast<UINT>(const_buf_size);
		desc.Usage = D3D10_USAGE_DYNAMIC;
		desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateBuffer(&desc, NULL, &shader_buf->ConstBuf)))
			return NULL;
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
				DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
				size_t size2;
				switch (layout_types[i])
				{
					case LayoutType_Int1:
						fmt = DXGI_FORMAT_R32_SINT;
						size2 = sizeof(int);
						break;
					case LayoutType_Int2:
						fmt = DXGI_FORMAT_R32G32_SINT;
						size2 = sizeof(int) * 2;
						break;
					case LayoutType_Int4:
						fmt = DXGI_FORMAT_R32G32B32A32_SINT;
						size2 = sizeof(int) * 4;
						break;
					case LayoutType_Float1:
						fmt = DXGI_FORMAT_R32_FLOAT;
						size2 = sizeof(float);
						break;
					case LayoutType_Float2:
						fmt = DXGI_FORMAT_R32G32_FLOAT;
						size2 = sizeof(float) * 2;
						break;
					case LayoutType_Float3:
						fmt = DXGI_FORMAT_R32G32B32_FLOAT;
						size2 = sizeof(float) * 3;
						break;
					case LayoutType_Float4:
						fmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
						size2 = sizeof(float) * 4;
						break;
					default:
						ASSERT(False);
						size2 = 0;
						break;
				}
				descs[i].Format = fmt;
				descs[i].InputSlot = 0;
				descs[i].AlignedByteOffset = static_cast<UINT>(offset);
				descs[i].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
				descs[i].InstanceDataStepRate = 0;
				offset += size2;
			}
		}
		if (FAILED(Device->CreateInputLayout(descs, static_cast<UINT>(layout_num), bin, size, &shader_buf->Layout)))
			return NULL;
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
	if (data != NULL)
	{
		void* buf;
		if (shader_buf2->ConstBuf->Map(D3D10_MAP_WRITE_DISCARD, 0, &buf))
			return;
		memcpy(buf, data, shader_buf2->ConstBufSize);
		shader_buf2->ConstBuf->Unmap();
	}

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
			return NULL;
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
			return NULL;
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

void Identity(double mat[4][4])
{
	mat[0][0] = 1.0;
	mat[0][1] = 0.0;
	mat[0][2] = 0.0;
	mat[0][3] = 0.0;
	mat[1][0] = 0.0;
	mat[1][1] = 1.0;
	mat[1][2] = 0.0;
	mat[1][3] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	mat[2][3] = 0.0;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
}

void IdentityFloat(float mat[4][4])
{
	mat[0][0] = 1.0f;
	mat[0][1] = 0.0f;
	mat[0][2] = 0.0f;
	mat[0][3] = 0.0f;
	mat[1][0] = 0.0f;
	mat[1][1] = 1.0f;
	mat[1][2] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][0] = 0.0f;
	mat[2][1] = 0.0f;
	mat[2][2] = 1.0f;
	mat[2][3] = 0.0f;
	mat[3][0] = 0.0f;
	mat[3][1] = 0.0f;
	mat[3][2] = 0.0f;
	mat[3][3] = 1.0f;
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

void MulMat(double out[4][4], const double a[4][4], const double b[4][4])
{
	out[0][0] = a[0][0] * b[0][0] + a[1][0] * b[0][1] + a[2][0] * b[0][2] + a[3][0] * b[0][3];
	out[0][1] = a[0][1] * b[0][0] + a[1][1] * b[0][1] + a[2][1] * b[0][2] + a[3][1] * b[0][3];
	out[0][2] = a[0][2] * b[0][0] + a[1][2] * b[0][1] + a[2][2] * b[0][2] + a[3][2] * b[0][3];
	out[0][3] = a[0][3] * b[0][0] + a[1][3] * b[0][1] + a[2][3] * b[0][2] + a[3][3] * b[0][3];
	out[1][0] = a[0][0] * b[1][0] + a[1][0] * b[1][1] + a[2][0] * b[1][2] + a[3][0] * b[1][3];
	out[1][1] = a[0][1] * b[1][0] + a[1][1] * b[1][1] + a[2][1] * b[1][2] + a[3][1] * b[1][3];
	out[1][2] = a[0][2] * b[1][0] + a[1][2] * b[1][1] + a[2][2] * b[1][2] + a[3][2] * b[1][3];
	out[1][3] = a[0][3] * b[1][0] + a[1][3] * b[1][1] + a[2][3] * b[1][2] + a[3][3] * b[1][3];
	out[2][0] = a[0][0] * b[2][0] + a[1][0] * b[2][1] + a[2][0] * b[2][2] + a[3][0] * b[2][3];
	out[2][1] = a[0][1] * b[2][0] + a[1][1] * b[2][1] + a[2][1] * b[2][2] + a[3][1] * b[2][3];
	out[2][2] = a[0][2] * b[2][0] + a[1][2] * b[2][1] + a[2][2] * b[2][2] + a[3][2] * b[2][3];
	out[2][3] = a[0][3] * b[2][0] + a[1][3] * b[2][1] + a[2][3] * b[2][2] + a[3][3] * b[2][3];
	out[3][0] = a[0][0] * b[3][0] + a[1][0] * b[3][1] + a[2][0] * b[3][2] + a[3][0] * b[3][3];
	out[3][1] = a[0][1] * b[3][0] + a[1][1] * b[3][1] + a[2][1] * b[3][2] + a[3][1] * b[3][3];
	out[3][2] = a[0][2] * b[3][0] + a[1][2] * b[3][1] + a[2][2] * b[3][2] + a[3][2] * b[3][3];
	out[3][3] = a[0][3] * b[3][0] + a[1][3] * b[3][1] + a[2][3] * b[3][2] + a[3][3] * b[3][3];
}

void SetProjViewMat(float out[4][4], const double proj[4][4], const double view[4][4])
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

HFONT ToFontHandle(SClass* font)
{
	return reinterpret_cast<SFont*>(font)->Font;
}

void ColorToArgb(double* a, double* r, double* g, double* b, S64 color)
{
	THROWDBG(color < 0 || 0xffffffff < color, EXCPT_DBG_ARG_OUT_DOMAIN);
	*a = static_cast<double>((color >> 24) & 0xff) / 255.0;
	*r = Gamma(static_cast<double>((color >> 16) & 0xff) / 255.0);
	*g = Gamma(static_cast<double>((color >> 8) & 0xff) / 255.0);
	*b = Gamma(static_cast<double>(color & 0xff) / 255.0);
}

S64 ArgbToColor(double a, double r, double g, double b)
{
	if (a < 0.0)
		a = 0.0;
	else if (a > 1.0)
		a = 1.0;
	if (r < 0.0)
		r = 0.0;
	else if (r > 1.0)
		r = 1.0;
	if (g < 0.0)
		g = 0.0;
	else if (g > 1.0)
		g = 1.0;
	if (b < 0.0)
		b = 0.0;
	else if (b > 1.0)
		b = 1.0;
	return (static_cast<S64>(a * 255.0 + 0.5) << 24) |
		(static_cast<S64>(Degamma(r) * 255.0 + 0.5) << 16) |
		(static_cast<S64>(Degamma(g) * 255.0 + 0.5) << 8) |
		static_cast<S64>(Degamma(b) * 255.0 + 0.5);
}

double Gamma(double value)
{
	return value * (value * (value * 0.305306011 + 0.682171111) + 0.012522878);
}

double Degamma(double value)
{
	value = 1.055 * pow(value, 0.416666667) - 0.055;
	if (value < 0.0)
		value = 0.0;
	return value;
}

U8* AdjustTexSize(U8* argb, int* width, int* height)
{
	int width2 = 1;
	int height2 = 1;
	while (width2 < *width)
		width2 *= 2;
	while (height2 < *height)
		height2 *= 2;

	U8* rgba2 = static_cast<U8*>(AllocMem(static_cast<size_t>(width2 * height2 * 4)));
	memset(rgba2, 0, static_cast<size_t>(width2 * height2 * 4));
	for (int i = 0; i < *height; i++)
		memcpy(rgba2 + width2 * 4 * i, argb + *width * 4 * i, *width * 4);

	*width = width2;
	*height = height2;
	FreeMem(argb);
	return rgba2;
}

void SetJointMat(const void* element, double frame, float (*joint)[4][4])
{
	const SObj::SPolygon* element2 = static_cast<const SObj::SPolygon*>(element);
	for (int i = 0; i < element2->JointNum; i++)
	{
		int offset = i * (element2->End - element2->Begin + 1);
		int mat_a = static_cast<int>(frame);
		int mat_b = mat_a == element2->End ? mat_a : mat_a + 1;
		float rate_b = static_cast<float>(frame - static_cast<double>(static_cast<int>(frame)));
		float rate_a = 1.0f - rate_b;
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
				joint[i][j][k] = rate_a * element2->Joints[offset + mat_a][j][k] + rate_b * element2->Joints[offset + mat_b][j][k];
		}
	}
}

SClass* MakeTexImpl(SClass* me_, const U8* path, Bool as_argb)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	S64 path_len = *reinterpret_cast<const S64*>(path + 0x08);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	STex* me2 = reinterpret_cast<STex*>(me_);
	void* bin = NULL;
	void* img = NULL;
	Bool img_ref = False; // Set to true when 'img' should not be released.
	DXGI_FORMAT fmt;
	int width;
	int height;
	{
		size_t size;
		bin = LoadFileAll(path2, &size);
		if (bin == NULL)
		{
			THROW(EXCPT_FILE_READ_FAILED);
			return NULL;
		}
		THROWDBG(path_len < 4, EXCPT_DBG_ARG_OUT_DOMAIN);
		if (StrCmpIgnoreCase(path2 + path_len - 4, L".png"))
		{
			img = DecodePng(size, bin, &width, &height);
			me2->ImgWidth = width;
			me2->ImgHeight = height;
			fmt = as_argb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				img = Draw::AdjustTexSize(static_cast<U8*>(img), &width, &height);
		}
		else if (StrCmpIgnoreCase(path2 + path_len - 4, L".jpg"))
		{
			img = DecodeJpg(size, bin, &width, &height);
			me2->ImgWidth = width;
			me2->ImgHeight = height;
			fmt = as_argb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				img = Draw::AdjustTexSize(static_cast<U8*>(img), &width, &height);
		}
		else if (StrCmpIgnoreCase(path2 + path_len - 4, L".dds"))
		{
			img = DecodeBc(size, bin, &width, &height);
			me2->ImgWidth = width;
			me2->ImgHeight = height;
			img_ref = True;
			fmt = as_argb ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC3_UNORM_SRGB;
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				THROW(EXCPT_INVALID_DATA_FMT);
		}
		else
		{
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
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
			if (!MakeTexWithImg(&me2->Tex, &me2->View, NULL, me2->Width, me2->Height, img, me2->Width * 4, fmt, D3D10_USAGE_IMMUTABLE, 0, False))
				break;
			success = True;
			break;
		}
		if (img != NULL && !img_ref)
			FreeMem(img);
		if (bin != NULL)
			FreeMem(bin);
		if (!success)
			THROW(EXCPT_DEVICE_INIT_FAILED);
	}
	return me_;
}

void Clear()
{
	Device->ClearRenderTargetView(CurWndBuf->TmpRenderTargetView, CurWndBuf->ClearColor);
	Device->ClearDepthStencilView(CurWndBuf->DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
}

} // namespace Draw
