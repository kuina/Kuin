cbuffer ConstBuf: register(b0)
{
	float4 Dir;
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
	float3 Eye: K_EYE;
	float2 Tex: TEXCOORD;
};

float4 main(PS_INPUT input): SV_TARGET
{
	float3 n_normal = normalize(input.Normal);
	float3 n_eye = normalize(input.Eye);

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
				float normal_dir = dot(n_normal, Dir.xyz);
				float normal_eye = dot(n_normal, n_eye);
				float3 half = normalize(Dir.xyz + n_eye);
				float normal_harf = dot(n_normal, half);
				output.xyz = DirColor.xyz *
					(
						diffuse.xyz *
						(
							0.05f +
							max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - normal_dir, 0.0f), 5.0f)), 0.0f) / 3.14159265358979f
						) +
						(0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(n_eye, half), 0.0f), 5.0f)) * pow(max(normal_harf, 0.0f), specular.w) / max(max(normal_dir, normal_eye), 0.00001f)
					);
				output.a = 1.0f;
#ifdef DBG
			}
			break;
		// Albedo.
		case 1: output = diffuse; break;
		// Normal.
		case 2: output = float4(n_normal * 0.5f + 0.5f, 1.0f); break;
		case 3:
			{
				float normal_dir = dot(n_normal, Dir.xyz);
				output = float4(float3(normal_dir * 0.5f + 0.5f, normal_dir * 0.5f + 0.5f, normal_dir * 0.5f + 0.5f), 1.0f);
			}
			break;
		case 4:
			{
				float normal_eye = dot(n_normal, n_eye);
				output = float4(float3(normal_eye * 0.5f + 0.5f, normal_eye * 0.5f + 0.5f, normal_eye * 0.5f + 0.5f), 1.0f);
			}
			break;
		// Diffuse.
		case 5:
			{
				float normal_dir = dot(n_normal, Dir.xyz);
				output.xyz = DirColor.xyz * diffuse.xyz * max(1.0f - (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - normal_dir, 0.0f), 5.0f)), 0.0f) / 3.14159265358979f;
				output.a = 1.0f;
			}
			break;
		// Specular.
		case 6:
			{
				float normal_dir = dot(n_normal, Dir.xyz);
				float normal_eye = dot(n_normal, n_eye);
				float3 half = normalize(Dir.xyz + n_eye);
				float normal_harf = dot(n_normal, half);
				output.xyz = DirColor.xyz * (0.0397436f * specular.w + 0.0856832f) * (specular.xyz + (1.0f - specular.xyz) * pow(max(1.0f - dot(n_eye, half), 0.0f), 5.0f)) * pow(max(normal_harf, 0.0f), specular.w) / max(max(normal_dir, normal_eye), 0.00001f);
				output.a = 1.0f;
			}
			break;
		// Eye.
		case 7: output = float4(n_eye * 0.5f + 0.5f, 1.0f); break;
		// Directional light.
		case 8: output = float4(Dir.xyz * 0.5f + 0.5f, 1.0f); break;
		// UV.
		case 9: output = float4(input.Tex.x, input.Tex.y, 0.0f, 1.0f); break;
		default: discard; output = float4(0.0f, 0.0f, 0.0f, 0.0f); break;
	}
#endif
	if (output.a <= 0.04f)
		discard;
	return output;
}
