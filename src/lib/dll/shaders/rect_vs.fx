cbuffer ConstBuf: register(b0)
{
	float4 Vecs;
};

struct VS_INPUT
{
	float2 Weight: K_WEIGHT;
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos.xy = Vecs.rg + Vecs.ba * input.Weight;
	output.Pos.z = 0.0f;
	output.Pos.w = 1.0f;
	return output;
}
