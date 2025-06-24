R"(
#version 420
#extension GL_ARB_separate_shader_objects : enable

%preprocessor

#ifdef VULKAN
#define USE_UBO 1
#endif

#define SAMPLER_MODE_NONE      0
#define SAMPLER_MODE_FONT2D    1
#define SAMPLER_MODE_FONT3D    2
#define SAMPLER_MODE_TEXTURE2D 3

#ifdef VULKAN
	layout(set=0, binding=0) uniform sampler2D fs0;
	layout(set=0, binding=1) uniform sampler2DArray fs1;
#else
	layout(binding=31) uniform sampler2D fs0;
	layout(binding=30) uniform sampler2DArray fs1;
#endif

layout(location=0) in vec2 tc0;
layout(location=1) in vec4 color;
layout(location=2) in vec4 clip_rect;

layout(location=0) out vec4 ocol;

#if USE_UBO
layout(%push_block) uniform FragmentConfiguration
{
	%push_block_offset
	uint fragment_config;
	float timestamp;
	float blur_intensity;
};
#else
	uniform uint fragment_config;
	uniform float timestamp;
	uniform float blur_intensity;
#endif

struct config_t
{
	bool clip_fragments;
	bool use_pulse_glow;
	uint sampler_mode;
};

config_t unpack_fragment_options()
{
	config_t result;
	result.clip_fragments = bitfieldExtract(fragment_config, 0, 1) != 0;
	result.use_pulse_glow = bitfieldExtract(fragment_config, 1, 1) != 0;
	result.sampler_mode = bitfieldExtract(fragment_config, 2, 2);
	return result;
}

vec4 blur_sample(sampler2D tex, vec2 coord, vec2 tex_offset)
{
	vec2 coords[9];
	coords[0] = coord - tex_offset;
	coords[1] = coord + vec2(0., -tex_offset.y);
	coords[2] = coord + vec2(tex_offset.x, -tex_offset.y);
	coords[3] = coord + vec2(-tex_offset.x, 0.);
	coords[4] = coord;
	coords[5] = coord + vec2(tex_offset.x, 0.);
	coords[6] = coord + vec2(-tex_offset.x, tex_offset.y);
	coords[7] = coord + vec2(0., tex_offset.y);
	coords[8] = coord + tex_offset;

	float weights[9] =
	{
		1., 2., 1.,
		2., 4., 2.,
		1., 2., 1.
	};

	vec4 blurred = vec4(0.);
	for (int n = 0; n < 9; ++n)
	{
		blurred += texture(tex, coords[n]) * weights[n];
	}

	return blurred / 16.f;
}

vec4 sample_image(sampler2D tex, vec2 coord, float blur_strength)
{
	vec4 original = texture(tex, coord);
	if (blur_strength == 0) return original;

	vec2 constraints = 1.f / vec2(640, 360);
	vec2 res_offset = 1.f / textureSize(fs0, 0);
	vec2 tex_offset = max(res_offset, constraints);

	// Sample triangle pattern and average
	// TODO: Nicer looking gaussian blur with less sampling
	vec4 blur0 = blur_sample(tex, coord + vec2(-res_offset.x, 0.), tex_offset);
	vec4 blur1 = blur_sample(tex, coord + vec2(res_offset.x, 0.), tex_offset);
	vec4 blur2 = blur_sample(tex, coord + vec2(0., res_offset.y), tex_offset);

	vec4 blurred = blur0 + blur1 + blur2;
	blurred /= 3.;
	return mix(original, blurred, blur_strength);
}

void main()
{
	config_t config = unpack_fragment_options();
	if (config.clip_fragments)
	{
		if (gl_FragCoord.x < clip_rect.x || gl_FragCoord.x > clip_rect.z ||
			gl_FragCoord.y < clip_rect.y || gl_FragCoord.y > clip_rect.w)
		{
			discard;
			return;
		}
	}

	vec4 diff_color = color;
	if (config.use_pulse_glow)
	{
		diff_color.a *= (sin(timestamp) + 1.f) * 0.5f;
	}

	switch (config.sampler_mode)
	{
	default:
	case SAMPLER_MODE_NONE:
		ocol = diff_color;
		break;
	case SAMPLER_MODE_FONT2D:
		ocol = texture(fs0, tc0).rrrr * diff_color;
		break;
	case SAMPLER_MODE_FONT3D:
		ocol = texture(fs1, vec3(tc0.x, fract(tc0.y), trunc(tc0.y))).rrrr * diff_color;
		break;
	case SAMPLER_MODE_TEXTURE2D:
#ifdef VULKAN
		ocol = sample_image(fs0, tc0, blur_intensity).bgra * diff_color;
#else
		ocol = sample_image(fs0, tc0, blur_intensity).rgba * diff_color;
#endif
		break;
	}
}
)"
