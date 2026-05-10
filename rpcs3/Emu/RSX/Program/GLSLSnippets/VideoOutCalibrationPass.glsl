R"(
#version 440

#define SAMPLER_BINDING(x) %sampler_binding

layout(%set_decorator, binding=SAMPLER_BINDING(0)) uniform sampler2D fs0;
layout(%set_decorator, binding=SAMPLER_BINDING(1)) uniform sampler2D fs1;

layout(location=0) in vec2 tc0;
layout(location=0) out vec4 ocol;

#define STEREO_MODE_DISABLED 0
#define STEREO_MODE_SIDE_BY_SIDE 1
#define STEREO_MODE_OVER_UNDER 2
#define STEREO_MODE_INTERLACED 3
#define STEREO_MODE_ANAGLYPH_RED_GREEN 4
#define STEREO_MODE_ANAGLYPH_RED_BLUE 5
#define STEREO_MODE_ANAGLYPH_RED_CYAN 6
#define STEREO_MODE_ANAGLYPH_MAGENTA_CYAN 7
#define STEREO_MODE_ANAGLYPH_TRIOSCOPIC 8
#define STEREO_MODE_ANAGLYPH_AMBER_BLUE 9
#define STEREO_MODE_ANAGLYPH_CUSTOM 10

#define TRUE 1
#define FALSE 0

const vec2 left_single_matrix  = vec2(1.f, 0.4898f);
const vec2 right_single_matrix = vec2(0.f, 0.510204f);
const vec2 sbs_single_matrix   = vec2(2.0, 0.4898f);
const vec2 sbs_multi_matrix    = vec2(2.0, 1.0);
const vec2 ou_single_matrix    = vec2(1.0, 0.9796f);
const vec2 ou_multi_matrix     = vec2(1.0, 2.0);

#ifdef VULKAN
layout(push_constant) uniform static_data
{
	float gamma;
	int limit_range;
	int stereo_display_mode;
	int stereo_image_count;
	mat3 left_anaglyph_matrix;
	mat3 right_anaglyph_matrix;
};
#else
uniform float gamma;
uniform int limit_range;
uniform int stereo_display_mode;
uniform int stereo_image_count;
uniform mat3 left_anaglyph_matrix;
uniform mat3 right_anaglyph_matrix;
#endif

vec3 applyMatrix(vec3 left, vec3 right)
{
	vec3 outColor = left_anaglyph_matrix * left + right_anaglyph_matrix * right;
	return clamp(outColor, 0.0, 1.0);
}

vec4 anaglyph(const in vec4 left, const in vec4 right)
{
	vec3 color = applyMatrix(left.rgb, right.rgb);
	return vec4(color, 1.0);
}

vec4 anaglyph_single_image()
{
	const vec4 left  = texture(fs0, tc0 * left_single_matrix);
	const vec4 right = texture(fs0, (tc0 * left_single_matrix) + right_single_matrix);

	return anaglyph(left, right);
}

vec4 anaglyph_stereo_image()
{
	const vec4 left = texture(fs0, tc0);
	const vec4 right = texture(fs1, tc0);
	
	return anaglyph(left, right);
}

vec4 read_source()
{
	if (stereo_display_mode == STEREO_MODE_DISABLED)
	{
		return texture(fs0, tc0);
	}

	if (stereo_image_count == 1)
	{
		switch (stereo_display_mode)
		{
			case STEREO_MODE_ANAGLYPH_RED_GREEN:
			case STEREO_MODE_ANAGLYPH_RED_BLUE:
			case STEREO_MODE_ANAGLYPH_RED_CYAN:
			case STEREO_MODE_ANAGLYPH_MAGENTA_CYAN:
			case STEREO_MODE_ANAGLYPH_TRIOSCOPIC:
			case STEREO_MODE_ANAGLYPH_AMBER_BLUE:
			case STEREO_MODE_ANAGLYPH_CUSTOM:
				return anaglyph_single_image();
			case STEREO_MODE_SIDE_BY_SIDE:
				return (tc0.x < 0.5)
					? texture(fs0, tc0 * sbs_single_matrix)
					: texture(fs0, (tc0 * sbs_single_matrix) + vec2(-1.f, 0.510204f));
			case STEREO_MODE_OVER_UNDER:
				return (tc0.y < 0.5)
					? texture(fs0, tc0 * ou_single_matrix)
					: texture(fs0, (tc0 * ou_single_matrix) + vec2(0.f, 0.020408f));
			case STEREO_MODE_INTERLACED:
				return ((int(gl_FragCoord.y) & 1) > 0)
					? texture(fs0, tc0 * left_single_matrix)
					: texture(fs0, (tc0 * left_single_matrix) + right_single_matrix);
			default: // undefined behavior
				return texture(fs0, tc0);
		}
	}

	if (stereo_image_count == 2)
	{
		switch (stereo_display_mode)
		{
			case STEREO_MODE_ANAGLYPH_RED_GREEN:
			case STEREO_MODE_ANAGLYPH_RED_BLUE:
			case STEREO_MODE_ANAGLYPH_RED_CYAN:
			case STEREO_MODE_ANAGLYPH_MAGENTA_CYAN:
			case STEREO_MODE_ANAGLYPH_TRIOSCOPIC:
			case STEREO_MODE_ANAGLYPH_AMBER_BLUE:
			case STEREO_MODE_ANAGLYPH_CUSTOM:
				return anaglyph_stereo_image();
			case STEREO_MODE_SIDE_BY_SIDE:
				return (tc0.x < 0.5)
					? texture(fs0, (tc0 * sbs_multi_matrix))
					: texture(fs1, (tc0 * sbs_multi_matrix) + vec2(-1.f, 0.f));
			case STEREO_MODE_OVER_UNDER:
				return (tc0.y < 0.5)
					? texture(fs0, (tc0 * ou_multi_matrix))
					: texture(fs1, (tc0 * ou_multi_matrix) + vec2(0.f, -1.f));
			case STEREO_MODE_INTERLACED:
				return ((int(gl_FragCoord.y) & 1) > 0)
					? texture(fs0, tc0)
					: texture(fs1, tc0);
			default: // undefined behavior
				return texture(fs0, tc0);
		}
	}

	// Unreachable. Return debug color fill
	return vec4(1., 0., 0., 1.);
}

void main()
{
	vec4 color = read_source();
	color.rgb = pow(color.rgb, vec3(gamma));
	ocol = (limit_range == FALSE)
		? color
		: ((color * 220.) + 16.) / 255.;
	ocol.a = 1.f;
}
)"
