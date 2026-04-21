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

#define SDF_DISABLED  0
#define SDF_ELLIPSE   1
#define SDF_BOX       2
#define SDF_ROUND_BOX 3

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
	vec4 sdf_params;
	vec4 sdf_origin;
	vec4 sdf_border_color;
};
#else
	uniform uint fragment_config;
	uniform float timestamp;
	uniform float blur_intensity;
	uniform vec4 sdf_params;
	uniform vec2 sdf_origin;
	uniform vec4 sdf_border_color;
#endif

struct config_t
{
	bool clip_fragments;
	bool use_pulse_glow;
	uint sampler_mode;
	uint sdf;
};

config_t unpack_fragment_options()
{
	config_t result;
	result.clip_fragments = bitfieldExtract(fragment_config, 0, 1) != 0;
	result.use_pulse_glow = bitfieldExtract(fragment_config, 1, 1) != 0;
	result.sampler_mode = bitfieldExtract(fragment_config, 2, 2);
	result.sdf = bitfieldExtract(fragment_config, 4, 2);
	return result;
}

vec4 SDF_blend(
	const in float sd,
	const in float border_width,
	const in vec4 inner_color,
	const in vec4 border_color,
	const in vec4 outer_color)
{
	// Crucially, we need to get the derivative without subracting the border width.
	// Subtracting the border width makes the function non-continuous and makes the jaggies hard to get rid of.
	float fw = fwidth(sd);

	// Compute the two transition points. The inner edge is of course biased by the border amount as the clamping point
	// Treat smoothstep as fancy clamp where e0 < x < e1
	float a = smoothstep(-border_width + fw, -border_width - fw, sd); // inner edge transition
	float b = smoothstep(fw, -fw, sd);                                // outer edge transition

	// Mix the 3 colors with the transition values.
	vec4 color = mix(outer_color, border_color, b);
	color      = mix(color, inner_color, a);
	return color;
}

float SDF_fn(const in uint sdf)
{
	const vec2 p = floor(gl_FragCoord.xy) - sdf_origin.xy;   // Screen-spac distance
	const vec2 hs = sdf_params.xy;                           // Half size
	const float r = sdf_params.z;                            // Radius (for round box, ellipses use half size instead)
	vec2 v;                                                  // Scratch
	float d;                                                 // Distance calculated

	switch (sdf)
	{
	case SDF_ELLIPSE:
		// Slightly inaccurate hack, but good enough for classification and allows oval shapes
		d = length(p / hs) - 1.f;
		// Now we need to correct for the border because the circle was scaled down to a unit
		return d * length(hs);
	case SDF_BOX:
		// Insanity, reduced junction of 3 functions
		// If for each axis the axis-aligned distance = D then you can select/clamp each axis separately by doing a max(D, 0) on all dimensions
		// Length then does the squareroot transformation.
		// The second term is to add back the inner distance which is useful for rendering borders
		v = abs(p) - hs;
		return length(max(v, 0.f)) + min(max(v.x, v.y), 0.0);
	case SDF_ROUND_BOX:
		// Modified BOX SDF.
		// The half box size is shrunk by R in it's diagonal, but we add radius back into the output to bias the output again
		v = abs(p) - (hs - r);
		return length(max(v, 0.f)) + min(max(v.x, v.y), 0.0) - r;
	default:
		return -1.f;
	}
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

	if (config.sdf != SDF_DISABLED)
	{
		const float border_w = sdf_params.w; // Border width
		const float d = SDF_fn(config.sdf);
		diff_color = SDF_blend(d, border_w, diff_color, sdf_border_color, vec4(0.));
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
