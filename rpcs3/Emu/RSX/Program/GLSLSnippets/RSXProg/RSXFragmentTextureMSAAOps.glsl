R"(
#define ZCOMPARE_FUNC(index) _get_bits(TEX_FLAGS(index), DEPTH_COMPARE, 3)
#define ZS_READ_MS(index, coord) vec2(sampleTexture2DMS(TEX_NAME(index), coord, index).r, float(sampleTexture2DMS(TEX_NAME_STENCIL(index), coord, index).x))
#define TEX2D_MS(index, coord2) process_texel(sampleTexture2DMS(TEX_NAME(index), coord2, index), TEX_FLAGS(index))
#define TEX2D_SHADOW_MS(index, coord3) vec4(comparison_passes(sampleTexture2DMS(TEX_NAME(index), coord3.xy, index).x, coord3.z, ZCOMPARE_FUNC(index)))
#define TEX2D_SHADOWPROJ_MS(index, coord4) TEX2D_SHADOW_MS(index, (coord4.xyz / coord4.w))
#define TEX2D_Z24X8_RGBA8_MS(index, coord2) process_texel(convert_z24x8_to_rgba8(ZS_READ_MS(index, coord2), texture_parameters[index].remap, TEX_FLAGS(index)), TEX_FLAGS(index))

vec3 compute2x2DownsampleWeights(const in float coord, const in float uv_step, const in float actual_step)
{
	const float next_sample_point = coord + actual_step;
	const float next_coord_step = fma(floor(coord / uv_step), uv_step, uv_step);
	const float next_coord_step_plus_one = next_coord_step + uv_step;
	vec3 weights = vec3(next_coord_step, min(next_coord_step_plus_one, next_sample_point), max(next_coord_step_plus_one, next_sample_point)) - vec3(coord, next_coord_step, next_coord_step_plus_one);
	return weights / actual_step;
}

)"
