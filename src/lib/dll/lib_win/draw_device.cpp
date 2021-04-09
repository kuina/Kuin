#include "draw_device.h"

#include "png_decoder.h"

#pragma comment(lib, "d3d10_1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "winmm.lib")

static const FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

static S64 Cnt;
static U32 PrevTime;
static double ViewMat[4][4];
static double ProjMat[4][4];
static int FilterIdx = 0;
static float FilterParam[4][4];

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
const U8* GetParticle2dVsBin(size_t* size);
const U8* GetParticle2dPsBin(size_t* size);
const U8* GetParticleUpdatingVsBin(size_t* size);
const U8* GetParticleUpdatingPsBin(size_t* size);
const U8* GetToonRampPngBin(size_t* size);

static void WriteBack();
static void Clear();

EXPORT_CPP void _drawInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

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
				if (SUCCEEDED(D3D10CreateDevice1(nullptr, type[i], nullptr, D3D10_CREATE_DEVICE_BGRA_SUPPORT, level[j], D3D10_1_SDK_VERSION, &Device)))
				{
					success = True;
					break;
				}
			}
			if (success)
				break;
		}
		if (!success)
			THROW(0xe9170009);
	}

	Cnt = 0;
	CurZBuf = -1;
	CurBlend = -1;
	CurSampler = -1;
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
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerStates[RasterizerState_Normal])))
			THROW(0xe9170009);
		desc.CullMode = D3D10_CULL_BACK;
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerStates[RasterizerState_Inverted])))
			THROW(0xe9170009);
		desc.CullMode = D3D10_CULL_NONE;
		if (FAILED(Device->CreateRasterizerState(&desc, &RasterizerStates[RasterizerState_None])))
			THROW(0xe9170009);
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
			THROW(0xe9170009);
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
			THROW(0xe9170009);
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
			THROW(0xe9170009);
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
				ShaderBufs[ShaderBuf_TriPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_CirclePs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 8, 0, nullptr, nullptr);
			}
		}

		// Initialize 'CircleLine'.
		{
			size_t size;
			const U8* bin = GetCircleLinePsBin(&size);
			ShaderBufs[ShaderBuf_CircleLinePs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 8, 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_TexPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetFontPsBin(&size);
				ShaderBufs[ShaderBuf_FontPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_ObjPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetObjSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetObjToonPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetObjToonSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjPsConstBuf), 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_ObjFastPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetObjFastSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjFastSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetObjToonFastPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonFastPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, nullptr, nullptr);
			}
			{
				size_t size;
				const U8* bin = GetObjToonFastSmPsBin(&size);
				ShaderBufs[ShaderBuf_ObjToonFastSmPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_ObjFlatPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjFastPsConstBuf), 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_ObjOutlinePs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SObjOutlinePsConstBuf), 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_Filter0Ps] = MakeShaderBuf(ShaderKind_Ps, size, bin, 0, 0, nullptr, nullptr);
			}
			if (IsResUsed(UseResFlagsKind_Draw_FilterMonotone))
			{
				size_t size;
				const U8* bin = GetFilterMonotonePsBin(&size);
				ShaderBufs[ShaderBuf_Filter1Ps] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(float) * 4, 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_Particle2dPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SParticlePsConstBuf), 0, nullptr, nullptr);
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
				ShaderBufs[ShaderBuf_ParticleUpdatingPs] = MakeShaderBuf(ShaderKind_Ps, size, bin, sizeof(SParticleUpdatingPsConstBuf), 0, nullptr, nullptr);
			}
		}
	}

	// Initialize the toon ramp texture.
	{
		void* img = nullptr;
		Bool success = False;
		for (; ; )
		{
			size_t size;
			int width;
			int height;
			const U8* bin = GetToonRampPngBin(&size);
			img = DecodePng(size, bin, &width, &height);
			if (!IsPowerOf2(static_cast<U64>(width)) || !IsPowerOf2(static_cast<U64>(height)))
				img = AdjustTexSize(static_cast<U8*>(img), &width, &height);
			if (!MakeTexWithImg(&TexToonRamp, &ViewToonRamp, nullptr, width, height, img, 4, DXGI_FORMAT_R8G8B8A8_UNORM, D3D10_USAGE_IMMUTABLE, 0, False))
				break;
			success = True;
			break;
		}
		if (img != nullptr)
			FreeMem(img);
		if (!success)
			THROW(0xe9170009);
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
		if (!MakeTexWithImg(&TexEven[i], &ViewEven[i], nullptr, 1, 1, img, sizeof(img), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D10_USAGE_IMMUTABLE, 0, False))
			THROW(0xe9170009);
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
	Device->RSSetState(RasterizerStates[RasterizerState_Normal]);
	_depth(False, False);
	_blend(1);
	_sampler(1);
}

