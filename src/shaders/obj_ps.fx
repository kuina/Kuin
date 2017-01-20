cbuffer ConstBuf: register(b0)
{
	float4 DirColor;
#ifdef DBG
	int4 Mode;
#endif
};

Texture2D ImgDiffuse: register(t0);
Texture2D ImgSpecular: register(t1);
SamplerState Sampler: register(s0);

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float3 Eye: K_EYE;
	float EyeLen: K_EYELEN;
	float3 Dir: K_DIR;
	float2 Tex: TEXCOORD;
};

float4 main(PS_INPUT input): SV_TARGET
{
	input.Normal = normalize(input.Normal);
	input.Tangent = normalize(input.Tangent);
	input.Binormal = normalize(input.Binormal);
	input.Eye = normalize(input.Eye);
	input.Dir = normalize(input.Dir);

	float4 output;
	float4 diffuse = ImgDiffuse.Sample(Sampler, input.Tex);
	float4 specular = ImgSpecular.Sample(Sampler, input.Tex);
#ifdef DBG
	switch (Mode.x)
	{
		// Integrated.
		case 0:
			{
#endif
				float3 half = normalize(input.Dir + input.Eye);
				output.xyz = DirColor.xyz *
					(
						diffuse.xyz *
						(
							0.05f +
							max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - input.Dir.y, 0.0f), 5.0f)), 0.0f) / 3.14159265358979f
						) +
						(0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(input.Eye, half), 0.0f), 5.0f)) * pow(max(half.y, 0.0f), specular.w) / max(max(input.Dir.y, input.Eye.y), 0.00001f)
					);
				output.a = 1.0f;
#ifdef DBG
			}
			break;
		// Albedo.
		case 1: output = diffuse; break;
		// Normal.
		case 2: output = float4(input.Normal.xyz * 0.5f + 0.5f, 1.0f); break;
		// Tangent.
		case 3: output = float4(input.Tangent.xyz * 0.5f + 0.5f, 1.0f); break;
		// Binormal.
		case 4: output = float4(input.Binormal.xyz * 0.5f + 0.5f, 1.0f); break;
		// Diffuse.
		case 5:
			output.xyz = DirColor.xyz * diffuse.xyz * max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - input.Dir.y, 0.0f), 5.0f)), 0.0f) / 3.14159265358979f;
			output.a = 1.0f;
			break;
		// Specular.
		case 6:
			{
				float3 half = normalize(input.Dir + input.Eye);
				output.xyz = DirColor.xyz * (0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(input.Eye, half), 0.0f), 5.0f)) * pow(max(half.y, 0.0f), specular.w) / max(max(input.Dir.y, input.Eye.y), 0.00001f);
				output.a = 1.0f;
			}
			break;
		// Eye.
		case 7: output = float4(input.Eye * 0.5f + 0.5f, 1.0f); break;
		// Directional light.
		case 8: output = float4(input.Dir * 0.5f + 0.5f, 1.0f); break;
		// UV.
		case 9: output = float4(input.Tex.x, input.Tex.y, 0.0f, 1.0f); break;
		default: discard; output = float4(0.0f, 0.0f, 0.0f, 0.0f); break;
	}
#endif
	if (output.a <= 0.04f)
		discard;
	return output;
}
