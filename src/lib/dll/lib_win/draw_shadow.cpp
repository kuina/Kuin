#include "draw_shadow.h"

EXPORT_CPP void _shadowAdd(SClass* me_, SClass* obj, S64 element, double frame)
{
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);
	SObj* obj2 = reinterpret_cast<SObj*>(obj);
	THROWDBG(element < 0 || static_cast<S64>(obj2->ElementNum) <= element, 0xe9170006);
	SObjShadowVsConstBuf const_buf;
	switch (obj2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(obj2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), 0xe9170006);
				if (element2->JointNum < 0 || JointMax < element2->JointNum)
					THROW(0xe9170008);
				Bool joint = element2->JointNum != 0;

				memcpy(const_buf.World, obj2->Mat, sizeof(float[4][4]));
				memcpy(const_buf.ProjView, me2->ShadowProjView, sizeof(float[4][4]));
				if ((obj2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, const_buf.Joint);
						ConstBuf(ShaderBufs[ShaderBuf_ObjShadowJointVs], &const_buf);
					}
					else
						ConstBuf(ShaderBufs[ShaderBuf_ObjShadowVs], &const_buf);
					Device->GSSetShader(nullptr);
					Device->PSSetShader(nullptr);
					VertexBuf(element2->VertexBuf);
				}
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

EXPORT_CPP void _shadowBeginRecord(SClass* me_, double x, double y, double z, double radius)
{
	THROWDBG(radius <= 0.0, 0xe9170006);
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);

	double proj_mat[4][4];
	double view_mat[4][4];

	double dir_at[3] =
	{
		-static_cast<double>(ObjVsConstBuf.CommonParam.Dir[0]),
		-static_cast<double>(ObjVsConstBuf.CommonParam.Dir[1]),
		-static_cast<double>(ObjVsConstBuf.CommonParam.Dir[2])
	};
	double len = Normalize(dir_at);
	THROWDBG(len == 0.0, 0xe917000a);

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
		Cross(right, up, dir_at);
		if (Normalize(right) == 0.0)
		{
			up[0] = 0.0;
			up[1] = 0.0;
			up[2] = 1.0;
			Cross(right, up, dir_at);
			Normalize(right);
		}

		Cross(up, dir_at, right);

		eye[0] = eye_pos[0];
		eye[1] = eye_pos[1];
		eye[2] = eye_pos[2];
		pxyz[0] = Dot(eye, right);
		pxyz[1] = Dot(eye, up);
		pxyz[2] = Dot(eye, dir_at);

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
	MulMat(proj_view, proj_mat, view_mat);
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
		SetProjViewMat(ObjVsConstBuf.CommonParam.ShadowProjView, uv_mat, proj_view);
	}

	Device->OMSetRenderTargets(0, nullptr, me2->DepthView);
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
	UNUSED(me_);
	Device->OMSetRenderTargets(1, &CurWndBuf->TmpRenderTargetView, CurWndBuf->DepthView);
	ResetViewport();
}

EXPORT_CPP void _shadowFin(SClass* me_)
{
	SShadow* me2 = reinterpret_cast<SShadow*>(me_);
	if (me2->ShadowProjView != nullptr)
	{
		FreeMem(me2->ShadowProjView);
		me2->ShadowProjView = nullptr;
	}
	if (me2->DepthResView != nullptr)
	{
		me2->DepthResView->Release();
		me2->DepthResView = nullptr;
	}
	if (me2->DepthView != nullptr)
	{
		me2->DepthView->Release();
		me2->DepthView = nullptr;
	}
	if (me2->DepthTex != nullptr)
	{
		me2->DepthTex->Release();
		me2->DepthTex = nullptr;
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
				if (FAILED(Device->CreateTexture2D(&desc, nullptr, &me2->DepthTex)))
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
					return nullptr;
			}
			success = True;
			break;
		}
		if (!success)
			THROW(0xe9170009);
	}
	me2->DepthWidth = static_cast<int>(width);
	me2->DepthHeight = static_cast<int>(height);
	me2->ShadowProjView = static_cast<float(*)[4]>(AllocMem(sizeof(float) * 4 * 4));
	return me_;
}
