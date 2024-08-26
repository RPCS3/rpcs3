R"(
vec4 texelFetch2DMS(in _MSAA_SAMPLER_TYPE_ tex, const in vec2 sample_count, const in ivec2 icoords, const in int index, const in ivec2 offset)
{
	const vec2 resolve_coords = vec2(icoords + offset);
	const vec2 aa_coords = floor(resolve_coords / sample_count);               // AA coords = real_coords / sample_count
	const vec2 sample_loc = fma(aa_coords, -sample_count, resolve_coords);     // Sample ID = real_coords % sample_count
	const float sample_index = fma(sample_loc.y, sample_count.y, sample_loc.x);
	return texelFetch(tex, ivec2(aa_coords), int(sample_index));
}

vec4 sampleTexture2DMS(in _MSAA_SAMPLER_TYPE_ tex, const in vec2 coords, const in int index)
{
	const uint flags = TEX_FLAGS(index);
	const vec2 scaled_coords = COORD_SCALE2(index, coords);
	const vec2 normalized_coords = texture2DMSCoord(scaled_coords, flags);
	const vec2 sample_count = vec2(2., textureSamples(tex) * 0.5);
	const vec2 image_size = textureSize(tex) * sample_count;
	const ivec2 icoords = ivec2(normalized_coords * image_size);
	const vec4 sample0 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(0));

	if (_get_bits(flags, FILTERED_MAG_BIT, 2) == 0)
	{
		return sample0;
	}

	// Bilinear scaling, with upto 2x2 downscaling with simple weights
	const vec2 uv_step = 1.0 / vec2(image_size);
	const vec2 actual_step = vec2(dFdx(normalized_coords.x), dFdy(normalized_coords.y));

	const bvec2 no_filter = lessThan(abs(uv_step - actual_step), vec2(0.000001));
	if (no_filter.x && no_filter.y)
	{
		return sample0;
	}

	vec4 a, b;
	float factor;
	const vec4 sample2 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(0, 1));     // Top left

	if (no_filter.x)
	{
		// No scaling, 1:1
		a = sample0;
		b = sample2;
	}
	else
	{
		// Filter required, sample more data
		const vec4 sample1 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(1, 0));     // Bottom right
		const vec4 sample3 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(1, 1));     // Top right

		if (actual_step.x > uv_step.x)
		{
		    // Downscale in X, centered
		    const vec3 weights = compute2x2DownsampleWeights(normalized_coords.x, uv_step.x, actual_step.x);

		    const vec4 sample4 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(2, 0));    // Further bottom right
		    a = fma(sample0, weights.xxxx, sample1 * weights.y) + (sample4 * weights.z);            // Weighted sum

		    if (!no_filter.y)
		    {
		        const vec4 sample5 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(2, 1));    // Further top right
		        b = fma(sample2, weights.xxxx, sample3 * weights.y) + (sample5 * weights.z);            // Weighted sum
		    }
		}
		else if (actual_step.x < uv_step.x)
		{
		    // Upscale in X
		    factor = fract(normalized_coords.x * image_size.x);
		    a = mix(sample0, sample1, factor);
		    b = mix(sample2, sample3, factor);
		}
	}

	if (no_filter.y)
	{
		// 1:1 no scale
		return a;
	}
	else if (actual_step.y > uv_step.y)
	{
		// Downscale in Y
		const vec3 weights = compute2x2DownsampleWeights(normalized_coords.y, uv_step.y, actual_step.y);
		// We only have 2 rows computed for performance reasons, so combine rows 1 and 2
		return a * weights.x + b * (weights.y + weights.z);
	}
	else if (actual_step.y < uv_step.y)
	{
		// Upscale in Y
		factor = fract(normalized_coords.y * image_size.y);
		return mix(a, b, factor);
	}
}

)"
