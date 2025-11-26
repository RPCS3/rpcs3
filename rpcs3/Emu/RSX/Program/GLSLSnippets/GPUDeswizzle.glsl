R"(
#version 450

#define SSBO_LOCATION(x) (x + %loc)

#define USE_8BIT_ADDRESSING %_8bit
#define USE_16BIT_ADDRESSING %_16bit

layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;

layout(%set, binding=SSBO_LOCATION(0), std430) buffer ssbo0{ uint data_in[]; };
layout(%set, binding=SSBO_LOCATION(1), std430) buffer ssbo1{ uint data_out[]; };
layout(%push_block) uniform parameters
{
	uint image_width;
	uint image_height;
	uint image_depth;
	uint image_logw;
	uint image_logh;
	uint image_logd;
	uint lod_count;
};

struct invocation_properties
{
	uint data_offset;
	uvec3 size;
	uvec3 size_log2;
};

#define bswap_u16(bits) (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8
#define bswap_u32(bits) (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24

invocation_properties invocation;

bool init_invocation_properties(const in uint offset)
{
	invocation.data_offset = 0;
	invocation.size.x = image_width;
	invocation.size.y = image_height;
	invocation.size.z = image_depth;
	invocation.size_log2.x = image_logw;
	invocation.size_log2.y = image_logh;
	invocation.size_log2.z = image_logd;
	uint level_end = image_width * image_height * image_depth;
	uint level = 1;

	while (offset >= level_end && level < lod_count)
	{
		invocation.data_offset = level_end;
		invocation.size.xy /= 2;
		invocation.size.xy = max(invocation.size.xy, uvec2(1));
		invocation.size_log2.xy = max(invocation.size_log2.xy, uvec2(1));
		invocation.size_log2.xy --;
		level_end += (invocation.size.x * invocation.size.y * image_depth);
		level++;
	}

	return (offset < level_end);
}

uint get_z_index(const in uint x_, const in uint y_, const in uint z_)
{
	uint offset = 0;
	uint shift = 0;
	uint x = x_;
	uint y = y_;
	uint z = z_;
	uint log2w = invocation.size_log2.x;
	uint log2h = invocation.size_log2.y;
	uint log2d = invocation.size_log2.z;

	do
	{
		if (log2w > 0)
		{
			offset |= (x & 1) << shift;
			shift++;
			x >>= 1;
			log2w--;
		}

		if (log2h > 0)
		{
			offset |= (y & 1) << shift;
			shift++;
			y >>= 1;
			log2h--;
		}

		if (log2d > 0)
		{
			offset |= (z & 1) << shift;
			shift++;
			z >>= 1;
			log2d--;
		}
	}
	while(x > 0 || y > 0 || z > 0);

	return offset;
}

#if USE_16BIT_ADDRESSING

void decode_16b(const in uint texel_id, in uint x, const in uint y, const in uint z)
{
	uint accumulator = 0;

	const uint subword_count = min(invocation.size.x, 2);
	for (uint subword = 0; subword < subword_count; ++subword, ++x)
	{
		uint src_texel_id = get_z_index(x, y, z);
		uint src_id = (src_texel_id + invocation.data_offset);
		int src_bit_offset = int(src_id % 2) << 4;
		uint src_value = bitfieldExtract(data_in[src_id / 2], src_bit_offset, 16);
		accumulator = bitfieldInsert(accumulator, src_value, int(subword << 4), 16);
	}

	data_out[texel_id / 2] = %f(accumulator);
}

#elif USE_8BIT_ADDRESSING

void decode_8b(const in uint texel_id, in uint x, const in uint y, const in uint z)
{
	uint accumulator = 0;

	const uint subword_count = min(invocation.size.x, 4);
	for (uint subword = 0; subword < subword_count; ++subword, ++x)
	{
		uint src_texel_id = get_z_index(x, y, z);
		uint src_id = (src_texel_id + invocation.data_offset);
		int src_bit_offset = int(src_id % 4) << 3;
		uint src_value = bitfieldExtract(data_in[src_id / 4], src_bit_offset, 8);
		accumulator = bitfieldInsert(accumulator, src_value, int(subword << 3), 8);
	}

	data_out[texel_id / 4] = accumulator;
}

#else

void decode_32b(const in uint texel_id, const in uint word_count, const in uint x, const in uint y, const in uint z)
{
	uint src_texel_id = get_z_index(x, y, z);
	uint dst_id = (texel_id * word_count);
	uint src_id = (src_texel_id + invocation.data_offset) * word_count;

	for (uint i = 0; i < word_count; ++i)
	{
		uint value = data_in[src_id++];
		data_out[dst_id++] = %f(value);
	}
}

#endif

void main()
{
	uint invocations_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
	uint texel_id = (gl_GlobalInvocationID.y * invocations_x) + gl_GlobalInvocationID.x;
	uint word_count = %_wordcount;

#if USE_8BIT_ADDRESSING
	texel_id *= 4;        // Each invocation consumes 4 texels
#elif USE_16BIT_ADDRESSING
	texel_id *= 2;        // Each invocation consumes 2 texels
#endif

	if (!init_invocation_properties(texel_id))
		return;

	// Calculations done in texels, not bytes
	uint row_length = invocation.size.x;
	uint slice_length = (invocation.size.y * row_length);
	uint level_offset = (texel_id - invocation.data_offset);
	uint slice_offset = (level_offset % slice_length);
	uint z = (level_offset / slice_length);
	uint y = (slice_offset / row_length);
	uint x = (slice_offset % row_length);

#if USE_8BIT_ADDRESSING
	decode_8b(texel_id, x, y, z);
#elif USE_16BIT_ADDRESSING
	decode_16b(texel_id, x, y, z);
#else
	decode_32b(texel_id, word_count, x, y, z);
#endif

}
)"
