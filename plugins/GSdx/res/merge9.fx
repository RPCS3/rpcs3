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

