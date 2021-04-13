cbuffer ConstBuf: register(b0)
{
	float4 AccelAndFriction;
	float4 SizeAccelAndRotAccel;
};

Texture2D Img0: register(t0);
Texture2D Img1: register(t1);
Texture2D Img2: register(t2);
SamplerState Sampler: register(s0);

struct PS_INPUT
{
	float4 Pos: SV_POSITION;
	float2 Tex: TEXCOORD;
};

struct PS_OUTPUT
{
	float4 Out0: SV_TARGET0;
	float4 Out1: SV_TARGET1;
	float4 Out2: SV_TARGET2;
};

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;
	output.Out0 = Img0.Sample(Sampler, input.Tex);
	output.Out1 = Img1.Sample(Sampler, input.Tex);
	output.Out2 = Img2.Sample(Sampler, input.Tex);

	// lifespan
	output.Out0.w = max(output.Out0.w - 1.0f, 0.0f);

	// velo
	output.Out1.xyz = (output.Out1.xyz + AccelAndFriction.xyz) * AccelAndFriction.w;

	// xyz
	output.Out0.xyz += output.Out1.xyz;

	// size
	output.Out2.y += SizeAccelAndRotAccel.x;
	output.Out2.x = max(output.Out2.x + output.Out2.y, 0.0f);

	// rot
	output.Out2.w += SizeAccelAndRotAccel.y;
	output.Out2.z += output.Out2.w;

	return output;
}
