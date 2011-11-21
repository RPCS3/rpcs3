#if 0

Texture2D Texture;
SamplerState Sampler;

cbuffer cb0
{
	float4 BGColor;
};

struct PS_INPUT
{
	float4 p : SV_Position;
	float2 t : TEXCOORD0;
};

float4 ps_main0(PS_INPUT input) : SV_Target0
{
	float4 c = Texture.Sample(Sampler, input.t);
	c.a = min(c.a * 2, 1);
	return c;
}

float4 ps_main1(PS_INPUT input) : SV_Target0
{
	float4 c = Texture.Sample(Sampler, input.t);
	c.a = BGColor.a;
	return c;
}

#endif
