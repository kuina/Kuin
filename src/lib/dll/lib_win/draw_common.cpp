#include "draw_common.h"

ID3D10Device1* Device;
ID3D10RasterizerState* RasterizerStates[RasterizerState_Num];
ID3D10DepthStencilState* DepthState[DepthNum];
ID3D10BlendState* BlendState[BlendNum];
ID3D10SamplerState* Sampler[SamplerNum];
SVertexBuf* VertexBufs[VertexBuf_Num];
SShaderBuf* ShaderBufs[ShaderBuf_Num];
SObjVsConstBuf ObjVsConstBuf;
SObjPsConstBuf ObjPsConstBuf;
ID3D10Texture2D* TexToonRamp;
ID3D10ShaderResourceView* ViewToonRamp;
ID3D10Texture2D* TexEven[TexEvenNum];
ID3D10ShaderResourceView* ViewEven[TexEvenNum];
SWndBuf* CurWndBuf;
void* (*Callback2d)(int kind, void* arg1, void* arg2);
int CurZBuf;
int CurBlend;
int CurSampler;

SVertexBuf* MakeVertexBuf(size_t vertex_size, const void* vertices, size_t vertex_line_size, size_t idx_size, const U32* idces)
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
			return nullptr;
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
			return nullptr;
	}

	return vertex_buf;
}

void FinVertexBuf(SVertexBuf* vertex_buf)
{
	if (vertex_buf->Idx != nullptr)
		vertex_buf->Idx->Release();
	if (vertex_buf->Vertex != nullptr)
		vertex_buf->Vertex->Release();
	FreeMem(vertex_buf);
}

SShaderBuf* MakeShaderBuf(EShaderKind kind, size_t size, const void* bin, size_t const_buf_size, int layout_num, const ELayoutType* layout_types, const Char** layout_semantics)
{
	SShaderBuf* shader_buf = static_cast<SShaderBuf*>(AllocMem(sizeof(SShaderBuf)));
	ASSERT(const_buf_size % 16 == 0);
	switch (kind)
	{
		case ShaderKind_Vs:
			if (FAILED(Device->CreateVertexShader(bin, size, reinterpret_cast<ID3D10VertexShader**>(&shader_buf->Shader))))
				return nullptr;
			break;
		case ShaderKind_Gs:
			if (FAILED(Device->CreateGeometryShader(bin, size, reinterpret_cast<ID3D10GeometryShader**>(&shader_buf->Shader))))
				return nullptr;
			break;
		case ShaderKind_Ps:
			if (FAILED(Device->CreatePixelShader(bin, size, reinterpret_cast<ID3D10PixelShader**>(&shader_buf->Shader))))
				return nullptr;
			break;
		default:
			ASSERT(False);
			break;
	}
	shader_buf->Kind = kind;
	shader_buf->ConstBufSize = const_buf_size;

	if (const_buf_size == 0)
		shader_buf->ConstBuf = nullptr;
	else
	{
		D3D10_BUFFER_DESC desc;
		desc.ByteWidth = static_cast<UINT>(const_buf_size);
		desc.Usage = D3D10_USAGE_DYNAMIC;
		desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (FAILED(Device->CreateBuffer(&desc, nullptr, &shader_buf->ConstBuf)))
			return nullptr;
	}

	if (layout_num == 0)
		shader_buf->Layout = nullptr;
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
			return nullptr;
		FreeMem(semantics);
		FreeMem(descs);
	}

	return shader_buf;
}

