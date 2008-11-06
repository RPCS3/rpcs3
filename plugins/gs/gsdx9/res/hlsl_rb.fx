sampler Texture : register(s0);

float4 main(float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.rgba = c.bgra;
	return c;
}
