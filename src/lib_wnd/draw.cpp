#include "draw.h"

// DirectX 10 is preinstalled on Windows Vista or later.
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d10.h>

const int DepthNum = 4;
const int BlendNum = 5;

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

static const FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

const U8* GetTriVsBin(size_t* size);
const U8* GetTriPsBin(size_t* size);
const U8* GetRectVsBin(size_t* size);
const U8* GetCircleVsBin(size_t* size);
const U8* GetCirclePsBin(size_t* size);

static ID3D10Device* Device = NULL;
static ID3D10RasterizerState* RasterizerState = NULL;
static ID3D10DepthStencilState* DepthState[DepthNum] = { NULL };
static ID3D10BlendState* BlendState[BlendNum] = { NULL };
static ID3D10SamplerState* Sampler = NULL;
static SWndBuf* CurWndBuf;
static void* TriVertex = NULL;
static void* TriVs = NULL;
static void* TriPs = NULL;
static void* RectVertex = NULL;
static void* RectVs = NULL;
static void* CircleVertex = NULL;
static void* CircleVs = NULL;
static void* CirclePs = NULL;

EXPORT_CPP void _render()
{
	CurWndBuf->SwapChain->Present(0, 0);
	Device->ClearRenderTargetView(CurWndBuf->RenderTargetView, CurWndBuf->ClearColor);
	Device->ClearDepthStencilView(CurWndBuf->DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	Device->RSSetState(RasterizerState);
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
	int depth = (static_cast<int>(test) << 1) | static_cast<int>(write);
	/*
	// TODO:
	if (ZBuf == zbuf)
	return;
	*/
	Device->OMSetDepthStencilState(DepthState[depth], 0);
	// TODO: ZBuf = zbuf;
}

EXPORT_CPP void _blend(S64 blend)
{
	ASSERT(0 <= blend && blend < BlendNum);
	int blend2 = static_cast<int>(blend);
	/*
	// TODO:
	if (Blend == blend2)
	return;
	*/
	Device->OMSetBlendState(BlendState[blend2], BlendFactor, 0xffffffff);
	// TODO: Blend = blend2;
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

/*
EXPORT_CPP void _flip()
{
	// TODO:
}

EXPORT_CPP void _clear()
{
	// TODO:
}

EXPORT_CPP void _viewport(double left, double top, double width, double height)
{
	// TODO:
}

EXPORT_CPP void _resetViewport()
{
	// TODO:
}

EXPORT_CPP void _zBuf(S64 zBuf)
{
	// TODO:
}

EXPORT_CPP void _blend(S64 blend)
{
	// TODO:
}

EXPORT_CPP void _rect(double left, double top, double width, double height, double r, double g, double b, double a)
{
	// TODO:
}

EXPORT_CPP SClass* _makeTex(SClass* me_, const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT_CPP void _texDraw(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH)
{
	// TODO:
}

EXPORT_CPP void _texDrawRot(SClass* me_, double dstX, double dstY, double dstW, double dstH, double srcX, double srcY, double srcW, double srcH, double centerX, double centerY, double angle)
{
	// TODO:
}

EXPORT_CPP double _texWidth(SClass* me_)
{
	// TODO:
	return 0.0;
}

EXPORT_CPP double _texHeight(SClass* me_)
{
	// TODO:
	return 0.0;
}

EXPORT_CPP SClass* _makeFont(SClass* me_, const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT_CPP void _fontDraw(SClass* me_, const U8* str, double x, double y, double r, double g, double b, double a)
{
	// TODO:
}

EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* path)
{
	// TODO:
	return NULL;
}

EXPORT_CPP void _objDraw(SClass* me_, SClass* tex, SClass* normTex, S64 group, double anim)
{
	// TODO:
}

EXPORT_CPP void _objReset(SClass* me_)
{
	// TODO:
}

EXPORT_CPP void _objScale(SClass* me_, double x, double y, double z)
{
	// TODO:
}

EXPORT_CPP void _objRotX(SClass* me_, double angle)
{
	// TODO:
}

EXPORT_CPP void _objRotY(SClass* me_, double angle)
{
	// TODO:
}

EXPORT_CPP void _objRotZ(SClass* me_, double angle)
{
	// TODO:
}

EXPORT_CPP void _objTrans(SClass* me_, double x, double y, double z)
{
	// TODO:
}

EXPORT_CPP void _objSrt(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ)
{
	// TODO:
}

EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ)
{
	// TODO:
}

EXPORT_CPP void _objLookFix(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ)
{
	// TODO:
}

EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ)
{
	// TODO:
}

EXPORT_CPP void _objLookCameraFix(SClass* me_, double x, double y, double z, double upX, double upY, double upZ)
{
	// TODO:
}

EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ)
{
	// TODO:
}

EXPORT_CPP void _proj(double fovy, double minZ, double maxZ)
{
	// TODO:
}

EXPORT_CPP void _ambLight(double topR, double topG, double topB, double bottomR, double bottomG, double bottomB)
{
	// TODO:
}

EXPORT_CPP void _dirLight(double atX, double atY, double atZ, double r, double g, double b)
{
	// TODO:
}
*/

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
		desc.CullMode = D3D10_CULL_BACK;
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
		desc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
		desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
		desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
		desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
		desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
		desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
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
	{
		D3D10_SAMPLER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Filter = D3D10_FILTER_ANISOTROPIC;
		desc.AddressU = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.AddressV = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.AddressW = D3D10_TEXTURE_ADDRESS_MIRROR;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 2;
		desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
		desc.BorderColor[0] = 0.0f;
		desc.BorderColor[1] = 0.0f;
		desc.BorderColor[2] = 0.0f;
		desc.BorderColor[3] = 0.0f;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D10_FLOAT32_MAX;
		if (FAILED(Device->CreateSamplerState(&desc, &Sampler)))
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

			U16 idces[] =
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

			U16 idces[] =
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

			U16 idces[] =
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

	/*
	// Initialize 'Obj'.
	{
		ObjAnimVS = static_cast<CShader*>(CMem::Alloc(sizeof(CShader)));
		{
			CShader::SLayout layouts[7] =
			{
				{ L"POSITION", CShader::SLayout::Type_Float3 },
				{ L"NORMAL", CShader::SLayout::Type_Float3 },
				{ L"TANGENT", CShader::SLayout::Type_Float3 },
				{ L"BINORMAL", CShader::SLayout::Type_Float3 },
				{ L"TEXCOORD", CShader::SLayout::Type_Float2 },
				{ L"JOINT", CShader::SLayout::Type_Int4 },
				{ L"WEIGHT", CShader::SLayout::Type_Float4 },
			};
			ObjAnimVS->CShader::CShader(CShader::Kind_VS, L"kuin/obj_a.vs", sizeof(SObjAnimVSConst), sizeof(layouts) / sizeof(layouts[0]), layouts);
		}
		ObjAnimPS = static_cast<CShader*>(CMem::Alloc(sizeof(CShader)));
		ObjAnimPS->CShader::CShader(CShader::Kind_PS, L"kuin/obj_a.ps", sizeof(SObjAnimPSConst));
	}
#if defined(DBG)
	{
		ObjDbgVS = static_cast<CShader*>(CMem::Alloc(sizeof(CShader)));
		{
			CShader::SLayout layouts[7] =
			{
				{ L"POSITION", CShader::SLayout::Type_Float3 },
				{ L"NORMAL", CShader::SLayout::Type_Float3 },
				{ L"TANGENT", CShader::SLayout::Type_Float3 },
				{ L"BINORMAL", CShader::SLayout::Type_Float3 },
				{ L"TEXCOORD", CShader::SLayout::Type_Float2 },
				{ L"JOINT", CShader::SLayout::Type_Int4 },
				{ L"WEIGHT", CShader::SLayout::Type_Float4 },
			};
			ObjDbgVS->CShader::CShader(CShader::Kind_VS, L"dbg/obj_dbg.vs", sizeof(SObjAnimVSConst), sizeof(layouts) / sizeof(layouts[0]), layouts);
		}
		ObjDbgPS = static_cast<CShader*>(CMem::Alloc(sizeof(CShader)));
		ObjDbgPS->CShader::CShader(CShader::Kind_PS, L"dbg/obj_dbg.ps", sizeof(SObjAnimPSConst));
	}
#endif
	*/

	/*
	// TODO:
	// Set the camera and the projection.
	ObjAnimVSConst.World.Identity();
	ObjAnimVSConst.NWorld.Identity();
	SetCamera(0.0, 0.0, -10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	SetProj(M_PI / 180.0 * 27.0, 16.0, 9.0, 0.01, 1000.0); // The angle of view of a 50mm lens is 27 degrees.
	ObjAnimVSConst.DirPos[0] = -1.0f;
	ObjAnimVSConst.DirPos[1] = 1.0f;
	ObjAnimVSConst.DirPos[2] = -1.0f;
	ObjAnimPSConst.DirCol[0] = 1.0f;
	ObjAnimPSConst.DirCol[1] = 1.0f;
	ObjAnimPSConst.DirCol[2] = 1.0f;
	ObjAnimPSConst.DirCol[3] = 0.0f;
	ObjAnimPSConst.Mode[0] = -1;

	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	*/
	Device->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Device->RSSetState(RasterizerState);
	Device->PSSetSamplers(0, 1, &Sampler);
	_depth(False, False);
	_blend(0);
}

void Fin()
{
	/*
	// TODO:
#if defined(DBG)
	if (ObjDbgPS != NULL)
	{
		ObjDbgPS->~CShader();
		CMem::Free(ObjDbgPS);
	}
	if (ObjDbgVS != NULL)
	{
		ObjDbgVS->~CShader();
		CMem::Free(ObjDbgVS);
	}
#endif
	if (ObjAnimPS != NULL)
	{
		ObjAnimPS->~CShader();
		CMem::Free(ObjAnimPS);
	}
	if (ObjAnimVS != NULL)
	{
		ObjAnimVS->~CShader();
		CMem::Free(ObjAnimVS);
	}
	*/
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
	if (Sampler != NULL)
		Sampler->Release();
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
	Device->IASetIndexBuffer(vertex_buf2->Idx, DXGI_FORMAT_R16_UINT, 0);
}

void* MakeVertexBuf(size_t vertex_size, const void* vertices, size_t vertex_line_size, size_t idx_size, const U16* idces)
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

} // namespace Draw
