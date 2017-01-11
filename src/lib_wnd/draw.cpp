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
};

static ID3D10Device* Device = NULL;
static ID3D10RasterizerState* RasterizerState = NULL;
static ID3D10DepthStencilState* DepthState[DepthNum] = { NULL };
static ID3D10BlendState* BlendState[BlendNum] = { NULL };
static ID3D10SamplerState* Sampler = NULL;

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

void InitDraw()
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

	// Initialize 'Rect'.
	{

		RectVertexIdx = static_cast<CVertexIdx*>(CMem::Alloc(sizeof(CVertexIdx)));
		{
			float vertex[] =
			{
				0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
				1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
				1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			};

			u16 idx[] =
			{
				0, 1, 2,
				2, 3, 0,
			};

			RectVertexIdx->CVertexIdx::CVertexIdx(sizeof(vertex), vertex, sizeof(float) * 5, sizeof(idx), idx);
		}

		RectVS = static_cast<CShader*>(CMem::Alloc(sizeof(CShader)));
		{
			CShader::SLayout layouts[2] =
			{
				{ L"POSITION", CShader::SLayout::Type_Float3 },
				{ L"TEXCOORD", CShader::SLayout::Type_Float2 },
			};
			RectVS->CShader::CShader(CShader::Kind_VS, L"kuin/rect.vs", sizeof(float) * 4 * 2, sizeof(layouts) / sizeof(layouts[0]), layouts);
		}
		RectPS = static_cast<CShader*>(CMem::Alloc(sizeof(CShader)));
		RectPS->CShader::CShader(CShader::Kind_PS, L"kuin/rect.ps", sizeof(float) * 4);
	}

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

	ResetViewport();
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	DeviceContext->RSSetState(RasterizerState);
	DeviceContext->PSSetSamplers(0, 1, &Sampler);
	DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	SetZBuf(false, false);
	SetBlend(Blend_None);
	*/
}

void FinDraw()
{
	/*
	// TODO:
	if (RectVertexIdx != NULL)
	{
		RectVertexIdx->~CVertexIdx();
		CMem::Free(RectVertexIdx);
	}
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
	if (RectPS != NULL)
	{
		RectPS->~CShader();
		CMem::Free(RectPS);
	}
	if (RectVS != NULL)
	{
		RectVS->~CShader();
		CMem::Free(RectVS);
	}
	*/
	if (Sampler != NULL)
	{
		Sampler->Release();
		Sampler = NULL;
	}
	for (int i = 0; i < BlendNum; i++)
	{
		if (BlendState[i] != NULL)
		{
			BlendState[i]->Release();
			BlendState[i] = NULL;
		}
	}
	for (int i = 0; i < DepthNum; i++)
	{
		if (DepthState[i] != NULL)
		{
			DepthState[i]->Release();
			DepthState[i] = NULL;
		}
	}
	if (RasterizerState != NULL)
	{
		RasterizerState->Release();
		RasterizerState = NULL;
	}
	if (Device != NULL)
	{
		Device->Release();
		Device = NULL;
	}
}

void* MakeWndBuf(int width, int height, HWND wnd)
{
	SWndBuf* wnd_buf = static_cast<SWndBuf*>(AllocMem(sizeof(SWndBuf)));
	memset(wnd_buf, 0, sizeof(SWndBuf));

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
