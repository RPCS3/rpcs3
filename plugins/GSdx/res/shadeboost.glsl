//#version 420 // Keep it for editor detection

/*
** Contrast, saturation, brightness
** Code of this function is from TGM's shader pack
** http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=21057
*/

struct vertex_basic
{
    vec4 p;
    vec2 t;
};

#ifdef FRAGMENT_SHADER

layout(location = 0) in vertex_basic PSin;

layout(location = 0) out vec4 SV_Target0;

layout(std140, binding = 6) uniform cb0
{
    vec4 BGColor;
};

layout(binding = 0) uniform sampler2D TextureSampler;

// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150% 
vec4 ContrastSaturationBrightness(vec4 color)
{
	const float sat = SB_SATURATION / 50.0;
	const float brt = SB_BRIGHTNESS / 50.0;
	const float con = SB_CONTRAST / 50.0;
	
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor = color.rgb * brt;
    float dot_intensity = dot(brtColor, LumCoeff);
	vec3 intensity = vec3(dot_intensity, dot_intensity, dot_intensity);
	vec3 satColor = mix(intensity, brtColor, sat);
	vec3 conColor = mix(AvgLumin, satColor, con);

	color.rgb = conColor;	
	return color;
}


void ps_main()
{
    vec4 c = texture(TextureSampler, PSin.t);
	SV_Target0 = ContrastSaturationBrightness(c);
}


#endif
