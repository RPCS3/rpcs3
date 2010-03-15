#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency
#if SHADER_MODEL >= 0x400

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

#elif SHADER_MODEL <= 0x300

sampler Texture : register(s0);

float4 g_params[1];

#define BGColor	(g_params[0])

struct PS_INPUT
{
	float2 t : TEXCOORD0;
};

float4 ps_main0(PS_INPUT input) : COLOR
{
	float4 c = tex2D(Texture, input.t);
	// a = ;
	return c.bgra;
}

float4 ps_main1(PS_INPUT input) : COLOR
{
	float4 c = tex2D(Texture, input.t);
	c.a = BGColor.a;
	return c.bgra;
}

#endif
#endif
