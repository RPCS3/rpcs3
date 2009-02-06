struct VS_INPUT
{
	float4 p : POSITION; 
	float2 t : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 p : SV_Position;
	float2 t : TEXCOORD0;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.p = input.p;
	output.t = input.t;

	return output;
}

Texture2D Texture;
SamplerState Sampler;

struct PS_INPUT
{
	float4 p : SV_Position;
	float2 t : TEXCOORD0;
};

float4 ps_main0(PS_INPUT input) : SV_Target0
{
	return Texture.Sample(Sampler, input.t);
}

uint ps_main1(PS_INPUT input) : SV_Target0
{
	float4 f = Texture.Sample(Sampler, input.t);

	f.a *= 256.0f/127; // hm, 0.5 won't give us 1.0 if we just multiply with 2

	uint4 i = f * float4(0x001f, 0x03e0, 0x7c00, 0x8000);

	return (i.x & 0x001f) | (i.y & 0x03e0) | (i.z & 0x7c00) | (i.w & 0x8000);	
}

float4 ps_main2(PS_INPUT input) : SV_Target0
{
	clip(Texture.Sample(Sampler, input.t).a - (0.5 - 0.9f/256));

	return 0;
}

float4 ps_main3(PS_INPUT input) : SV_Target0
{
	clip((0.5 - 0.9f/256) -  Texture.Sample(Sampler, input.t).a);

	return 0;
}

float4 ps_main4(PS_INPUT input) : SV_Target0
{
	float4 c = Texture.Sample(Sampler, input.t);
	
	return fmod(c * 255 + 0.5f, 256) / 255;
}
