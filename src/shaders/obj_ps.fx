cbuffer ConstBuf: register(b0)
{
	float4 AmbTopColor;
	float4 AmbBottomColor;
	float4 DirColor;
};

Texture2D ImgDiffuse: register(t0);
Texture2D ImgSpecular: register(t1);
Texture2D ImgNormal: register(t2);
SamplerState Sampler: register(s0);

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float3 Eye: K_EYE;
	float EyeLen: K_EYELEN;
	float3 Dir: K_DIR;
	float3 Up: K_UP;
	float2 Tex: TEXCOORD;
};

float4 main(PS_INPUT input): SV_TARGET
{
	input.Eye = normalize(input.Eye);
	input.Dir = normalize(input.Dir);

	float4 output;
	float4 diffuse = ImgDiffuse.Sample(Sampler, input.Tex);
	float4 specular = ImgSpecular.Sample(Sampler, input.Tex);
	float3 normal = normalize(ImgNormal.Sample(Sampler, input.Tex).rgb * 2.0f - 1.0f);

	float up = dot(input.Up, normal) * 0.5f + 0.5f;
	float3 half = normalize(input.Dir + input.Eye);
	output.xyz = DirColor.xyz *
		(
			diffuse.xyz *
			(
				AmbTopColor.xyz * up + AmbBottomColor.xyz * (1.0f - up) +
				max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(normal, input.Dir), 0.0f), 5.0f)), 0.0f) / 3.14159265358979f
			) +
			(0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(input.Eye, half), 0.0f), 5.0f)) * pow(max(dot(normal, half), 0.0f), specular.w) / max(max(dot(normal, input.Dir), dot(normal, input.Eye)), 0.00001f)
		);
	output.a = 1.0f;

	if (output.a <= 0.02f)
		discard;
	return output;
}
