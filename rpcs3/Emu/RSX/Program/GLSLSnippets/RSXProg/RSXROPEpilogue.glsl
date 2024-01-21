R"(

#ifdef _DISABLE_EARLY_DISCARD
	if (_fragment_discard)
	{
		discard;
	}
#endif

#ifdef _ENABLE_FRAMEBUFFER_SRGB
	if (_test_bit(rop_control, SRGB_FRAMEBUFFER_BIT))
	{
		col0.rgb = _mrt_color_t(linear_to_srgb(col0)).rgb;
		col1.rgb = _mrt_color_t(linear_to_srgb(col1)).rgb;
		col2.rgb = _mrt_color_t(linear_to_srgb(col2)).rgb;
		col3.rgb = _mrt_color_t(linear_to_srgb(col3)).rgb;
	}
#endif

#ifdef _ENABLE_ROP_OUTPUT_ROUNDING
	if (_test_bit(rop_control, INT_FRAMEBUFFER_BIT))
	{
		col0 = round_to_8bit(col0);
		col1 = round_to_8bit(col1);
		col2 = round_to_8bit(col2);
		col3 = round_to_8bit(col3);
	}
#endif

	// Post-output stages
	// Alpha Testing
	if (_test_bit(rop_control, ALPHA_TEST_ENABLE_BIT))
	{
		const uint alpha_func = _get_bits(rop_control, ALPHA_TEST_FUNC_OFFSET, ALPHA_TEST_FUNC_LENGTH);
		if (!comparison_passes(col0.a, alpha_ref, alpha_func))
		{
			discard;
		}
	}

#ifdef _EMULATE_COVERAGE_TEST
	if (_test_bit(rop_control, ALPHA_TO_COVERAGE_ENABLE_BIT))
	{
		if (!_test_bit(rop_control, MSAA_WRITE_ENABLE_BIT) || !coverage_test_passes(col0))
		{
			discard;
		}
	}
#endif

#ifdef _ENABLE_PROGRAMMABLE_BLENDING
	switch (framebufferCount)
	{
		case 4:
			col3 = do_blend(col3, mrt_color[3]);
			// Fallthrough
		case 3:
			col2 = do_blend(col2, mrt_color[2]);
			// Fallthrough
		case 2:
			col1 = do_blend(col1, mrt_color[1]);
			// Fallthrough
		default:
			col0 = do_blend(col0, mrt_color[0]);
			break;
	}
#endif

	// Commit
	ocol0 = col0;
	ocol1 = col1;
	ocol2 = col2;
	ocol3 = col3;
)"
