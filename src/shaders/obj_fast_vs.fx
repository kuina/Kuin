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
	float3 Normal: NORMAL;
#ifdef SM
	float4 TexSm: K_SM_COORD;
#endif
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// Convert vertices to world space.
#ifdef JOINT
	float4x4 joint_mat = input.Weight[0] * Joint[input.Joint[0]] + input.Weight[1] * Joint[input.Joint[1]];
	float4x4 mat = mul(World, joint_mat);
	float4 world_pos = mul(mat, float4(input.Pos, 1.0f));
	float a = 1.0f / (mat[0][0] * mat[1][1] * mat[2][2] + mat[0][1] * mat[1][2] * mat[2][0] + mat[0][2] * mat[1][0] * mat[2][1] - mat[0][2] * mat[1][1] * mat[2][0] - mat[0][1] * mat[1][0] * mat[2][2] - mat[0][0] * mat[1][2] * mat[2][1]);
	float4x4 normal_mat =
	{
		{ a * (mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1]), -a * (mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0]), a * (mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0]), 0.0f },
		{ -a * (mat[0][1] * mat[2][2] - mat[0][2] * mat[2][1]), a * (mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0]), -a * (mat[0][0] * mat[2][1] - mat[0][1] * mat[2][0]), 0.0f },
		{ a * (mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1]), -a * (mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0]), a * (mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0]), 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
#else
	float4 world_pos = mul(World, float4(input.Pos, 1.0f));
	float4x4 normal_mat = NormWorld;
#endif

#ifdef SM
	output.TexSm = mul(ShadowProjView, world_pos);
#endif

	output.Pos = mul(ProjView, world_pos);
	output.Tex = input.Tex;
	output.Normal = mul(normal_mat, float4(input.Normal, 1.0f)).xyz;

	return output;
}
