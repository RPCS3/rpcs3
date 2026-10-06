R"(
// Some RSX data such as the D16_FLOAT format data uses E4M12 instead of the standard FP16.
// Unsigned, exponent bias 16: value = 2^(E-16) * (1 + M/4096), so the format covers [0, 0.99987793] with most of its precision near 1.
// Hardware stores 0.5 as 0xF000 and saturates 1.0 and above to 0xFFFF; Z > 1 is not representable.
#if defined(_CONVERT_F32_TO_E4M12) && _CONVERT_F32_TO_E4M12
// Clamp to [0, 0.99987793] (0xFFFF), rebias by 2^-111, then round the 11 dropped mantissa bits to nearest.
uint pack_e4m12_pack16(const in uvec2 value)
{
	const vec2 f = clamp(uintBitsToFloat(value), 0., uintBitsToFloat(0x3F7FF800u)) * uintBitsToFloat(0x08000000u);
	const uvec2 result = (floatBitsToUint(f) + 0x400u) >> 11;
	return bitfieldInsert(result.x, result.y, 16, 16);
}
#else
// Move each half's E4M12 bits onto the low end of the f32 exponent/mantissa, then rebias by 2^111.
// Rebiasing with a float multiply keeps 0 as 0 and decodes E=0 as a denormal (m * 2^-27).
uvec2 unpack_e4m12_pack16(const in uint value)
{
	const uvec2 bits = uvec2(value << 11u, value >> 5u) & 0x07FFF800u;             // 5 = 16 - 11. We'd have shifted down by 16 then up by 11 normally to extract the high word...
	return floatBitsToUint(uintBitsToFloat(bits) * uintBitsToFloat(0x77000000u));  // Exponent addition using mul to preserve 0. Otherwise special numbers like 0 would get corrupted if you add (111 << 23) like the old impl.
}
#endif
)"
