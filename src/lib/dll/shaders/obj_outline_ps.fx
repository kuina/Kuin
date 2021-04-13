cbuffer ConstBuf: register(b0)
{
	float4 OutlineColor;
};

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
};

float4 main(PS_INPUT input): SV_TARGET
{
	float4 output = OutlineColor;

	if (output.a <= 0.02f)
		discard;
	return output;
}
