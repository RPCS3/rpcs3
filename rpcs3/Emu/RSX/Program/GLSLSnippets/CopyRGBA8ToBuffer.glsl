R"(
#version 450
layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;

#define IMAGE_LOCATION(x) (x + %loc)
#define SSBO_LOCATION IMAGE_LOCATION(1)

layout(%set, binding=IMAGE_LOCATION(0)) uniform sampler2D colorData;
layout(%set, binding=SSBO_LOCATION, std430) writeonly restrict buffer OutputBlock
{
	uint data[];
};

#if USE_UBO
layout(%push_block) uniform Configuration
{
	uint swap_bytes;
	uint output_pitch;
	uint block_width;
	uint is_bgra;
	ivec2 region_offset;
	ivec2 region_size;
};
#else
	uniform uint swap_bytes;
	uniform uint output_pitch;
	uniform uint block_width;
	uniform uint is_bgra;
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
		if (index > uint(region_size.x * region_size.y))
		{
			return;
		}

		ivec2 coord = linear_id_to_input_coord(index);
		vec4 color = texelFetch(colorData, coord, 0);

		if (is_bgra != 0)
		{
			color = color.bgra;
		}

		// Specific to 8-bit color in ARGB8 format. Need to generalize later
		if (swap_bytes != 0 && block_width > 1)
		{
			color = (block_width == 4) ?
				color.wzyx :
				color.yxwz;
		}

		uvec4 bytes = uvec4(color * 255);
		uint result = bytes.x | (bytes.y << 8u) | (bytes.z << 16u) | (bytes.w << 24u); // UINT_8_8_8_8_REV

		uint output_id = input_coord_to_output_id(coord);
		data[output_id] = result;
	}
}
)"
