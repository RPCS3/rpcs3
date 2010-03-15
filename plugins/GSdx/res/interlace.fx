#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency
#if SHADER_MODEL >= 0x400

Texture2D Texture;
SamplerState Sampler;

cbuffer cb0
{
	float2 ZrH;
	float hH;
};

struct PS_INPUT
{
	float4 p : SV_Position;
	float2 t : TEXCOORD0;
};

float4 ps_main0(PS_INPUT input) : SV_Target0
{
	clip(frac(input.t.y * hH) - 0.5);

	return Texture.Sample(Sampler, input.t);
}

float4 ps_main1(PS_INPUT input) : SV_Target0
{
	clip(0.5 - frac(input.t.y * hH));

	return Texture.Sample(Sampler, input.t);
}

float4 ps_main2(PS_INPUT input) : SV_Target0
{
	float4 c0 = Texture.Sample(Sampler, input.t - ZrH);
	float4 c1 = Texture.Sample(Sampler, input.t);
	float4 c2 = Texture.Sample(Sampler, input.t + ZrH);

	return (c0 + c1 * 2 + c2) / 4;
}

float4 ps_main3(PS_INPUT input) : SV_Target0
{
	return Texture.Sample(Sampler, input.t);
}

#elif SHADER_MODEL <= 0x300

sampler s0 : register(s0);

float4 Params1 : register(c0);

#define ZrH (Params1.xy)
#define hH  (Params1.z)

float4 ps_main0(float2 tex : TEXCOORD0) : COLOR
{
	clip(frac(tex.y * hH) - 0.5);

	return tex2D(s0, tex);
}

float4 ps_main1(float2 tex : TEXCOORD0) : COLOR
{
	clip(0.5 - frac(tex.y * hH));

	return tex2D(s0, tex);
}

float4 ps_main2(float2 tex : TEXCOORD0) : COLOR
{
	float4 c0 = tex2D(s0, tex - ZrH);
	float4 c1 = tex2D(s0, tex);
	float4 c2 = tex2D(s0, tex + ZrH);

	return (c0 + c1 * 2 + c2) / 4;
}

float4 ps_main3(float2 tex : TEXCOORD0) : COLOR
{
	return tex2D(s0, tex);
}

#endif
#endif
