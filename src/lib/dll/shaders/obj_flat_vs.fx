cbuffer ConstBuf: register(b0)
{
	float4x4 World;
	float4x4 NormWorld;
	float4x4 ProjView;
	float4x4 ShadowProjView;
	float4 Eye;
	float4 Dir;
#ifdef JOINT
	float4x4 Joint[256];
#endif
};

struct VS_INPUT
{
	float3 Pos: POSITION;
	float3 Normal: NORMAL;
#ifndef FAST
	float3 Tangent: TANGENT;
#endif
	float2 Tex: TEXCOORD;
#ifdef JOINT
	float4 Weight: K_WEIGHT;
	int4 Joint: K_JOINT;
#endif
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// Convert vertices to world space.
#ifdef JOINT
	float4x4 joint_mat = input.Weight[0] * Joint[input.Joint[0]] + input.Weight[1] * Joint[input.Joint[1]];
	float4x4 mat = mul(World, joint_mat);
	float4 world_pos = mul(mat, float4(input.Pos, 1.0f));
#else
	float4 world_pos = mul(World, float4(input.Pos, 1.0f));
#endif

	output.Pos = mul(ProjView, world_pos);
	output.Tex = input.Tex;

	return output;
}
