#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency

float4 ConvertYUV(float4 color)
{
	float4 color2 = color;

	float fSat = SB_SATURATION / 100.0;
	float fBrt = SB_BRIGHTNESS / 100.0;
	float fCont = SB_CONTRAST / 100.0;

	float gY = color.r*0.299+color.g*0.587+color.b*0.114;
	float gCr = ( color.r-gY )*0.713*(0.5+fSat);
	float gCb = ( color.b-gY )*0.565*(0.5+fSat);

	gY = gY*(0.5+fCont);

	color2.r = gY + 1.40252*gCr;
	color2.g = gY - 0.714403*gCr - 0.343731*gCb;
	color2.b = gY + 1.76991*gCb;

	color2.r = color2.r*(0.5+fBrt);
	color2.g = color2.g*(0.5+fBrt);
	color2.b = color2.b*(0.5+fBrt);

	return color2;
}

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

float4 ps_main(PS_INPUT input) : SV_Target0
{
	float4 c = Texture.Sample(Sampler, input.t);
	return ConvertYUV(c);
}


#elif SHADER_MODEL <= 0x300

sampler Texture : register(s0);

float4 g_params[1];

#define BGColor	(g_params[0])

struct PS_INPUT
{
	float2 t : TEXCOORD0;
};

float4 ps_main(PS_INPUT input) : COLOR
{
	float4 c = tex2D(Texture, input.t);
	return ConvertYUV(c);
}

#endif
#endif