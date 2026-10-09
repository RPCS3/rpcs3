R"(
#ifdef _EMULATE_DEPTH_RANGE
	// The vertex stage mapped the clip range onto the [0, 1] viewport and hardware clipping is disabled.
	// Clip or clamp against the real range here, then store the window Z encoded.
	if (!_test_bit(rop_control, DEPTH_CLAMP_ENABLE_BIT) && (gl_FragCoord.z < 0. || gl_FragCoord.z > 1.))
	{
		discard;
	}

#ifdef _ENABLE_DEPTH_EXPORT
	// Hardware tests show exported depth is not clamped to 1 on float depth surfaces
	float emulated_depth = encode_emulated_depth(r1.z);
#else
	float emulated_depth = encode_emulated_depth(depth_range.x + _saturate(gl_FragCoord.z) * (depth_range.y - depth_range.x));
#endif

#ifdef _ENABLE_DEPTH_COMPARE
	// dstDepth is the stored value, already encoded. One step of the 24-bit float format is 64 steps of the encoding.
	if (abs(int(floatBitsToUint(emulated_depth)) - int(floatBitsToUint(dstDepth))) < 64)
	{
		emulated_depth = dstDepth;
	}
#endif

	gl_FragDepth = emulated_depth;
#endif
)"
