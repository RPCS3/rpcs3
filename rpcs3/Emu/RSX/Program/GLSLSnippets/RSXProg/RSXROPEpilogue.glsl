R"(

#ifdef _DISABLE_EARLY_DISCARD
	if (_fragment_discard)
	{
		discard;
	}
#endif

#if defined(_ENABLE_ROP_OUTPUT_ROUNDING)
	MRT0_DO(col0 = round_to_8bit(col0);)
	MRT1_DO(col1 = round_to_8bit(col1);)
	MRT2_DO(col2 = round_to_8bit(col2);)
	MRT3_DO(col3 = round_to_8bit(col3);)
#endif

// Alpha Testing. This stage always runs even without a color output.
#ifdef _ENABLE_ALPHA_TEST
	// TODO: Verify that alpha test actually runs on quantized output!
	// Behavior was inferred from game observations but real tests will be needed here.
#if _MRT_BUFFERS_COUNT >= 1
  #define _alpha_quantize(v) v
#else
  #define _alpha_quantize(v) round_to_8bit(v)
#endif // _MRT_BUFFERS_COUNT
	const uint alpha_func = _get_bits(rop_control, ALPHA_TEST_FUNC_OFFSET, ALPHA_TEST_FUNC_LENGTH);
	if (!comparison_passes(_alpha_quantize(col0.a), alpha_ref, alpha_func))
	{
		discard;
	}
#endif // _ENABLE_ALPHA_TEST

#ifdef _ENABLE_ALPHA_TO_COVERAGE_TEST
	if (!coverage_test_passes(col0))
	{
		discard;
	}
#endif

// ================= Post-output color stages =================
#if _MRT_BUFFERS_COUNT >= 1

// NOTE: Blending happens before output remapping as per hardware tests.
// Sources = raw shader output, Dest = Remapped output from previous draw
#ifdef _ENABLE_PROGRAMMABLE_BLENDING
	MRT0_BLEND_DO(col0 = _mrt_color_t(do_blend(col0, mrt_color0));)
	MRT1_BLEND_DO(col1 = _mrt_color_t(do_blend(col1, mrt_color1));)
	MRT2_BLEND_DO(col2 = _mrt_color_t(do_blend(col2, mrt_color2));)
	MRT3_BLEND_DO(col3 = _mrt_color_t(do_blend(col3, mrt_color3));)
#endif

#ifdef _ENABLE_ROP_CHANNEL_REMAPPING
	MRT0_DO(col0 = _mrt_color_t(remap_ROP_output(col0, ROP_remap));)
	MRT1_DO(col1 = _mrt_color_t(remap_ROP_output(col1, ROP_remap));)
	MRT2_DO(col2 = _mrt_color_t(remap_ROP_output(col2, ROP_remap));)
	MRT3_DO(col3 = _mrt_color_t(remap_ROP_output(col3, ROP_remap));)
#endif // _ENABLE_ROP_CHANNEL_REMAPPING

// TODO: Check order on real hw. Does this happen before quantization or after?
#ifdef _ENABLE_FRAMEBUFFER_SRGB
	MRT0_DO(col0.rgb = _mrt_color_t(linear_to_srgb(col0)).rgb;)
	MRT1_DO(col1.rgb = _mrt_color_t(linear_to_srgb(col1)).rgb;)
	MRT2_DO(col2.rgb = _mrt_color_t(linear_to_srgb(col2)).rgb;)
	MRT3_DO(col3.rgb = _mrt_color_t(linear_to_srgb(col3)).rgb;)
#endif

	// Commit COLOR aspect
	MRT0_DO(ocol0 = col0;)
	MRT1_DO(ocol1 = col1;)
	MRT2_DO(ocol2 = col2;)
	MRT3_DO(ocol3 = col3;)

#endif // _MRT_BUFFERS_COUNT >= 1

//// ====================== Depth Export ===========================

#ifdef _ENABLE_DEPTH_COMPARE
#ifdef _ENABLE_ROP_OUTPUT_MULTISAMPLED
	float dstDepth = texelFetch(frag_depth, ivec2(gl_FragCoord.xy), gl_SampleID).r;
#else
	float dstDepth = texelFetch(frag_depth, ivec2(gl_FragCoord.xy), 0).r;
#endif // _ENABLE_ROP_OUTPUT_MULTISAMPLED
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
#endif // _ENABLE_DEPTH_COMPARE
)"
