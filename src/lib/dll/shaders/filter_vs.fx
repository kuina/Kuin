struct VS_INPUT
{
	float2 Pos: K_POSITION;
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos.xy = input.Pos;
	output.Pos.z = 0.0f;
	output.Pos.w = 1.0f;
	output.Tex.x = (input.Pos.x + 1.0f) / 2.0f;
	output.Tex.y = 1.0f - (input.Pos.y + 1.0f) / 2.0f;
	return output;
}
