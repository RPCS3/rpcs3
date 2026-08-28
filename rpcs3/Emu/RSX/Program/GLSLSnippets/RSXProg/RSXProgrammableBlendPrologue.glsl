R"(

/**
 * Required register definitions from ROP config
 struct {
 	vec4 blend_constants;    // fp32x4
	uint blend_eqn;          // Packed RGB in lower 16 bits, A in upper 16 bits
	uint blend_sfactors;     // Ditto
	uint blend_dfactors;     // Ditto
 }
*/

#define BLEND_FACTOR_ZERO 0
#define BLEND_FACTOR_ONE  1
#define BLEND_FACTOR_SRC_COLOR 0x0300
#define BLEND_FACTOR_ONE_MINUS_SRC_COLOR 0x0301
#define BLEND_FACTOR_SRC_ALPHA 0x0302
#define BLEND_FACTOR_ONE_MINUS_SRC_ALPHA 0x0303
#define BLEND_FACTOR_DST_ALPHA 0x0304
#define BLEND_FACTOR_ONE_MINUS_DST_ALPHA 0x0305
#define BLEND_FACTOR_DST_COLOR 0x0306
#define BLEND_FACTOR_ONE_MINUS_DST_COLOR 0x0307
#define BLEND_FACTOR_SRC_ALPHA_SATURATE 0x0308
#define BLEND_FACTOR_CONSTANT_COLOR 0x8001
#define BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR 0x8002
#define BLEND_FACTOR_CONSTANT_ALPHA 0x8003
#define BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA 0x8004

#define BLEND_FUNC_ADD 0x8006
#define BLEND_MIN 0x8007
#define BLEND_MAX 0x8008
#define BLEND_FUNC_SUBTRACT 0x800A
#define BLEND_FUNC_REVERSE_SUBTRACT 0x800B
#define BLEND_FUNC_REVERSE_SUBTRACT_SIGNED 0x0000F005
#define BLEND_FUNC_ADD_SIGNED 0x0000F006
#define BLEND_FUNC_REVERSE_ADD_SIGNED 0x0000F007

#ifdef INT_FRAMEBUFFER_BIT
#define _blend_saturate _saturate
#define _blend_saturate_signed(v) clamp(v, -1.f, 1.f)
#else
#define _blend_saturate(v) v
#define _blend_saturate_signed(v) v
#endif

float get_blend_factor_a(const in uint op, const in vec4 src, const in vec4 dst)
{
	switch (op)
	{
	case BLEND_FACTOR_ZERO: return 0.;
	case BLEND_FACTOR_ONE: return 1.;
	case BLEND_FACTOR_SRC_COLOR:
	case BLEND_FACTOR_SRC_ALPHA: return src.a;
	case BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
	case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return 1. - src.a;
	case BLEND_FACTOR_DST_ALPHA:
	case BLEND_FACTOR_DST_COLOR: return dst.a;
	case BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
	case BLEND_FACTOR_ONE_MINUS_DST_COLOR: return 1. - dst.a;
	case BLEND_FACTOR_SRC_ALPHA_SATURATE: return 1;
	case BLEND_FACTOR_CONSTANT_COLOR:
	case BLEND_FACTOR_CONSTANT_ALPHA: return blend_constants.a;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return 1. - blend_constants.a;
	}
	return 0.;
}

vec3 get_blend_factor_rgb(const in uint op, const in vec4 src, const in vec4 dst)
{
	switch (op)
	{
	case BLEND_FACTOR_ZERO: return vec3(0.);
	case BLEND_FACTOR_ONE: return vec3(1.);
	case BLEND_FACTOR_SRC_COLOR: return src.rgb;
	case BLEND_FACTOR_SRC_ALPHA: return src.aaa;
	case BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return 1. - src.rgb;
	case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return 1. - src.aaa;
	case BLEND_FACTOR_DST_COLOR: return dst.rgb;
	case BLEND_FACTOR_DST_ALPHA: return dst.aaa;
	case BLEND_FACTOR_ONE_MINUS_DST_COLOR: return 1. - dst.rgb;
	case BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return 1. - dst.aaa;
	case BLEND_FACTOR_SRC_ALPHA_SATURATE: return vec3(min(src.a, 1. - dst.a));
	case BLEND_FACTOR_CONSTANT_COLOR: return blend_constants.rgb;
	case BLEND_FACTOR_CONSTANT_ALPHA: return blend_constants.aaa;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return 1. - blend_constants.rgb;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return 1. - blend_constants.aaa;
	}
	return vec3(0.);
}

