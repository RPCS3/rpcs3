R"(

#ifdef _32_BIT_OUTPUT
// Default. Used when we're not utilizing native fp16
#define round_to_8bit(v4) (floor(fma(v4, vec4(255.), vec4(0.5))) / vec4(255.))
#else
// FP16 version
#define round_to_8bit(v4) (floor(fma(v4, f16vec4(255.), f16vec4(0.5))) / f16vec4(255.))
#endif

#ifdef _DISABLE_EARLY_DISCARD
#define kill() _fragment_discard = true
#else
#define kill() discard
#endif

#ifdef _ENABLE_WPOS
vec4 get_wpos()
{
	float abs_scale = abs(wpos_scale);
	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(0., wpos_bias, 0., 0.);
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
