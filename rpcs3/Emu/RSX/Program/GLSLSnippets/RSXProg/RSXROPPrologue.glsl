R"(

#ifdef _ENABLE_POLYGON_STIPPLE
	if (_test_bit(rop_control, POLYGON_STIPPLE_ENABLE_BIT))
	{
		// Convert x,y to linear address
		const uvec2 stipple_coord = uvec2(gl_FragCoord.xy) % uvec2(32, 32);
		const uint address = stipple_coord.y * 32u + stipple_coord.x;
		const uint bit_offset = (address & 31u);
		const uint word_index = _get_bits(address, 7, 3);
		const uint sub_index = _get_bits(address, 5, 2);

		if (!_test_bit(stipple_pattern[word_index][sub_index], int(bit_offset)))
		{
			_kill();
		}
	}
#endif

#ifdef _ENABLE_PROGRAMMABLE_BLENDING
	vec4 mrt_color[4];
	for (int n = 0; n < framebufferCount; ++n)
	{
		mrt_color[n] = subPassLoad(mrtAttachments[n]);
	}
#endif
)"
