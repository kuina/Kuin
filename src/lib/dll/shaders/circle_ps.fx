cbuffer ConstBuf: register(b0)
{
	float4 Color;
	float4 PixelLen;
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
	float len = input.Tex.x * input.Tex.x + input.Tex.y * input.Tex.y;
	if (len > 1.0f)
		discard;
	output.a *= min((1.0f - len) * PixelLen.x, 1.0f);
	return output;
}
