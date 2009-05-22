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

float4 ps_crt(float2 t, int i)
{
	float4 mask[4] = 
	{
		float4(1, 0, 0, 0), 
		float4(0, 1, 0, 0), 
		float4(0, 0, 1, 0), 
		float4(1, 1, 1, 0)
	};
	
	return tex2D(Texture, t) * saturate(mask[i] + 0.5f);
}

float4 ps_main5(float2 t : TEXCOORD0, float4 vPos : VPOS) : COLOR // triangular
{
	int4 p = (int4)vPos;

	// return ps_crt(t, ((p.x + (p.y % 2) * 3) / 2) % 3);
	return ps_crt(t, ((p.x + ((p.y / 2) % 2) * 3) / 2) % 3);
}

float4 ps_main6(float2 t : TEXCOORD0, float4 vPos : VPOS) : COLOR // diagonal
{
	int4 p = (int4)vPos;

	return ps_crt(t, (p.x + (p.y % 3)) % 3);
}
