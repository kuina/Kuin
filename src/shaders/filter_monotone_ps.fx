cbuffer ConstBuf: register(b0)
{
	float4 Color;
};

Texture2D Img: register(t0);
SamplerState Sampler: register(s0);

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
};

float4 main(PS_INPUT input): SV_TARGET
{
	float4 tex_color = Img.Sample(Sampler, input.Tex);
	float luminance = 0.298912f * tex_color.r + 0.586611f * tex_color.g + 0.114478f * tex_color.b;
	float4 output;
	output.rgb = luminance * float3(Color.rgb) * Color.a + tex_color.rgb * (1.0f - Color.a);
	output.a = 1.0f;
	return output;
}
