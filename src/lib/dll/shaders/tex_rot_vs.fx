cbuffer ConstBuf: register(b0)
{
	float4 Vecs[2];
	float4 Rot[2];
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
	float2 xy = Vecs[0].ba * input.Weight - Rot[0].rg;
	output.Pos.x = xy.x * Rot[0].a - xy.y * Rot[0].b / Rot[1].r;
	output.Pos.y = xy.x * Rot[0].b * Rot[1].r + xy.y * Rot[0].a;
	output.Pos.xy += Vecs[0].rg + Rot[0].rg;
	output.Pos.z = 0.0f;
	output.Pos.w = 1.0f;
	output.Tex = Vecs[1].rg + Vecs[1].ba * input.Weight;
	return output;
}
