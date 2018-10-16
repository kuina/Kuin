cbuffer ConstBuf: register(b0)
{
	float4 Poses[64];
	float4 Color[64];
};

struct VS_INPUT
{
	int Idx: K_IDX;
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
	float4 Color: K_COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos.xy = Poses[input.Idx].xy;
	output.Pos.z = 0.0f;
	output.Pos.w = 1.0f;
	output.Color = Color[input.Idx];
	return output;
}
