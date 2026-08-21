R"(
#define ZS_READ(index, coord) vec2(texture(TEX_NAME(index), coord).r, float(texture(TEX_NAME_STENCIL(index), coord).x))
#define TEX1D_Z24X8_RGBA8(index, coord1) _process_texel(convert_z24x8_to_rgba8(ZS_READ(index, COORD_SCALE1(index, coord1)), TEX_PARAM(index).remap, TEX_FLAGS(index)), TEX_FLAGS(index))
#define TEX2D_Z24X8_RGBA8(index, coord2) _process_texel(convert_z24x8_to_rgba8(ZS_READ(index, COORD_SCALE2(index, coord2)), TEX_PARAM(index).remap, TEX_FLAGS(index)), TEX_FLAGS(index))
#define TEX3D_Z24X8_RGBA8(index, coord3) _process_texel(convert_z24x8_to_rgba8(ZS_READ(index, COORD_SCALE3(index, coord3)), TEX_PARAM(index).remap, TEX_FLAGS(index)), TEX_FLAGS(index))

// NOTE: Memory layout is fetched as byteswapped BGRA [GBAR] (GOW collection, DS2, DeS)
// The A component (Z) is useless (should contain stencil8 or just 1)
vec4 decode_depth24(const in float depth_value, const in bool depth_float)
{
	uint value;
	if (!depth_float)
	{
		value = uint(depth_value * 16777215.);
	}
	else
	{
		value = _get_bits(floatBitsToUint(depth_value), 7, 24);
	}

	uint b = _get_bits(value, 0, 8);
	uint g = _get_bits(value, 8, 8);
	uint r = _get_bits(value, 16, 8);
	const vec4 color = vec4(float(g), float(b) , 1., float(r));
	const vec4 scale = vec4(255., 255., 1., 255.);
	return color / scale;
}

vec4 convert_z24x8_to_rgba8(const in vec2 depth_stencil, const in uint remap, const in uint flags)
{
	vec4 result = decode_depth24(depth_stencil.x, _test_bit(flags, DEPTH_FLOAT));
	result.z = depth_stencil.y / 255.;

	if (remap == 0xAAE4)
		return result;

	return remap_vector(result, remap);
}

)"