float apply_blend_func_a(const in vec4 src, const in vec4 dst)
{
	uint blend_factor_a_s = _get_bits(blend_sfactors, 16, 16);
	uint blend_factor_a_d = _get_bits(blend_dfactors, 16, 16);
	uint func = _get_bits(blend_eqn, 16, 16);

	const float src_factor_a = get_blend_factor_a(blend_factor_a_s, src, dst);
	const float dst_factor_a = get_blend_factor_a(blend_factor_a_d, src, dst);

	// NOTE: Destination data is already saturated due to encoding.
	const float s = src.a * src_factor_a;
	const float d = dst.a * dst_factor_a;

	switch (func)
	{
	case BLEND_FUNC_ADD: return _blend_saturate(s) + d;
	case BLEND_MIN: return min(_blend_saturate(src.a), dst.a);
	case BLEND_MAX: return max(_blend_saturate(src.a), dst.a);
	case BLEND_FUNC_SUBTRACT: return _blend_saturate(s) - d;
	case BLEND_FUNC_REVERSE_SUBTRACT: return d - _blend_saturate(s);
	case BLEND_FUNC_REVERSE_SUBTRACT_SIGNED: return d - _blend_saturate_signed(s);
	case BLEND_FUNC_ADD_SIGNED: return _blend_saturate_signed(s) + d;
	case BLEND_FUNC_REVERSE_ADD_SIGNED: return _blend_saturate(s) + _blend_saturate_signed(d);
	}

	return 0.f;
}

vec3 apply_blend_func_rgb(const in vec4 src, const in vec4 dst)
{
	uint blend_factor_rgb_s = _get_bits(blend_sfactors, 0, 16);
	uint blend_factor_rgb_d = _get_bits(blend_dfactors, 0, 16);
	uint func = _get_bits(blend_eqn, 0, 16);

	const vec3 src_factor_rgb = get_blend_factor_rgb(blend_factor_rgb_s, src, dst);
	const vec3 dst_factor_rgb = get_blend_factor_rgb(blend_factor_rgb_d, src, dst);

	// NOTE: Destination data is already saturated due to encoding.
	const vec3 s = src.rgb * src_factor_rgb;
	const vec3 d = dst.rgb * dst_factor_rgb;

	switch (func)
	{
	case BLEND_FUNC_ADD: return _blend_saturate(s) + d;
	case BLEND_MIN: return min(_blend_saturate(src.rgb), dst.rgb);
	case BLEND_MAX: return max(_blend_saturate(src.rgb), dst.rgb);
	case BLEND_FUNC_SUBTRACT: return _blend_saturate(s) - d;
	case BLEND_FUNC_REVERSE_SUBTRACT: return d - _blend_saturate(s);
	case BLEND_FUNC_REVERSE_SUBTRACT_SIGNED: return d - _blend_saturate_signed(s);
	case BLEND_FUNC_ADD_SIGNED: return _blend_saturate_signed(s) + d;
	case BLEND_FUNC_REVERSE_ADD_SIGNED: return _blend_saturate(s) + _blend_saturate_signed(d);
	}

	return vec3(0.);
}

vec4 do_blend(const in vec4 src, const in vec4 dst)
{
	// Read blend_constants from config and apply blend op
	const vec4 result = vec4(
		apply_blend_func_rgb(src, dst),
		apply_blend_func_a(src, dst)
	);

#ifdef _ENABLE_ROP_OUTPUT_ROUNDING
	// Accurate int conversion with wrapping
	return round_to_8bit(result);
#else
	return result;
#endif
}

)"
