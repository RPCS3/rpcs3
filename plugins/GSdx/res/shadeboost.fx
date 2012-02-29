#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency

/*
** Contrast, saturation, brightness
** Code of this function is from TGM's shader pack
** http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=21057
*/

// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150% 
float4 ContrastSaturationBrightness(float4 color) // Ported to HLSL
{
	const float sat = SB_SATURATION / 50.0;
	const float brt = SB_BRIGHTNESS / 50.0;
	const float con = SB_CONTRAST / 50.0;
	
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const float3 LumCoeff = float3(0.2125, 0.7154, 0.0721);
	
	float3 AvgLumin = float3(AvgLumR, AvgLumG, AvgLumB);
	float3 brtColor = color.rgb * brt;
	float3 intensity = dot(brtColor, LumCoeff);
	float3 satColor = lerp(intensity, brtColor, sat);
	float3 conColor = lerp(AvgLumin, satColor, con);

	color.rgb = conColor;	
	return color;
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
	return ContrastSaturationBrightness(c);
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
	return ContrastSaturationBrightness(c);
}

#endif
#endif