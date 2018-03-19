cbuffer ConstBuf: register(b0)
{
	float4x4 World;
	float4x4 NormWorld;
	float4x4 ProjView;
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
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 Tex: TEXCOORD;
#ifdef JOINT
	float4 Weight: K_WEIGHT;
	int4 Joint: K_JOINT;
#endif
};

struct VS_OUTPUT
{
	float4 Pos: SV_POSITION;
#ifdef DBG
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
#endif
	float3 Eye: K_EYE;
	float EyeLen: K_EYELEN;
	float3 Dir: K_DIR;
	float3 Up: K_UP;
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

	// Convert normals to world space.
	float4 tangent = mul(normal_mat, float4(input.Tangent, 1.0f));
	float4 binormal = mul(normal_mat, float4(input.Binormal, 1.0f));
	float4 normal = mul(normal_mat, float4(input.Normal, 1.0f));
	float4x4 space =
	{
		{ tangent.x, tangent.y, tangent.z, 0.0f },
		{ binormal.x, binormal.y, binormal.z, 0.0f },
		{ normal.x, normal.y, normal.z, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};

	// Convert each information to normal space.
#ifdef DBG
	output.Tangent = mul(space, float4(1.0f, 0.0f, 0.0f, 1.0f)).xyz;
	output.Binormal = mul(space, float4(0.0f, 1.0f, 0.0f, 1.0f)).xyz;
	output.Normal = mul(space, float4(0.0f, 0.0f, 1.0f, 1.0f)).xyz;
#endif
	output.Eye = mul(space, float4(normalize(Eye.xyz - world_pos.xyz), 1.0f)).xyz;
	output.EyeLen = Eye.w;
	output.Dir = mul(space, float4(Dir.xyz, 1.0f)).xyz;
	output.Up = mul(space, float4(0.0f, 1.0f, 0.0f, 1.0f)).xyz;

	// UV.
	output.Tex = input.Tex;

	return output;
}
