R"(

#ifdef _ENABLE_POLYGON_STIPPLE
	// Convert x,y to linear address
	const uvec2 stipple_coord = uvec2(gl_FragCoord.xy) % uvec2(32, 32);
	const uint address = stipple_coord.y * 32u + stipple_coord.x;
	const uint bit_offset = (address & 31u);
#ifdef VULKAN
	// In vulkan we have a unified array with a dynamic offset
	const uint word_index = _get_bits(address, 7, 3) + _fs_stipple_pattern_array_offset;
#else
	const uint word_index = _get_bits(address, 7, 3);
#endif
	const uint sub_index = _get_bits(address, 5, 2);

	if (!_test_bit(stipple_pattern[word_index][sub_index], int(bit_offset)))
	{
		_kill();
	}
#endif

#if defined(_ENABLE_PROGRAMMABLE_BLENDING) && _MRT_BUFFERS_COUNT >= 1
	// TODO: Multisampled output support.
	vec4 mrt_color[_MRT_BUFFERS_COUNT];
	for (int n = 0; n < _MRT_BUFFERS_COUNT; ++n)
	{
		mrt_color[n] = subpassLoad(mrtAttachments[n]);
	}
#endif
)"
