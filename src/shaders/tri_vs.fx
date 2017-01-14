cbuffer ConstBuf: register(b0)
{
	float4 Vecs[2];
};

struct VS_INPUT
{
	float3 Weight: K_WEIGHT;
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos.xy = Vecs[0].rg * input.Weight.r + Vecs[0].ba * input.Weight.g + Vecs[1].rg * input.Weight.b;
	output.Pos.z = 0.0f;
	output.Pos.w = 1.0f;
	return output;
}
