struct VS_INPUT
{
	float4 p : POSITION; 
	float2 t : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 p : POSITION;
	float2 t : TEXCOORD0;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.p = input.p;
	output.t = input.t;

	return output;
}

sampler Texture : register(s0);

float4 ps_main0(float2 t : TEXCOORD0) : COLOR
{
	return tex2D(Texture, t);
}

float4 ps_main1(float2 t : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, t);
	c.a *= 128.0f / 255; // *= 0.5f is no good here, need to do this in order to get 0x80 for 1.0f (instead of 0x7f)
	return c;
}

float4 ps_main2(float2 t : TEXCOORD0) : COLOR
{
	clip(tex2D(Texture, t).a - (1.0f - 0.9f/256));

	return 0;
}

float4 ps_main3(float2 t : TEXCOORD0) : COLOR
{
	clip((1.0f - 0.9f/256) -  tex2D(Texture, t).a);

	return 0;
}

float4 ps_main4() : COLOR
{
	return 1;
}

