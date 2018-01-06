cbuffer ConstBuf: register(b0)
{
	float4 Color;
};

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
};

float4 main(PS_INPUT input): SV_TARGET
{
	float4 output = Color;
	if (output.a <= 0.02f)
		discard;
	if (input.Tex.x * input.Tex.x + input.Tex.y * input.Tex.y > 1.0f)
		discard;
	return output;
}
