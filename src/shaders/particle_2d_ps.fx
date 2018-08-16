cbuffer ConstBuf: register(b0)
{
	float4 Color1;
	float4 Color2;
};

Texture2D Img: register(t0);
SamplerState Sampler: register(s0);

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
	float Lifespan: K_LIFESPAN;
};

float4 main(PS_INPUT input): SV_TARGET
{
	float4 color = Color1 * input.Lifespan + Color2 * (1.0f - input.Lifespan);
	float4 output = Img.Sample(Sampler, input.Tex) * color;
	return output;
}
