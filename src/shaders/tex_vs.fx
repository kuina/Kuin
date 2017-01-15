cbuffer ConstBuf: register(b0)
{
	float4 Vecs[2];
};

struct VS_INPUT
{
	float2 Weight: K_WEIGHT;
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos.xy = Vecs[0].rg + Vecs[0].ba * input.Weight;
	output.Pos.z = 0.0f;
	output.Pos.w = 1.0f;
	output.Tex = Vecs[1].rg + Vecs[1].ba * input.Weight;
	return output;
}
