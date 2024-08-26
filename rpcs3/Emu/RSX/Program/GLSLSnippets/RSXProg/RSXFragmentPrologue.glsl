R"(

#ifdef _32_BIT_OUTPUT
// Everything is fp32 on ouput channels
#define _mrt_color_t(expr) expr
#else
// Mixed types. We have fp16 outputs
#define _mrt_color_t f16vec4
#endif

#if defined(_ENABLE_ROP_OUTPUT_ROUNDING) || defined(_ENABLE_PROGRAMMABLE_BLENDING)
// Truncate float by discarding lower 12-bits of the mantissa
#define _fx12_truncate(x) uintBitsToFloat(floatBitsToUint(x) & 0xfffff000u)

// Default. Used when we're not utilizing native fp16
vec4 round_to_8bit(const in vec4 v4)
{
	uvec4 raw = uvec4(floor(fma(_fx12_truncate(v4), vec4(255.), vec4(0.5))));
	return vec4(raw) / vec4(255.);
}
#ifndef _32_BIT_OUTPUT
f16vec4 round_to_8bit(const in f16vec4 v4)
{
	uvec4 raw = uvec4(floor(fma(_fx12_truncate(vec4(v4)), f16vec4(255.), f16vec4(0.5))));
	return f16vec4(raw) / f16vec4(255.);
}
#endif
#endif

#ifdef _DISABLE_EARLY_DISCARD
bool _fragment_discard = false;
#define _kill() _fragment_discard = true
#else
#define _kill() discard
#endif

#ifdef _ENABLE_WPOS
vec4 get_wpos()
{
	float abs_scale = abs(wpos_scale);
	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(0., wpos_bias, 0., 0.);
}
#endif

#ifdef _ENABLE_FOG_READ
vec4 fetch_fog_value(const in uint mode)
{
	vec4 result = vec4(fog_c.x, 0., 0., 0.);
	switch(mode)
	{
	default:
		return result;
	case FOG_LINEAR:
		// linear
		result.y = fog_param1 * fog_c.x + (fog_param0 - 1.);
		break;
	case FOG_EXP:
		// exponential
		result.y = exp(11.084 * (fog_param1 * fog_c.x + fog_param0 - 1.5));
		break;
	case FOG_EXP2:
		// exponential2
		result.y = exp(-pow(4.709 * (fog_param1 * fog_c.x + fog_param0 - 1.5), 2.));
		break;
	case FOG_EXP_ABS:
		// exponential_abs
		result.y = exp(11.084 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5));
		break;
	case FOG_EXP2_ABS:
		// exponential2_abs
		result.y = exp(-pow(4.709 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5), 2.));
		break;
	case FOG_LINEAR_ABS:
		// linear_abs
		result.y = fog_param1 * abs(fog_c.x) + (fog_param0 - 1.);
		break;
	}

	result.y = clamp(result.y, 0., 1.);
	return result;
}
#endif

#ifdef _EMULATE_COVERAGE_TEST
// Purely stochastic
bool coverage_test_passes(const in vec4 _sample)
{
	float random_val = _rand(gl_FragCoord);
	return (_sample.a > random_val);
}
#endif

#ifdef _ENABLE_LINEAR_TO_SRGB
vec4 linear_to_srgb(const in vec4 cl)
{
	vec4 low = cl * 12.92;
	vec4 high = 1.055 * pow(cl, vec4(1. / 2.4)) - 0.055;
	bvec4 selection = lessThan(cl, vec4(0.0031308));
	return clamp(mix(high, low, selection), 0., 1.);
}
#endif

#ifdef _ENABLE_SRGB_TO_LINEAR
vec4 srgb_to_linear(const in vec4 cs)
{
	vec4 a = cs / 12.92;
	vec4 b = pow((cs + 0.055) / 1.055, vec4(2.4));
	return _select(a, b, greaterThan(cs, vec4(0.04045)));
}
#endif

// Required by all fragment shaders for alpha test
bool comparison_passes(const in float a, const in float b, const in uint func)
{
	switch (func)
	{
		default:
		case 0: return false; //never
		case 1: return (CMP_FIXUP(a) < CMP_FIXUP(b)); //less
		case 2: return (CMP_FIXUP(a) == CMP_FIXUP(b)); //equal
		case 3: return (CMP_FIXUP(a) <= CMP_FIXUP(b)); //lequal
		case 4: return (CMP_FIXUP(a) > CMP_FIXUP(b)); //greater
		case 5: return (CMP_FIXUP(a) != CMP_FIXUP(b)); //nequal
		case 6: return (CMP_FIXUP(a) >= CMP_FIXUP(b)); //gequal
		case 7: return true; //always
	}
}

)"
