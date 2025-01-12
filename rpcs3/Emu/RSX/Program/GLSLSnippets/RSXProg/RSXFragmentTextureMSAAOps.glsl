R"(
#define ZCOMPARE_FUNC(index) _get_bits(TEX_FLAGS(index), DEPTH_COMPARE, 3)
#define ZS_READ_MS(index, coord) vec2(sampleTexture2DMS(TEX_NAME(index), coord, index).r, float(sampleTexture2DMS(TEX_NAME_STENCIL(index), coord, index).x))
#define TEX2D_MS(index, coord2) _process_texel(sampleTexture2DMS(TEX_NAME(index), coord2, index), TEX_FLAGS(index))
#define TEX2D_SHADOW_MS(index, coord3) vec4(comparison_passes(sampleTexture2DMS(TEX_NAME(index), coord3.xy, index).x, coord3.z, ZCOMPARE_FUNC(index)))
#define TEX2D_SHADOWPROJ_MS(index, coord4) TEX2D_SHADOW_MS(index, (coord4.xyz / coord4.w))
#define TEX2D_Z24X8_RGBA8_MS(index, coord2) _process_texel(convert_z24x8_to_rgba8(ZS_READ_MS(index, coord2), texture_parameters[index].remap, TEX_FLAGS(index)), TEX_FLAGS(index))

vec3 compute2x2DownsampleWeights(const in float coord, const in float uv_step, const in float actual_step)
{
	const float next_sample_point = coord + actual_step;
	const float next_coord_step = fma(floor(coord / uv_step), uv_step, uv_step);
	const float next_coord_step_plus_one = next_coord_step + uv_step;

	// We calculate the weights by getting the distances of our sample points from the 'texel centers' and scaling by the actual step
	// However, since our weights must add up to exactly 1.0, we can skip one term and just get the remainder for a more accurate result
	// Let's allot the overflow to the original texel in this case (a)
	// const float a0 = next_coord_step;
	const float b0 = min(next_coord_step_plus_one, next_sample_point);
	const float c0 = max(next_coord_step_plus_one, next_sample_point);

	// const float a1 = coord;
	const float b1 = next_coord_step;
	const float c1 = next_coord_step_plus_one;

	const vec2 computed_weights = vec2(b0 - b1, c0 - c1) / actual_step;
	return vec3(1.0 - (computed_weights.x + computed_weights.y), computed_weights.xy);
}

vec2 texture2DMSCoord(const in vec2 coords, const in uint flags)
{
	if (0u == (flags & (WRAP_S_MASK | WRAP_T_MASK)))
	{
		return coords;
	}

	const vec2 wrapped_coords_raw = mod(coords, vec2(1.0));
	const vec2 wrapped_coords = mod(wrapped_coords_raw + vec2(1.0), vec2(1.0));
	const bvec2 wrap_control_mask = bvec2(uvec2(flags) & uvec2(WRAP_S_MASK, WRAP_T_MASK));
	return _select(coords, wrapped_coords, wrap_control_mask);
}

)"
