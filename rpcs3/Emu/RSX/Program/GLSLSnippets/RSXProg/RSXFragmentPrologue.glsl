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
	const uvec4 raw = uvec4(max(floor(fma(_fx12_truncate(v4), vec4(255.), vec4(0.5))), vec4(0.)));
	return vec4(raw) / vec4(255.);
}
#ifndef _32_BIT_OUTPUT
f16vec4 round_to_8bit(const in f16vec4 v4)
{
	const uvec4 raw = uvec4(max(floor(fma(_fx12_truncate(vec4(v4)), f16vec4(255.), f16vec4(0.5))), vec4(0.)));
	return f16vec4(raw) / f16vec4(255.);
}
#endif

// Scalar variant of 8-bit rounding is only available for f32.
float round_to_8bit(const in float v)
{
	const uint raw = uint(max(floor(fma(_fx12_truncate(v), 255.f, 0.5f)), 0.f));
	return float(raw) / 255.f;
}

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
	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(wpos_bias, 0., 0.);
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

#ifdef _ENABLE_ALPHA_TO_COVERAGE_TEST
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

#ifdef _ENABLE_COMPARISON_FUNC
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
#endif

#ifdef _ENABLE_COLOR_CHANNEL_REMAPPING
vec4 remap_vector(const in vec4 color, const in uint remap)
{
	vec4 result;
	if (_get_bits(remap, 0, 8) == 0xE4)
	{
		result = color;
	}
	else
	{
		uvec4 remap_channel = uvec4(remap) >> uvec4(2, 4, 6, 0);
		remap_channel &= 3;
		remap_channel = (remap_channel + 3) % 4; // Map A-R-G-B to R-G-B-A

		// Generate remapped result
		result.a = color[remap_channel.a];
		result.r = color[remap_channel.r];
		result.g = color[remap_channel.g];
		result.b = color[remap_channel.b];
	}

	if (_get_bits(remap, 8, 8) == 0xAA)
		return result;

	uvec4 remap_select = uvec4(remap) >> uvec4(10, 12, 14, 8);
	remap_select &= 3;
	bvec4 choice = lessThan(remap_select, uvec4(2));
	return _select(result, vec4(remap_select), choice);
}
#endif

#ifdef _ENABLE_ROP_CHANNEL_REMAPPING
#define get_ROP_channel_remap() _get_bits(rop_control, MRT_CHANNEL_REMAP_OFFSET, MRT_CHANNEL_REMAP_LENGTH)
vec4 remap_ROP_output(const in vec4 col, const in uint remap_index)
{
	switch (remap_index)
	{
	default:
	case ROP_REMAP_SWIZZLE_RGBA: // RGBA
		return col;
	case ROP_REMAP_SWIZZLE_BBBB: // B8
		return col.bbbb;
	case ROP_REMAP_SWIZZLE_GBGB: // G8B8
		return col.bgbg;
	case ROP_REMAP_SWIZZLE_RGB1: // RGB1
		return vec4(col.rgb, 1.f);
	case ROP_REMAP_SWIZZLE_RGB0: // RGB0
		return vec4(col.rgb, 0.f);
	}
}
vec4 remap_ROP_input(const in vec4 col, const in uint remap_index)
{
	switch (remap_index)
	{
	default:
	case ROP_REMAP_SWIZZLE_RGBA: // RGBA
		return col;
	case ROP_REMAP_SWIZZLE_BBBB: // B8
		return col.rrrr;
	case ROP_REMAP_SWIZZLE_GBGB: // G8B8
		return col.rgrg;
	case ROP_REMAP_SWIZZLE_RGB1: // RGB1
		return vec4(col.rgb, 1.f);
	case ROP_REMAP_SWIZZLE_RGB0: // RGB0
		return vec4(col.rgb, 0.f);
	}
}
#endif

)"
