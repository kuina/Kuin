cbuffer ConstBuf: register(b0)
{
	float4 AmbTopColor;
	float4 AmbBottomColor;
	float4 DirColor;
	float4 Eye;
	float4 Dir;
	float4 Half;
};

Texture2D ImgDiffuse: register(t0);
Texture2D ImgSpecular: register(t1);
SamplerState Sampler: register(s0);

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
	float3 Normal: NORMAL;
};

float4 main(PS_INPUT input): SV_TARGET
{
	float4 output;
	float4 diffuse = ImgDiffuse.Sample(Sampler, input.Tex);
	float4 specular = ImgSpecular.Sample(Sampler, input.Tex);

	input.Normal = normalize(input.Normal);

	float up = input.Normal.y * 0.5f + 0.5f;
	output.xyz = DirColor.xyz *
		(
			diffuse.xyz *
			(
				AmbTopColor.xyz * up + AmbBottomColor.xyz * (1.0f - up) +
				max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(input.Normal, Dir.xyz), 0.0f), 5.0f)), 0.0f) / 3.14159265358979f
			) +
			(0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * Half.w) * pow(max(dot(input.Normal, Half.xyz), 0.0f), specular.w) / max(max(dot(input.Normal, Dir.xyz), dot(input.Normal, Eye.xyz)), 0.00001f)
		);
	output.a = 1.0f;

	if (output.a <= 0.02f)
		discard;
	return output;
}
