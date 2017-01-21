cbuffer ConstBuf: register(b0)
{
	float4x4 World;
	float4x4 NormWorld;
	float4x4 ProjView;
	float4 Eye;
#ifdef JOINT
	float4x4 Joint[64];
#endif
};

struct VS_INPUT
{
	float3 Pos: POSITION;
	float3 Normal: NORMAL;
	float2 Tex: TEXCOORD;
#ifdef JOINT
	float4 Weight: K_WEIGHT;
	int4 Joint: K_JOINT;
#endif
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
	float3 Normal: NORMAL;
	float3 Eye: K_EYE;
	float2 Tex: TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// Convert vertices to world space.
#ifdef JOINT
	float4x4 joint_mat = input.Weight[0] * Joint[input.Joint[0]] + input.Weight[1] * Joint[input.Joint[1]] + input.Weight[2] * Joint[input.Joint[2]] + input.Weight[3] * Joint[input.Joint[3]];
	float4 world_pos = mul(World, mul(joint_mat, float4(input.Pos, 1.0f)));
	float4x4 normal_mat = mul(NormWorld, joint_mat);
#else
	float4 world_pos = mul(World, float4(input.Pos, 1.0f));
	float4x4 normal_mat = NormWorld;
#endif
	output.Pos = mul(ProjView, world_pos);
	output.Normal = mul(normal_mat, float4(input.Normal, 1.0f)).xyz;
	output.Eye = Eye.xyz - world_pos.xyz;

	// UV.
	output.Tex = input.Tex;

	return output;
}
