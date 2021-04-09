#include "draw_particle.h"

#include "draw_device.h"

static const D3D10_VIEWPORT ParticleViewport = { 0, 0, static_cast<UINT>(ParticleNum), 1, 0.0f, 1.0f };

static void UpdateParticles(SParticle* particle);

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
	ConstBuf(ShaderBufs[ShaderBuf_Particle2dVs], screen);
	Device->GSSetShader(nullptr);
	ConstBuf(ShaderBufs[ShaderBuf_Particle2dPs], &me2->PsConstBuf);
	VertexBuf(VertexBufs[VertexBuf_ParticleVertex]);
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

EXPORT_CPP void _particleFin(SClass* me_)
{
	SParticle* me2 = reinterpret_cast<SParticle*>(me_);
	if (me2->TexSet != nullptr)
	{
		for (int i = 0; i < ParticleTexNum * 2; i++)
		{
			if (me2->TexSet[i].RenderTargetViewParam != nullptr)
				me2->TexSet[i].RenderTargetViewParam->Release();
			if (me2->TexSet[i].ViewParam != nullptr)
				me2->TexSet[i].ViewParam->Release();
			if (me2->TexSet[i].TexParam != nullptr)
				me2->TexSet[i].TexParam->Release();
		}
		FreeMem(me2->TexSet);
		me2->TexSet = nullptr;
	}
	if (me2->TexTmp != nullptr)
	{
		me2->TexTmp->Release();
		me2->TexTmp = nullptr;
	}
}

EXPORT_CPP SClass* _makeParticle(SClass* me_, S64 life_span, S64 color1, S64 color2, double friction, double accel_x, double accel_y, double accel_z, double size_accel, double rot_accel)
{
	THROWDBG(life_span <= 0, 0xe9170006);
	THROWDBG(friction < 0.0, 0xe9170006);

	SParticle* me2 = reinterpret_cast<SParticle*>(me_);
	me2->Lifespan = life_span;
	me2->ParticlePtr = 0;
	me2->Draw1To2 = True;

	{
		double a, r, g, b;
		ColorToArgb(&a, &r, &g, &b, color1);
		me2->PsConstBuf.Color1[0] = static_cast<float>(r);
		me2->PsConstBuf.Color1[1] = static_cast<float>(g);
		me2->PsConstBuf.Color1[2] = static_cast<float>(b);
		me2->PsConstBuf.Color1[3] = static_cast<float>(a);
	}
	{
		double a, r, g, b;
		ColorToArgb(&a, &r, &g, &b, color2);
		me2->PsConstBuf.Color2[0] = static_cast<float>(r);
		me2->PsConstBuf.Color2[1] = static_cast<float>(g);
		me2->PsConstBuf.Color2[2] = static_cast<float>(b);
		me2->PsConstBuf.Color2[3] = static_cast<float>(a);
	}

	me2->UpdatingPsConstBuf.AccelAndFriction[0] = static_cast<float>(accel_x);
	me2->UpdatingPsConstBuf.AccelAndFriction[1] = static_cast<float>(accel_y);
	me2->UpdatingPsConstBuf.AccelAndFriction[2] = static_cast<float>(accel_z);
	me2->UpdatingPsConstBuf.AccelAndFriction[3] = static_cast<float>(friction);
	me2->UpdatingPsConstBuf.SizeAccelAndRotAccel[0] = static_cast<float>(size_accel);
	me2->UpdatingPsConstBuf.SizeAccelAndRotAccel[1] = static_cast<float>(rot_accel);
	me2->UpdatingPsConstBuf.SizeAccelAndRotAccel[2] = 0.0f;
	me2->UpdatingPsConstBuf.SizeAccelAndRotAccel[3] = 0.0f;

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
			return nullptr;
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
		if (FAILED(Device->CreateTexture2D(&desc, nullptr, &me2->TexTmp)))
			return nullptr;
	}
	return me_;
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
	Device->OMSetRenderTargets(static_cast<UINT>(ParticleTexNum), targets, nullptr);
	Device->RSSetViewports(1, &ParticleViewport);
	{
		ConstBuf(ShaderBufs[ShaderBuf_ParticleUpdatingVs], nullptr);
		Device->GSSetShader(nullptr);
		ConstBuf(ShaderBufs[ShaderBuf_ParticleUpdatingPs], &particle->UpdatingPsConstBuf);
		VertexBuf(VertexBufs[VertexBuf_ParticleUpdatingVertex]);
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
	ResetViewport();

	particle->Draw1To2 = !particle->Draw1To2;
}
