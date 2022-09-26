R"(
#version 450
layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;

#define IMAGE_LOCATION(x) (x + %loc)
#define SSBO_LOCATION IMAGE_LOCATION(2)

#define bswap_u32(bits) (bits & 0xFFu) << 24u | (bits & 0xFF00u) << 8u | (bits & 0xFF0000u) >> 8u | (bits & 0xFF000000u) >> 24u

layout(%set, binding=IMAGE_LOCATION(0)) uniform sampler2D depthData;
layout(%set, binding=IMAGE_LOCATION(1)) uniform usampler2D stencilData;

layout(%set, binding=SSBO_LOCATION, std430) writeonly restrict buffer OutputBlock
{
	uint data[];
};

#if USE_UBO
layout(%push_block) uniform Configuration
{
	uint swap_bytes;
	uint output_pitch;
	ivec2 region_offset;
	ivec2 region_size;
};
#else
	uniform uint swap_bytes;
	uniform uint output_pitch;
	uniform ivec2 region_offset;
	uniform ivec2 region_size;
#endif

#define KERNEL_SIZE %wks

uint linear_invocation_id()
{
	uint size_in_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
	return (gl_GlobalInvocationID.y * size_in_x) + gl_GlobalInvocationID.x;
}

ivec2 linear_id_to_input_coord(uint index)
{
	return ivec2(int(index % region_size.x), int(index / output_pitch)) + region_offset;
}

uint input_coord_to_output_id(ivec2 coord)
{
	coord -= region_offset;
	return coord.y * output_pitch + coord.x;
}

void main()
{
	uint index = linear_invocation_id() * KERNEL_SIZE;

	for (int loop = 0; loop < KERNEL_SIZE; ++loop, ++index)
	{
		if (index > (region_size.x * region_size.y))
		{
			return;
		}

		ivec2 coord = linear_id_to_input_coord(index);
		float depth = texelFetch(depthData, coord, 0).x;
		uint stencil = texelFetch(stencilData, coord, 0).x;
		uint depth_bytes = uint(depth * 0xffffff);
		uint value = (depth_bytes << 8) | stencil;

		if (swap_bytes != 0)
		{
			// PS3-style byteswap (full word). PC byteswap is slightly different.
			value = bswap_u32(value);
		}

		data[input_coord_to_output_id(coord)] = value;
	}
}
)"
