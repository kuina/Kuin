cbuffer ConstBuf: register(b0)
{
	float4 AmbTopColor;
	float4 AmbBottomColor;
	float4 DirColor;
#ifdef DBG
	int4 Mode;
#endif
};

Texture2D ImgDiffuse: register(t0);
Texture2D ImgSpecular: register(t1);
Texture2D ImgNormal: register(t2);
SamplerState Sampler: register(s0);

struct PS_INPUT
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

float4 main(PS_INPUT input): SV_TARGET
{
#ifdef DBG
	input.Normal = normalize(input.Normal);
	input.Tangent = normalize(input.Tangent);
	input.Binormal = normalize(input.Binormal);
#endif
	input.Eye = normalize(input.Eye);
	input.Dir = normalize(input.Dir);

	float4 output;
	float4 diffuse = ImgDiffuse.Sample(Sampler, input.Tex);
	float4 specular = ImgSpecular.Sample(Sampler, input.Tex);
	float3 normal = normalize(ImgNormal.Sample(Sampler, input.Tex).rgb * 2.0f - 1.0f);

#ifdef DBG
	switch (Mode.x)
	{
		// Integrated.
		case 0:
			{
#endif
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
#ifdef DBG
			}
			break;
		// Albedo.
		case 1: output = float4(diffuse.xyz, 1.0f); break;
		// Normal.
		case 2: output = float4(dot(input.Tangent.xyz, normal) * 0.5f + 0.5f, dot(input.Binormal.xyz, normal) * 0.5f + 0.5f, dot(input.Normal.xyz, normal) * 0.5f + 0.5f, 1.0f); break;
		// Diffuse.
		case 5:
			output.xyz = DirColor.xyz * diffuse.xyz * max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(normal, input.Dir), 0.0f), 5.0f)), 0.0f) / 3.14159265358979f;
			output.a = 1.0f;
			break;
		// Specular.
		case 6:
			{
				float3 half = normalize(input.Dir + input.Eye);
				output.xyz = DirColor.xyz * (0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(input.Eye, half), 0.0f), 5.0f)) * pow(max(dot(normal, half), 0.0f), specular.w) / max(max(dot(normal, input.Dir), dot(normal, input.Eye)), 0.00001f);
				output.a = 1.0f;
			}
			break;
		// Eye.
		case 7: output = float4(input.Eye * 0.5f + 0.5f, 1.0f); break;
		// Directional light.
		case 8: output = float4(input.Dir * 0.5f + 0.5f, 1.0f); break;
		// UV.
		case 9: output = float4(input.Tex.x, input.Tex.y, 0.0f, 1.0f); break;
		// Ambient.
		case 10:
			{
				float up = dot(input.Up, normal) * 0.5f + 0.5f;
				output.xyz = AmbTopColor.xyz * up + AmbBottomColor.xyz * (1.0f - up);
				output.a = 1.0f;
			}
			break;
		default: discard; output = float4(0.0f, 0.0f, 0.0f, 0.0f); break;
	}
#endif
	if (output.a <= 0.02f)
		discard;
	return output;
}
