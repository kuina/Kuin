cbuffer ConstBuf: register(b0)
{
	float4 AmbTopColor;
	float4 AmbBottomColor;
	float4 DirColor;
};

Texture2D ImgDiffuse: register(t0);
Texture2D ImgSpecular: register(t1);
Texture2D ImgNormal: register(t2);
Texture2D ImgToon: register(t3);
SamplerState Sampler: register(s0);
#ifdef SM
Texture2D ImgSm: register(t4);
SamplerComparisonState SamplerSm: register(s1);
#endif

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float3 Eye: K_EYE;
	float EyeLen: K_EYELEN;
	float3 Dir: K_DIR;
	float3 Up: K_UP;
	float2 Tex: TEXCOORD;
#ifdef SM
	float4 TexSm: K_SM_COORD;
#endif
};

float4 main(PS_INPUT input): SV_TARGET
{
	input.Eye = normalize(input.Eye);
	input.Dir = normalize(input.Dir);

	float4 output;
	float4 diffuse = ImgDiffuse.Sample(Sampler, input.Tex);
	float4 specular = ImgSpecular.Sample(Sampler, input.Tex);
	float3 normal = normalize(ImgNormal.Sample(Sampler, input.Tex).rgb * 2.0f - 1.0f);
	float3 toon = ImgToon.Sample(Sampler, input.Tex).rgb;

	float up = dot(input.Up, normal) * 0.5f + 0.5f;
	float3 half = normalize(input.Dir + input.Eye);
	float toon_x = dot(normal, input.Dir) * 0.5f + 0.5f;
	float specular_x = clamp((0.0397436f * specular.w + 0.0856832f) * (specular.x + (1.0f - specular.x) * pow(max(1.0f - dot(input.Eye, half), 0.0f), 5.0f)) * pow(max(dot(normal, half), 0.0f), specular.w) / max(max(dot(normal, input.Dir), dot(normal, input.Eye)), 0.00001f), 0.0f, 1.0f);
	float toon_value = ImgToon.Sample(Sampler, float2(toon_x, 0.0f)).r;

#ifdef SM
	float3 coord = input.TexSm.xyz / input.TexSm.w;
	float bias = min(0.01f + 0.01f * max(abs(ddx(coord.z)), abs(ddy(coord.z))), 0.1f);
	float3 dirColor2 = DirColor.xyz * ImgSm.SampleCmpLevelZero(SamplerSm, coord.xy, saturate(coord.z - bias));
#else
	float3 dirColor2 = DirColor.xyz;
#endif

	output.xyz =
		diffuse.xyz *
		(
			AmbTopColor.xyz * up + AmbBottomColor.xyz * (1.0f - up) +
			dirColor2 * toon_value
		) +
		dirColor2 * ImgToon.Sample(Sampler, float2(specular_x, 1.0f)).rgb;
	output.a = diffuse.w;

	if (output.a <= 0.02f)
		discard;
	return output;
}
