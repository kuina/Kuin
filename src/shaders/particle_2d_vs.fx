cbuffer ConstBuf: register(b0)
{
	float4 Screen;
};

Texture2D Img0: register(t0);
Texture2D Img2: register(t1);
SamplerState Sampler: register(s0);

struct VS_INPUT
{
	float2 Pos: K_POSITION;
	float Idx: K_IDX;
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
	float Lifespan: K_LIFESPAN;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float2 tex = float2(input.Idx, 0.0f);
	float4 tex0 = Img0.SampleLevel(Sampler, tex, 0);
	float4 tex2 = Img2.SampleLevel(Sampler, tex, 0);

	if (tex0.w <= 0.0f)
	{
		output.Pos.xy = 0.0f;
		output.Lifespan = 0.0f;
	}
	else
	{
		float cos_th = cos(tex2.z);
		float sin_th = sin(tex2.z);
		output.Pos.xy = input.Pos * tex2.x / 2.0f;
		float x2, y2;
		x2 = cos_th * output.Pos.x + sin_th * output.Pos.y;
		y2 = -sin_th * output.Pos.x + cos_th * output.Pos.y;
		output.Pos.x = x2;
		output.Pos.y = y2;
		output.Pos.x += tex0.x;
		output.Pos.y += 1.0f / Screen.y - tex0.y;
		output.Pos.xy = output.Pos.xy * Screen.xy * 2.0f - 1.0f;
		output.Lifespan = clamp((tex0.w - 1.0f) / Screen.w, 0.0f, 1.0f);
	}
	output.Pos.z = tex0.z;
	output.Pos.w = 1.0f;
	output.Tex.x = (input.Pos.x + 1.0f) / 2.0f;
	output.Tex.y = 1.0f - (input.Pos.y + 1.0f) / 2.0f;
	return output;
}
