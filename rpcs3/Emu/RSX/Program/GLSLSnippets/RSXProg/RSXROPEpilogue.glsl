R"(

#ifdef _DISABLE_EARLY_DISCARD
	if (_fragment_discard)
	{
		discard;
	}
#endif

// Alpha Testing. This stage always runs even without a color output.
#ifdef _ENABLE_ALPHA_TEST
// TODO: Verify that alpha test actually runs on quantized output!
// Behavior was inferred from game observations but real tests will be needed here.
#if defined(_ENABLE_ROP_OUTPUT_ROUNDING)
#define _alpha_quantize(v) round_to_8bit(v)
#else
#define _alpha_quantize(v) (v)
#endif
	const uint alpha_func = _get_bits(rop_control, ALPHA_TEST_FUNC_OFFSET, ALPHA_TEST_FUNC_LENGTH);
	if (!comparison_passes(_alpha_quantize(col0.a), alpha_ref, alpha_func))
	{
		discard;
	}
#endif

#ifdef _ENABLE_ALPHA_TO_COVERAGE_TEST
	if (!coverage_test_passes(col0))
	{
		discard;
	}
#endif

// ================= Post-output color stages =================

#if _MRT_BUFFERS_COUNT >= 1

#ifdef _ENABLE_PROGRAMMABLE_BLENDING
	// Stash the raw source data
	vec4 blend_s0 = col0;
	vec4 blend_s1 = col1;
	vec4 blend_s2 = col2;
	vec4 blend_s3 = col3;
#endif

#if defined(_ENABLE_ROP_OUTPUT_ROUNDING) && !defined(_ENABLE_PROGRAMMABLE_BLENDING)
	col0 = round_to_8bit(col0);
	col1 = round_to_8bit(col1);
	col2 = round_to_8bit(col2);
	col3 = round_to_8bit(col3);
#endif

// NOTE: Blending happens before output remapping as per hardware tests.
// Sources = raw shader output, Dest = Remapped output from previous draw
#ifdef _ENABLE_PROGRAMMABLE_BLENDING
#if _MRT_BUFFERS_COUNT >= 1
	col0 = _mrt_color_t(do_blend(blend_s0, mrt_color[0]));
#endif

#if _MRT_BUFFERS_COUNT >= 2
	col1 = _mrt_color_t(do_blend(blend_s1, mrt_color[1]));
#endif

#if _MRT_BUFFERS_COUNT >= 3
	col2 = _mrt_color_t(do_blend(blend_s2, mrt_color[2]));
#endif

#if _MRT_BUFFERS_COUNT == 4
	col3 = _mrt_color_t(do_blend(blend_s3, mrt_color[3]));
#endif
#endif

#ifdef _ENABLE_ROP_CHANNEL_REMAPPING
	const uint ROP_remap = get_ROP_channel_remap();
	col0 = _mrt_color_t(remap_ROP_output(col0, ROP_remap));
	col1 = _mrt_color_t(remap_ROP_output(col1, ROP_remap));
	col2 = _mrt_color_t(remap_ROP_output(col2, ROP_remap));
	col3 = _mrt_color_t(remap_ROP_output(col3, ROP_remap));
#endif

// TODO: Check order on real hw. Does this happen before quantization or after?
#ifdef _ENABLE_FRAMEBUFFER_SRGB
	col0.rgb = _mrt_color_t(linear_to_srgb(col0)).rgb;
	col1.rgb = _mrt_color_t(linear_to_srgb(col1)).rgb;
	col2.rgb = _mrt_color_t(linear_to_srgb(col2)).rgb;
	col3.rgb = _mrt_color_t(linear_to_srgb(col3)).rgb;
#endif

	// Commit COLOR aspect
	ocol0 = col0;
	ocol1 = col1;
	ocol2 = col2;
	ocol3 = col3;

#endif // _MRT_BUFFERS_COUNT >= 1

//// ====================== Depth Export ===========================

#ifdef _ENABLE_DEPTH_COMPARE
#ifdef _ENABLE_DEPTH_BUFFER_MULTISAMPLED
	float dstDepth = texelFetch(frag_depth, ivec2(gl_FragCoord.xy), gl_SampleID).r;
#else
	float dstDepth = texelFetch(frag_depth, ivec2(gl_FragCoord.xy), 0).r;
#endif
	float srcDepth = gl_FragCoord.z;
	float scale = _test_bit(rop_control, FRAG_DEPTH_24_BIT) ? float(0xffffffu) : float(0xffffu);

	// Quantize and compare
	if (abs(srcDepth - dstDepth) < (1.f / scale))
	{
		gl_FragDepth = dstDepth;
	}
	else
	{
		gl_FragDepth = srcDepth;
	}
#endif
)"