void FinShaderBuf(SShaderBuf* shader_buf)
{
	if (shader_buf->Layout != nullptr)
		shader_buf->Layout->Release();
	if (shader_buf->ConstBuf != nullptr)
		shader_buf->ConstBuf->Release();
	if (shader_buf->Shader != nullptr)
	{
		switch (shader_buf->Kind)
		{
			case ShaderKind_Vs:
				static_cast<ID3D10VertexShader*>(shader_buf->Shader)->Release();
				break;
			case ShaderKind_Gs:
				static_cast<ID3D10GeometryShader*>(shader_buf->Shader)->Release();
				break;
			case ShaderKind_Ps:
				static_cast<ID3D10PixelShader*>(shader_buf->Shader)->Release();
				break;
			default:
				ASSERT(False);
				break;
		}
	}
	FreeMem(shader_buf);
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

Bool IsPowerOf2(U64 n)
{
	return (n & (n - 1)) == 0;
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

void* MakeDrawBuf(int tex_width, int tex_height, int split, HWND wnd, void* old, Bool editable)
{
	if (Device == nullptr)
		return nullptr;
	SWndBuf* old2 = static_cast<SWndBuf*>(old);
	FLOAT clear_color[4];
	if (old == nullptr)
	{
		clear_color[0] = 0.0f;
		clear_color[1] = 0.0f;
		clear_color[2] = 0.0f;
		clear_color[3] = 1.0f;
	}
	else
	{
		memcpy(clear_color, old2->ClearColor, sizeof(FLOAT) * 4);
		FinDrawBuf(old2);
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
		IDXGIFactory* factory = nullptr;
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
		if (factory != nullptr)
			factory->Release();
		if (!success)
			THROW(0xe9170009);
	}

	// Create a back buffer and a depth buffer.
	{
		ID3D10Texture2D* back = nullptr;
		ID3D10Texture2D* depth_stencil = nullptr;
		Bool success = False;
		for (; ; )
		{
			if (FAILED(wnd_buf->SwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void**>(&back))))
				break;
			if (FAILED(Device->CreateRenderTargetView(back, nullptr, &wnd_buf->RenderTargetView)))
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
				if (FAILED(Device->CreateTexture2D(&desc, nullptr, &depth_stencil)))
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
		if (depth_stencil != nullptr)
			depth_stencil->Release();
		if (back != nullptr)
			back->Release();
		if (!success)
			THROW(0xe9170009);
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
			if (FAILED(Device->CreateTexture2D(&desc, nullptr, &wnd_buf->TmpTex)))
				THROW(0xe9170009);
		}
		{
			D3D10_SHADER_RESOURCE_VIEW_DESC desc;
			memset(&desc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = 0;
			desc.Texture2D.MipLevels = 1;
			if (FAILED(Device->CreateShaderResourceView(wnd_buf->TmpTex, &desc, &wnd_buf->TmpShaderResView)))
				THROW(0xe9170009);
		}
		if (FAILED(Device->CreateRenderTargetView(wnd_buf->TmpTex, nullptr, &wnd_buf->TmpRenderTargetView)))
			THROW(0xe9170009);
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
		if (FAILED(Device->CreateTexture2D(&desc, nullptr, &wnd_buf->EditableTex)))
			THROW(0xe9170009);
	}
	else
		wnd_buf->EditableTex = nullptr;

	if (Callback2d != nullptr)
	{
		IDXGISurface* surface = nullptr;
		if (FAILED(wnd_buf->TmpTex->QueryInterface(&surface)))
			THROW(0xe9170009);
		double scale = 1.0 / static_cast<double>(split);
		void* params[2] = { surface, &scale };
		Callback2d(0, wnd_buf, params);
	}

	ActiveDrawBuf(wnd_buf);
	ResetViewport();
	return wnd_buf;
}

void FinDrawBuf(void* wnd_buf)
{
	if (CurWndBuf == wnd_buf)
		CurWndBuf = nullptr;
	SWndBuf* wnd_buf2 = static_cast<SWndBuf*>(wnd_buf);
	if (Callback2d != nullptr)
		Callback2d(1, wnd_buf, nullptr);
	if (wnd_buf2->TmpRenderTargetView != nullptr)
		wnd_buf2->TmpRenderTargetView->Release();
	if (wnd_buf2->TmpShaderResView != nullptr)
		wnd_buf2->TmpShaderResView->Release();
	if (wnd_buf2->TmpTex != nullptr)
		wnd_buf2->TmpTex->Release();
	if (wnd_buf2->DepthView != nullptr)
		wnd_buf2->DepthView->Release();
	if (wnd_buf2->RenderTargetView != nullptr)
		wnd_buf2->RenderTargetView->Release();
	if (wnd_buf2->SwapChain != nullptr)
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
		ResetViewport();

		if (Callback2d != nullptr)
			Callback2d(2, wnd_buf, nullptr);
	}
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

Bool MakeTexWithImg(ID3D10Texture2D** tex, ID3D10ShaderResourceView** view, ID3D10RenderTargetView** render_target_view, int width, int height, const void* img, size_t pitch, DXGI_FORMAT fmt, D3D10_USAGE usage, UINT cpu_access_flag, Bool render_target)
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
	if (render_target_view != nullptr)
	{
		if (FAILED(Device->CreateRenderTargetView(*tex, nullptr, render_target_view)))
			return False;
	}
	return True;
}

void ConstBuf(void* shader_buf, const void* data)
{
	SShaderBuf* shader_buf2 = static_cast<SShaderBuf*>(shader_buf);
	if (data != nullptr)
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

	if (shader_buf2->Layout != nullptr)
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

void ColorToArgb(double* a, double* r, double* g, double* b, S64 color)
{
	THROWDBG(color < 0 || 0xffffffff < color, 0xe9170006);
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

void SetJointMat(const void* element, double frame, float(*joint)[4][4])
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