EXPORT_CPP void _drawFin()
{
	for (int i = 0; i < TexEvenNum; i++)
	{
		if (ViewEven[i] != nullptr)
			ViewEven[i]->Release();
		if (TexEven[i] != nullptr)
			TexEven[i]->Release();
	}
	if (ViewToonRamp != nullptr)
		ViewToonRamp->Release();
	if (TexToonRamp != nullptr)
		TexToonRamp->Release();
	for (int i = 0; i < VertexBuf_Num; i++)
	{
		if (VertexBufs[i] != nullptr)
			FinVertexBuf(VertexBufs[i]);
	}
	for (int i = 0; i < ShaderBuf_Num; i++)
	{
		if (ShaderBufs[i] != nullptr)
			FinShaderBuf(ShaderBufs[i]);
	}
	for (int i = 0; i < SamplerNum; i++)
	{
		if (Sampler[i] != nullptr)
			Sampler[i]->Release();
	}
	for (int i = 0; i < BlendNum; i++)
	{
		if (BlendState[i] != nullptr)
			BlendState[i]->Release();
	}
	for (int i = 0; i < DepthNum; i++)
	{
		if (DepthState[i] != nullptr)
			DepthState[i]->Release();
	}
	for (int i = 0; i < RasterizerState_Num; i++)
	{
		if (RasterizerStates[i] != nullptr)
			RasterizerStates[i]->Release();
	}
	if (Device != nullptr)
		Device->Release();
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

EXPORT_CPP S64 _argbToColor(double a, double r, double g, double b)
{
	return ArgbToColor(a, r, g, b);
}

EXPORT_CPP void _autoClear(Bool enabled)
{
	CurWndBuf->AutoClear = enabled;
}

EXPORT_CPP void _blend(S64 kind)
{
	THROWDBG(kind < 0 || BlendNum <= kind, 0xe9170006);
	int kind2 = static_cast<int>(kind);
	if (CurBlend == kind2)
		return;
	Device->OMSetBlendState(BlendState[kind2], BlendFactor, 0xffffffff);
	CurBlend = kind2;
}

EXPORT_CPP void _camera(double eyeX, double eyeY, double eyeZ, double atX, double atY, double atZ, double upX, double upY, double upZ)
{
	double look[3], up[3], right[3], eye[3], pxyz[3], eye_len;

	look[0] = atX - eyeX;
	look[1] = atY - eyeY;
	look[2] = atZ - eyeZ;
	eye_len = Normalize(look);
	if (eye_len == 0.0)
		return;

	up[0] = upX;
	up[1] = upY;
	up[2] = upZ;
	Cross(right, up, look);
	if (Normalize(right) == 0.0)
		return;

	Cross(up, look, right);

	eye[0] = eyeX;
	eye[1] = eyeY;
	eye[2] = eyeZ;
	pxyz[0] = Dot(eye, right);
	pxyz[1] = Dot(eye, up);
	pxyz[2] = Dot(eye, look);

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

	SetProjViewMat(ObjVsConstBuf.CommonParam.ProjView, ProjMat, ViewMat);
}

EXPORT_CPP Bool _capture(const U8* path)
{
	THROWDBG(path == nullptr, EXCPT_ACCESS_VIOLATION);
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
		if (FAILED(Device->CreateTexture2D(&desc, nullptr, &tex)))
			return False;
	}
	Bool result = False;
	Device->CopyResource(tex, CurWndBuf->TmpTex);
	{
		FILE* file_ptr = _wfopen(reinterpret_cast<const Char*>(path + 0x10), L"wb");
		if (file_ptr != nullptr)
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
						buf[0] = static_cast<U8>(max(min(static_cast<int>(Degamma(static_cast<double>(dst[(j * CurWndBuf->TexWidth + i) * 4 + 0]) / 255.0) * 255.0), 255), 0));
						buf[1] = static_cast<U8>(max(min(static_cast<int>(Degamma(static_cast<double>(dst[(j * CurWndBuf->TexWidth + i) * 4 + 1]) / 255.0) * 255.0), 255), 0));
						buf[2] = static_cast<U8>(max(min(static_cast<int>(Degamma(static_cast<double>(dst[(j * CurWndBuf->TexWidth + i) * 4 + 2]) / 255.0) * 255.0), 255), 0));
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

EXPORT_CPP void _clear()
{
	WriteBack();
	Clear();
}

EXPORT_CPP void _clearColor(S64 color)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
	CurWndBuf->ClearColor[0] = static_cast<FLOAT>(r);
	CurWndBuf->ClearColor[1] = static_cast<FLOAT>(g);
	CurWndBuf->ClearColor[2] = static_cast<FLOAT>(b);
}

EXPORT_CPP S64 _cnt()
{
	return Cnt;
}

EXPORT_CPP void _colorToArgb(double* a, double* r, double* g, double* b, S64 color)
{
	ColorToArgb(a, r, g, b, color);
}

EXPORT_CPP void _depth(Bool test, Bool write)
{
	int kind = (static_cast<int>(test) << 1) | static_cast<int>(write);
	if (CurZBuf == kind)
		return;
	Device->OMSetDepthStencilState(DepthState[kind], 0);
	CurZBuf = kind;
}

EXPORT_CPP void _dirLight(double atX, double atY, double atZ, double r, double g, double b)
{
	double dir[3] = { atX, atY, atZ };
	Normalize(dir);
	ObjVsConstBuf.CommonParam.Dir[0] = -static_cast<float>(dir[0]);
	ObjVsConstBuf.CommonParam.Dir[1] = -static_cast<float>(dir[1]);
	ObjVsConstBuf.CommonParam.Dir[2] = -static_cast<float>(dir[2]);
	ObjPsConstBuf.CommonParam.DirColor[0] = static_cast<float>(r);
	ObjPsConstBuf.CommonParam.DirColor[1] = static_cast<float>(g);
	ObjPsConstBuf.CommonParam.DirColor[2] = static_cast<float>(b);
}

EXPORT_CPP void _editPixels(const void* callback)
{
	THROWDBG(callback == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(!CurWndBuf->Editable, 0xe917000a);
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

EXPORT_CPP void _filterMonotone(S64 color, double rate)
{
	double r, g, b, a;
	ColorToArgb(&a, &r, &g, &b, color);
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

EXPORT_CPP void _filterNone()
{
	FilterIdx = 0;
}

EXPORT_CPP void _proj(double fovy, double aspectX, double aspectY, double nearZ, double farZ)
{
	THROWDBG(fovy <= 0.0 || M_PI / 2.0 <= fovy || aspectX <= 0.0 || aspectY <= 0.0 || nearZ <= 0.0 || farZ <= nearZ, 0xe9170006);
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

	SetProjViewMat(ObjVsConstBuf.CommonParam.ProjView, ProjMat, ViewMat);
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

		Device->OMSetRenderTargets(1, &CurWndBuf->RenderTargetView, nullptr);
		{
			ConstBuf(ShaderBufs[ShaderBuf_FilterVs], nullptr);
			Device->GSSetShader(nullptr);
			ConstBuf(ShaderBufs[ShaderBuf_Filter0Ps + FilterIdx], FilterIdx == 0 ? nullptr : FilterParam);
			VertexBuf(VertexBufs[VertexBuf_FilterVertex]);
			Device->PSSetShaderResources(0, 1, &CurWndBuf->TmpShaderResView);
		}
		Device->DrawIndexed(6, 0, 0);

		_depth((old_z_buf & 2) != 0, (old_z_buf & 1) != 0);
		_blend(old_blend);
		_sampler(old_sampler);

		ResetViewport();
	}

	CurWndBuf->SwapChain->Present(fps == 0 ? 0 : 1, 0);
	Device->OMSetRenderTargets(1, &CurWndBuf->TmpRenderTargetView, CurWndBuf->DepthView);
	if (CurWndBuf->AutoClear)
		Clear();
	Device->RSSetState(RasterizerStates[RasterizerState_Normal]);

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
			THROWDBG(True, 0xe9170006);
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

EXPORT_CPP void _sampler(S64 kind)
{
	THROWDBG(kind < 0 || SamplerNum <= kind, 0xe9170006);
	int kind2 = static_cast<int>(kind);
	if (CurSampler == kind2)
		return;
	Device->PSSetSamplers(0, 1, &Sampler[kind2]);
	CurSampler = kind2;
}

EXPORT_CPP S64 _screenHeight()
{
	return CurWndBuf->TexHeight;
}

EXPORT_CPP S64 _screenWidth()
{
	return CurWndBuf->TexWidth;
}

EXPORT_CPP void _set2dCallback(void* (*callback)(int, void*, void*))
{
	Callback2d = callback;
}

static void WriteBack()
{
	if (Callback2d != nullptr)
		Callback2d(3, nullptr, nullptr);
}

static void Clear()
{
	Device->ClearRenderTargetView(CurWndBuf->TmpRenderTargetView, CurWndBuf->ClearColor);
	Device->ClearDepthStencilView(CurWndBuf->DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
}
