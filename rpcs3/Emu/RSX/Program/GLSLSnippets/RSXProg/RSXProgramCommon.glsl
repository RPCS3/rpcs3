R"(
#define _select mix
#define _saturate(x) clamp(x, 0., 1.)
#define _get_bits(x, off, count) bitfieldExtract(x, off, count)
#define _set_bits(x, y, off, count) bitfieldInsert(x, y, off, count)
#define _test_bit(x, y) (_get_bits(x, y, 1) != 0)
#define _rand(seed) fract(sin(dot(seed.xy, vec2(12.9898f, 78.233f))) * 43758.5453f)

#ifdef _GPU_LOW_PRECISION_COMPARE
#define CMP_FIXUP(a) (sign(a) * 16. + a)
#else
#define CMP_FIXUP(a) (a)
#endif

#ifdef _ENABLE_LIT_EMULATION
vec4 lit_legacy(const in vec4 val)
{
	vec4 clamped_val = vec4(max(val.xy, vec2(0.)), val.zw);
	return vec4(
		1.,
		clamped_val.x,
		exp2(clamped_val.w * log2(max(clamped_val.y, 0.0000000001))) * sign(clamped_val.x),
		1.);
}
#endif

)"
