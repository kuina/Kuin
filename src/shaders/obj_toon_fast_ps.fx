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
Texture2D ImgToon: register(t2);
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
	float3 toon = ImgToon.Sample(Sampler, input.Tex).rgb;

	input.Normal = normalize(input.Normal);

	float up = input.Normal.y * 0.5f + 0.5f;
	float toon_x = dot(input.Normal.xyz, Dir.xyz) * 0.5f + 0.5f;
	float specular_x = clamp((0.0397436f * specular.w + 0.0856832f) * (specular.x + (1.0f - specular.x) * Half.w) * pow(max(dot(input.Normal, Half.xyz), 0.0f), specular.w) / max(max(dot(input.Normal, Dir.xyz), dot(input.Normal, Eye.xyz)), 0.00001f), 0.0f, 1.0f);
	float toon_value = ImgToon.Sample(Sampler, float2(toon_x, 0.0f)).r;
	output.xyz = DirColor.xyz *
		(
			diffuse.xyz *
			(toon_value + diffuse.xyz * 0.5f * (1.0f - toon_value)) +
			(AmbTopColor.xyz * up + AmbBottomColor.xyz * (1.0f - up)) +
			ImgToon.Sample(Sampler, float2(specular_x, 1.0f)).rgb
		);
	output.a = 1.0f;

	if (output.a <= 0.02f)
		discard;
	return output;
}
